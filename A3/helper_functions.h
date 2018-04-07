#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H

#include "ext2.h"

#define BLOCK_SIZE 1024
#define ROOT_INODE_INDEX 1
#define INODE_TABLE_BLOCK 5
#define INODE_SIZE 128
#define NUMBER_OF_SINGLE_POINTERS 12
#define INODE_BITMAP_BLOCK 4
#define BLOCK_BITMAP_BLOCK 3
#define INODE_BITMAP_BYTES 4
#define MAX_ARGUMENTS 4
#define MIN_ARGUMENTS 3
#define FIRST_ARGUMENT 1
#define SECOND_ARGUMENT 2
#define THIRD_ARGUMENT 3
#define BLOCK_BITMAP_BYTES 16
#define NUMBER_OF_INDIRECT_POINTERS 256
#define ROOT_BLOCK 2

int last_sep_index (char *path);

char *parent_path(int index, char *path);

char *last_file_name(int index, char *path);

struct ext2_dir_entry_2 *findDirEntryInBlock(unsigned char *disk, int blockNum, char *name);

unsigned int findInodeInDir(unsigned char *disk, struct ext2_inode *inode, char *path);

unsigned int walkPath(unsigned char *disk, char *path);

int mod_round(int x);

int last_block_in_dir (struct ext2_inode *parent_inode);

void set_bitmap(unsigned char *bitmap, int block_num, int bit);

int find_free_inode(unsigned char *inode_bitmap);

int find_free_block(unsigned char *block_bitmap);

#endif
