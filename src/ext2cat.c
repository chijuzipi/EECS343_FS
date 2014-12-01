#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "ext2_access.h"
#include "mmapfs.h"


int main(int argc, char ** argv) {
    // Extract the file given in the second argument from the filesystem image
    // given in the first.
    if (argc != 3) {
        printf("usage: ext2cat <filesystem_image> <path_within_filesystem>\n");
        exit(1);
    }
    char * fs_path = argv[1];
    char * file_path = argv[2];
    void * fs = mmap_fs(fs_path);

    // Get the inode for our target file.
    __u32 target_ino_num = get_inode_by_path(fs, file_path);
    if (target_ino_num == 0) {
        printf("cat: %s does not exist\n", file_path);
        return 1;
    }

    struct ext2_inode * target_ino = get_inode(fs, target_ino_num);

    __u32 block_size = get_block_size(fs);
    __u32 size = target_ino->i_size;
      //printf("the block size is %d\n", block_size);
      //printf("the size is %d\n", size);
    __u32 bytes_read = 0;
    void * buf = calloc(size, 1);
    void * buf2 = calloc(size, 1);

    // Read the file one block at a time. In the real world, there would be a
    // lot of error-handling code here.
    __u32 bytes_left;
    for (int i = 0; i < EXT2_NDIR_BLOCKS; i++) {
        bytes_left = size - bytes_read;
        if (bytes_left == 0) break;
        //printf("the bytes_left is %d\n", bytes_left);
        __u32 bytes_to_read = bytes_left > block_size ? block_size : bytes_left;
        void * block = get_block(fs, target_ino->i_block[i]);
        memcpy(buf + bytes_read, block, bytes_to_read);
        bytes_read += bytes_to_read;
    }
    
    /* check if the readed byte is smaller than the file size */
    /* go to indirect block if it does */
    if (bytes_read < size){
      for (int i = 0; i < EXT2_IND_BLOCK; i++) {

        /* the indirect pointer start after direct block, which is 12 */
        void * pointer_block = get_block(fs, target_ino->i_block[12]);
          __u32 offset = 0;

          /* iterative until the size is matched */
          while (bytes_read < size){

            int * address = pointer_block + offset;
            bytes_left = size - bytes_read;
            if (bytes_left == 0) break;
            __u32 bytes_to_read = bytes_left > block_size ? block_size : bytes_left;
            void * block = get_block(fs, * address);
            memcpy(buf + bytes_read, block, bytes_to_read);
            bytes_read += bytes_to_read;
            offset += 4;
            if (offset >= block_size)
              break;
          }

          /* redirect the output to a file named img.jpg */
          FILE *f = fopen("output.jpg", "w");
          if (f == NULL)
          {
              printf("Error opening file!\n");
                  exit(1);
          }
          fwrite(buf, 1, bytes_read, f);
          fclose(f);

          return 0;
      }
    }

    else{
          /* redirect the output to a file named img.jpg */
          FILE *f = fopen("output.jpg", "w");
          if (f == NULL)
          {
              printf("Error opening file!\n");
                  exit(1);
          }
          fwrite(buf, 1, bytes_read, f);
          fclose(f);
    }
  return 0;
}

