 #include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include "helper_functions.h"

/*
* returns the index of '\' before the last file name in path
*/
int last_sep_index (char *path) {

    int path_len = strlen(path);
    if((path_len == 1) && (path[0] == '/')) {
        return 0;
    }
    int not_found = 1;
    int index = path_len;
    while(not_found) {
        index = index -1;
        if ((path[index] == '/') && (index != path_len - 1)){
            not_found = 0;
        }
        //invalid path error
        if(index == -1) {
            return -1;
        }
    }
    return index;
}

/*
* input: index of last '/' and a path
* returns path of the parent of the last file in given path
* returns "/" if parent is the root or if path is the root
*/
char *parent_path(int index, char *path) {

    if(index == 0) {
        //parent path is just '/'
        char *separator = malloc(sizeof(char) * 2);
        separator[0] = '/';
        separator[1] = '\0';
        return separator;
    }
    char *parent = malloc(sizeof(char) * (index + 1));
	int i;
    for(i = 0; i < (index + 1); i++){
        parent[i] = path[i];
    }
    //must add terminating char at the end of string
    parent[index + 1] = '\0';
    return parent;
}

/*
* input: index of last '/' and a path
* returns the name of the last file in given path
* returns "/" if path is the root
*/
char *last_file_name(int index, char *path) {

    int path_len = strlen(path);
    char separator[2] = "/";
    if(index == 0) {
        if(path_len == 1) {
            return path;
        } else {
            return strtok(path, separator);
        }
    }
    int file_name_len = path_len - index;
    char *file_name = malloc(file_name_len * sizeof(char));
    int count = 0;
	int i;
    for(i = index + 1; i < path_len; i++) {
        file_name[count] = path[i];
        count ++;
    }
    //add terminating char at the end
    file_name[file_name_len] = '\0';
    return strtok(file_name, separator);
}

/*
* find free block using block bitmap
*/
int find_free_block(unsigned char *block_bitmap) {
	int i;
    for (i = 0; i < BLOCK_BITMAP_BYTES; i++) {
        unsigned char b = block_bitmap[i];
		int j;
        for(j = 0; j < 8; j++) {
            if(!(b >> j & 1)) {
                //i byte, j bits is the free block
                return (i * 8 + j + 1);
            }
        }
    }
    return -1;
}

/*
* find free inode using block bitmap
*/
int find_free_inode(unsigned char *inode_bitmap) {
	int i;
    for (i = 0; i < INODE_BITMAP_BYTES; i++) {
        unsigned char b = inode_bitmap[i];
		int j;
        for(j = 0; j < 8; j++) {
            if(!(b >> j & 1)) {
                //i byte, j bits is the free inode
                return (i * 8 + j + 1);
            }
        }
    }
    return -1;
}

/*
* set free block given free block number (from find_free_block/inode)
*/
void set_bitmap(unsigned char *bitmap, int block_num, int bit) {

    int j = (block_num - 1) % 8;
    int i = (block_num - 1 - j) / 8;
    unsigned char b = bitmap[i];
	unsigned char temp = 1;
	int count;    
	for(count = 0; count < j; count ++) {
        temp = temp << 1; 
    }
	if (bit == 0){
		bitmap[i] = b ^ temp;
	}
	else {
		bitmap[i] = b | temp;
	}
    
}

struct ext2_dir_entry_2 *findDirEntryInBlock(unsigned char *disk, int blockNum, char *name){
	int offset = 0;
	//iterate over all the directory entries in the block
	while(offset < BLOCK_SIZE){
		struct ext2_dir_entry_2 *dirEntry = (struct ext2_dir_entry_2 *)(disk + (BLOCK_SIZE * blockNum) + offset);
		char entryName[dirEntry->name_len + 1];
		memcpy(entryName, &dirEntry->name, dirEntry->name_len);
		entryName[dirEntry->name_len] = '\0';
		//strcmp returns 0 when the two str are equal
		if(strcmp(entryName, name) == 0){
			return dirEntry;
		}
		//increment the offset by the size of the previous dirEntry
		offset = offset + dirEntry->rec_len;
	}
	return NULL;
}

unsigned int findInodeInDir(unsigned char *disk, struct ext2_inode *inode, char *path){
	//ensure inode is a directory
	if ((inode->i_mode & EXT2_S_IFDIR) == EXT2_S_IFDIR){
		int i;
		struct ext2_dir_entry_2 *dirEntry;
		//iterate over all the direct block pointers
		for(i = 0; i < NUMBER_OF_SINGLE_POINTERS; i++) {
			//check if the current block is valid
			if(inode->i_block[i]){
				//search the block for a dir entry with the specified path name
				dirEntry = findDirEntryInBlock(disk, inode->i_block[i], path);
				if(dirEntry != NULL){
					//in this case, we found the directory entry, now we find and return the inode associated with that directory entry
					unsigned int inodeNumber = (dirEntry->inode);
					//struct ext2_inode *inode = (struct ext2_inode *)(disk + INODE_TABLE_BLOCK * BLOCK_SIZE + INODE_SIZE * inodeIndex);
					//return inode;
					return inodeNumber;
				}
			}	
		}
	}
	return 0;
}

unsigned int walkPath(unsigned char *disk, char *path){
	char tempPath[strlen(path) + 1];
	strcpy(tempPath, path);
	//get the root inode
	struct ext2_inode *currInode = (struct ext2_inode *)(disk + INODE_TABLE_BLOCK * BLOCK_SIZE + INODE_SIZE);
	if(strcmp(path, "/") == 0){
		return ROOT_BLOCK;
	}
	//Iterate through each directory/file in the path
	const char delim[2] = "/";
	char *curr;
	curr = strtok(tempPath, delim);
	unsigned int inodeNumber = 0;
	while(curr != NULL) {
		inodeNumber = findInodeInDir(disk, currInode, curr);
		if (inodeNumber == 0){
			//There was no file/directory with the specified name
			return 0;
		}
		currInode = (struct ext2_inode *)(disk + INODE_TABLE_BLOCK * BLOCK_SIZE + INODE_SIZE * (inodeNumber - 1));
		curr = strtok(NULL, delim);
	}
	return inodeNumber;
}

/*
* input: integer x
* output: x rounded up so it is divisible by 4
*/
int mod_round(int x) {
    int temp = 4 - (x % 4);
    int res = x + temp;
    return res;
}

int last_block_in_dir (struct ext2_inode *parent_inode) {
    int block_count = 0;
     while((parent_inode -> i_block[block_count + 1]) && block_count != 15) {
        block_count ++;
    }
    return block_count;
}
