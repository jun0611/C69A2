#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H

#define BLOCK_SIZE 1024
#define ROOT_INODE_INDEX 1
#define INODE_TABLE_BLOCK 5
#define INODE_SIZE 128
#define NUMBER_OF_SINGLE_POINTERS 12
#define INODE_BITMAP_BLOCK 4
#define INODE_BITMAP_BYTES 4

int last_sep_index (char *path);

char *parent_path(int index, char *path);

char *last_file_name(int index, char *path);

#endif
