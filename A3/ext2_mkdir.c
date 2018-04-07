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

    if(argc != 3) {
        fprintf(stderr, "Usage: ext2_ls <image file name> <absolute path>\n");
        exit(1);
    }
    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
    perror("mmap");
    exit(1);
    }
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + BLOCK_SIZE);
    struct ext2_group_desc *gd =  (struct ext2_group_desc *)(disk + BLOCK_SIZE * 2);
    char *absolutePath = argv[2];
    int seperator_index = last_sep_index(absolutePath);
    char *par_path = parent_path(seperator_index, absolutePath);
    char *file_name = last_file_name(seperator_index, absolutePath);
    
    //find inode of parent if it exists
    unsigned int check_path = walkPath(disk, absolutePath);
    if(!(check_path)) {
        exit(EEXIST);
    }
    unsigned int parent_inode_index = walkPath(disk, par_path);
    if(!(parent_inode_index)) {
        exit(ENOENT);
    }
    unsigned int inode_table_block = gd -> bg_inode_table;
    struct ext2_inode *parent_inode = (struct ext2_inode *)(disk + (BLOCK_SIZE *inode_table_block)
        + parent_inode_index);
    //make sure inode is a directory
    if ((parent_inode->i_mode & EXT2_S_IFDIR) == EXT2_S_IFDIR) {
        //find free block
        unsigned int blocks_bitmap_block = gd->bg_block_bitmap;
        void *block_bitmap = (void *)(disk + blocks_bitmap_block * BLOCK_SIZE);
        int free_block_num = find_free_block(block_bitmap);
        unsigned int inodes_bitmap_block = gd->bg_inode_bitmap;
        void *inode_bitmap = (void *)(disk + inodes_bitmap_block * BLOCK_SIZE);
        int free_inode_num = find_free_inode(inode_bitmap);
        //create new inode
        struct ext2_inode *new_inode = (struct ext2_inode *)(disk + (BLOCK_SIZE * inode_table_block) + free_inode_num);
        new_inode -> i_mode = EXT2_S_IFDIR;
        new_inode -> i_size = INODE_SIZE;
        new_inode -> i_links_count = 2;
        new_inode -> i_blocks = 2;
        unsigned int first_data_block = sb -> s_first_data_block;
        new_inode -> i_block[0] = first_data_block + free_block_num;
        //create directory entry . (current directory)
        struct ext2_dir_entry_2 *cur_dir_entry = (struct ext2_dir_entry_2 *)(disk + (BLOCK_SIZE * 
            (new_inode -> i_block[0])));
        cur_dir_entry -> inode = free_inode_num;
        cur_dir_entry -> rec_len = 12;
        cur_dir_entry -> name_len = 1;
        cur_dir_entry -> file_type = EXT2_FT_DIR;
        strcpy(new_dir_entry -> name, ".");
        //create directory entry for .. (parent)
        struct ext2_dir_entry_2 *parent_dir_entry = (struct ext2_dir_entry_2 *)(disk + (BLOCK_SIZE * 
            (new_inode -> i_block[0])) + (cur_dir_entry -> rec_len));
        parent_dir_entry -> inode = parent_inode_index;
        parent_dir_entry -> rec_len = BLOCK_SIZE - 12;
        parent_dir_entry -> name_len = 2;
        parent_dir_entry -> file_type = EXT2_FT_DIR;
        strcpy(new_dir_entry -> name, "..");
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
                int new_entry_size = strlen(file_name) + 8;
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
                        new_entry -> inode = free_inode_num;
                        new_entry -> rec_len = BLOCK_SIZE;
                        new_entry -> name_len = strlen(file_name);
                        new_entry -> file_type = EXT2_FT_DIR;
                        strncpy(new_entry -> name, file_name, strlen(file_name));
                        break;
                    }
                } else {
                        struct ext2_dir_entry_2 *new_entry = (struct ext2_dir_entry_2 *)(disk +
                             (BLOCK_SIZE * (parent_inode -> i_block[block_count]) + total_rec_len + entry_size));
                        new_entry -> inode = free_inode_num;
                        new_entry -> rec_len = available_space;
                        new_entry -> name_len = strlen(file_name);
                        new_entry -> file_type = EXT2_FT_DIR;
                        strncpy(new_entry -> name, file_name, strlen(file_name));
                        // update previous entry rec_len
                        dir_entry -> rec_len = total_rec_len + entry_size;
                        break;
                }
            }
            total_rec_len = total_rec_len + (dir_entry -> rec_len);
        }
        // update super block
        sb -> s_inodes_count += 1;
        sb -> s_blocks_count += 1;
        // update bit map
        set_bitmap(inode_bitmap, free_inode_num, 1);
        set_bitmap(block_bitmap, free_block_num, 1);
    return 0;
}
