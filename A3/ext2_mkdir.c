#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2.h"


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
    char *parent_path = parent_path(seperator_index, absolutePath);
    char *file_name = last_file_name(seperator_index, absolutePath);
    //find inode of parent if it exists
    struct ext2_inode *check_path = walkPath(absolutePath);
    if(check_path != ENOENT) {
        exit(EEXIST);
    }
    struct ext2_inode *parent_inode = walkPath(parent_path);
    if(parent_inode == ENOENT) {
        exit(ENOENT);
    }
    //make sure inode is a directory
    if ((inode->i_mode & EXT2_S_IFDIR) == EXT2_S_IFDIR) {
        //find free block
        unsigned int blocks_bitmap_block = gd->bg_block_bitmap;
        void *block_bitmap = (void *)(disk + blocks_bitmap_block * BLOCK_SIZE);
        int free_block = find_free_block(block_bitmap);
        unsigned int inodes_bitmap_block = gd->bg_inode_bitmap;
        void *inode_bitmap = (void *)(disk + inodes_bitmap_block * BLOCK_SIZE);
        int free_inode_num = find_free_inode(inode_bitmap);
        //create new inode
        unsigned int inode_table_block = gd -> bg_inode_table;
        struct ext2_inode *new_inode = (struct ext2_inode *)(disk + (BLOCK_SIZE * inode_table_block + free_inode_num));
    }
    return 0;
}