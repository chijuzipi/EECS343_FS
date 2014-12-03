// ext2 definitions from the real driver in the Linux kernel.
#include <assert.h> 
#include "ext2fs.h"

// This header allows your project to link against the reference library. If you
// complete the entire project, you should be able to remove this directive and
// still compile your code.
//#include "reference_implementation.h"

// Definitions for ext2cat to compile against.
#include "ext2_access.h"



///////////////////////////////////////////////////////////
//  Accessors for the basic components of ext2.
///////////////////////////////////////////////////////////

// Return a pointer to the primary superblock of a filesystem.
struct ext2_super_block * get_super_block(void * fs) {
    struct ext2_super_block * super;
    super = fs + SUPERBLOCK_OFFSET;
    return super;
}


// Return the block size for a filesystem.
__u32 get_block_size(void * fs) {
    __u32 size;
    size = get_super_block(fs)->s_log_block_size;
    size = 1024 << size;
    return size;
}


// Return a pointer to a block given its number.
// get_block(fs, 0) == fs;
void * get_block(void * fs, __u32 block_num) {
    __u32 block_size = get_block_size(fs);
    void * block_p = fs + (block_num * block_size);
    return block_p;
}


// Return a pointer to the first block group descriptor in a filesystem. Real
// ext2 filesystems will have several of these, but, for simplicity, we will
// assume there is only one.
struct ext2_group_desc * get_block_group(void * fs, __u32 block_group_num) {
    
    __u32 block_size = get_block_size(fs);
    __u32 bgd_block = (SUPERBLOCK_OFFSET / block_size) + 1;
    printf("the bgd_block is %d\n", bgd_block);
    struct ext2_group_desc * egd;
    egd = get_block(fs, bgd_block);
    return egd;
}


// Return a pointer to an inode given its number. In a real filesystem, this
// would require finding the correct block group, but you may assume it's in the
// first one.
struct ext2_inode * get_inode(void * fs, __u32 inode_num) {
    struct ext2_group_desc * bgd = get_block_group(fs, 0);
    __u32 inode_table_block = bgd->bg_inode_table;
    struct ext2_inode * inode_table = get_block(fs, inode_table_block);
    return inode_table -1 + inode_num;
}



///////////////////////////////////////////////////////////
//  High-level code for accessing filesystem components by path.
///////////////////////////////////////////////////////////

// Chunk a filename into pieces.
// split_path("/a/b/c") will return {"a", "b", "c"}.
//
// This one's a freebie.
char ** split_path(char * path) {
    int num_slashes = 0;
    for (char * slash = path; slash != NULL; slash = strchr(slash + 1, '/')) {
        num_slashes++;
    }

    // Copy out each piece by advancing two pointers (piece_start and slash).
    char ** parts = (char **) calloc(num_slashes, sizeof(char *));
    char * piece_start = path + 1;
    int i = 0;
    for (char * slash = strchr(path + 1, '/');
         slash != NULL;
         slash = strchr(slash + 1, '/')) {
        int part_len = slash - piece_start;
        parts[i] = (char *) calloc(part_len + 1, sizeof(char));
        strncpy(parts[i], piece_start, part_len);
        piece_start = slash + 1;
        i++;
    }
    // Get the last piece.
    parts[i] = (char *) calloc(strlen(piece_start) + 1, sizeof(char));
    strncpy(parts[i], piece_start, strlen(piece_start));
    return parts;
}


// Convenience function to get the inode of the root directory.
struct ext2_inode * get_root_dir(void * fs) {
    return get_inode(fs, EXT2_ROOT_INO);
}


// Given the inode for a directory and a filename, return the inode number of
// that file inside that directory, or 0 if it doesn't exist there.
// 
// name should be a single component: "foo.txt", not "/files/foo.txt".
__u32 get_inode_from_dir(void * fs, struct ext2_inode * dir, 
        char * name) {

    //assert the file system is in dir mode
    assert(LINUX_S_ISDIR(dir->i_mode));

    __u32 blockSize = get_block_size(fs);
    void * dir_block = get_block(fs, dir->i_block[0]);
    struct ext2_dir_entry * entry = (struct ext2_dir_entry *) dir_block;
    void * end = ((void *) entry) + blockSize;

    __u32 target_inode = 0;
    // Iterate over each entry of the directory, looking for a match for name.
    while ((void *) entry < end) {
        // check the unused entry first 
        if (entry->inode == 0) {
            continue;
        }

        if (strlen(name) == (unsigned char) (entry->name_len) &&
            strncmp(name, entry->name, strlen(name)) == 0) {
            target_inode = entry->inode;
        }

        // go to the next dir name.
        entry = (struct ext2_dir_entry *) (((void *) entry) + entry->rec_len);
    }

    if (target_inode != 0) {
        return target_inode;
    } else {
        return 0;
    }
}


// Find the inode number for a file by its full path.
// This is the functionality that ext2cat ultimately needs.
__u32 get_inode_by_path(void * fs, char * path) {

    char ** parts = split_path(path);
    __u32 ino_number = EXT2_ROOT_INO;
    for (char ** part = parts; *part != NULL; part++) {
        struct ext2_inode * ino = get_inode(fs, ino_number);

        //assert the i_mode
        if (!LINUX_S_ISDIR(ino->i_mode))
          break;

        ino_number = get_inode_from_dir(fs, ino, *part);
        if (ino_number == 0) {
            break;
        }
    }

    if (ino_number == EXT2_ROOT_INO){
        return 0;
    } else {
        return ino_number;
    }
}

