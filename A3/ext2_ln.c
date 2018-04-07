#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2.h"
#include <errno.h>
#include <string.h>

#include "helper_functions.h"

unsigned char *disk;

int main(int argc, char **argv) {

    if(argc > 5) {
        fprintf(stderr, "Usage: ext2_ls <image file name> <absolute path>\n");
        exit(1);
    } else {
        int fd = open(argv[1], O_RDWR);

        disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
        }
        struct ext2_super_block *sb = (struct ext2_super_block *)(disk + BLOCK_SIZE);
        struct ext2_group_desc *gd =  (struct ext2_group_desc *)(disk + BLOCK_SIZE * 2);
        char *file_path;
        char *link_path;
        int is_hard = 0;
        if(argc == 4) {
            file_path = argv[2];
            link_path = argv[3];
            is_hard = 1;
        }
        if(argc == 5) {
            file_path = argv[3];
            link_path = argv[4];
        }
        unsigned int file_path_index = walkPath(disk, file_path);
        //check if file exists
        if(!(file_path_index)) {
            exit(ENOENT);
        }
        unsigned int link_path_index = walkPath(disk, link_path);
        //check if link name already exist
        if(link_path_index != 0) {
            exit(EEXIST);
        }
        unsigned int inode_table_block = gd -> bg_inode_table;
        struct ext2_inode *file_inode = (struct ext2_inode *)(disk + (BLOCK_SIZE *inode_table_block)
        + file_path_index);
        if ((file_inode->i_mode & EXT2_S_IFDIR) == EXT2_S_IFDIR) {
            exit(EISDIR);
        }
        int sep_index = last_sep_index(link_path);
        char *parent_of_link = parent_path(sep_index, link_path);
        char *link_name = last_file_name(sep_index, link_path);
        unsigned int parent_inode_index = walkPath(disk, parent_of_link);
        struct ext2_inode *parent_inode = (struct ext2_inode *)(disk + (BLOCK_SIZE *inode_table_block)
            + parent_inode_index);
        //for symlink create new inode
        unsigned int blocks_bitmap_block = gd->bg_block_bitmap;
        void *block_bitmap = (void *)(disk + blocks_bitmap_block * BLOCK_SIZE);
        unsigned int inodes_bitmap_block = gd->bg_inode_bitmap;
        void *inode_bitmap = (void *)(disk + inodes_bitmap_block * BLOCK_SIZE);
        int free_inode_num = find_free_inode(inode_bitmap);
        if(!(is_hard)) {
            int free_block_num = find_free_block(block_bitmap);
            //create new inode
            struct ext2_inode *new_inode = (struct ext2_inode *)(disk + (BLOCK_SIZE * inode_table_block) + free_inode_num);
            new_inode -> i_mode = EXT2_S_IFLNK;
            new_inode -> i_size = INODE_SIZE;
            new_inode -> i_links_count = 1;
            new_inode -> i_blocks = 1;
            unsigned int first_data_block = sb -> s_first_data_block;
            new_inode -> i_block[0] = first_data_block + free_block_num;
            //save path to file
            memcpy((unsigned int)new_inode -> i_block[0], file_path, strlen(file_path));
                // update super block
            sb -> s_inodes_count += 1;
            sb -> s_blocks_count += 1;
            // update bit map
            set_bitmap(inode_bitmap, free_inode_num, 1);
            set_bitmap(block_bitmap, free_block_num, 1);
        }
        //make sure inode is a directory
        if ((parent_inode->i_mode & EXT2_S_IFDIR) == EXT2_S_IFDIR) {
            //find the end of parent directory block
            int block_count = last_block_in_dir(parent_inode);
            // find end space in the block
            int total_rec_len = 0;
            while(total_rec_len < BLOCK_SIZE) {
                struct ext2_dir_entry_2 *dir_entry = (struct ext2_dir_entry_2 *)(disk +
                    (BLOCK_SIZE * (parent_inode -> i_block[block_count])) + total_rec_len);
                int entry_rec_len = dir_entry -> rec_len;
                // this is the last dir entry
                if((total_rec_len + entry_rec_len) == BLOCK_SIZE) {
                    int entry_size = (dir_entry -> name_len) + 8;
                    // entry_size most be divisible by 4
                    if((entry_size % 4) != 0) {
                        entry_size = mod_round(entry_size);
                    }
                    int available_space = entry_rec_len - entry_size;
                    int new_entry_size = strlen(link_name) + 8;
                    if((new_entry_size % 4) != 0) {
                        new_entry_size = mod_round(new_entry_size);
                    }
                    // create dir entry for new dir
                    if(available_space < new_entry_size) {
                        // not enough space
                        if(block_count == 12) {
                            printf("Not enough space");
                            exit(1);
                        } else {
                            //use the next block
                            struct ext2_dir_entry_2 *new_entry = (struct ext2_dir_entry_2 *)(disk +
                                (BLOCK_SIZE * (parent_inode -> i_block[block_count + 1])));
                            if(is_hard){
                                new_entry -> inode = file_path_index;
                                new_entry -> file_type = EXT2_FT_REG_FILE;
                            } else {
                                new_entry -> inode = free_inode_num;
                                new_entry -> file_type = EXT2_FT_SYMLINK;
                            }
                            new_entry -> rec_len = BLOCK_SIZE;
                            new_entry -> name_len = strlen(link_name);
                            
                            strncpy(new_entry -> name, link_name, strlen(link_name));
                            break;
                        }
                    } else {
                            struct ext2_dir_entry_2 *new_entry = (struct ext2_dir_entry_2 *)(disk +
                                (BLOCK_SIZE * (parent_inode -> i_block[block_count]) + total_rec_len + entry_size));
                            if(is_hard) {
                                new_entry -> inode = file_path_index;
                                new_entry -> file_type = EXT2_FT_REG_FILE;
                            } else {
                                new_entry -> inode = free_inode_num;
                                new_entry -> file_type = EXT2_FT_SYMLINK;
                            }
                            new_entry -> rec_len = available_space;
                            new_entry -> name_len = strlen(link_name);
                            strncpy(new_entry -> name, link_name, strlen(link_name));
                            // update previous entry rec_len
                            dir_entry -> rec_len = total_rec_len + entry_size;
                            break;
                    }
                }
                total_rec_len = total_rec_len + (dir_entry -> rec_len);
            }
            if(is_hard) {
                //increment link count in file i_node
                file_inode -> i_links_count += 1;
            } 
        }
    }
}
