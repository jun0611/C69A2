#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2.h"
#include <string.h>

#define BLOCK_SIZE 1024
#define ROOT_INODE_INDEX 1
#define INODE_TABLE_BLOCK 5
#define INODE_SIZE 128
#define NUMBER_OF_SINGLE_POINTERS 12

unsigned char *disk;
char *absolutePath;

void walkPath(char *path){
	//get the root inode
	struct ext2_inode *currInode = (struct ext2_inode *)(disk + INODE_TABLE_BLOCK * BLOCK_SIZE) + (INODE_SIZE);

	//Iterate through each directory/file in the path
	const char delim[1] = "/";
	char *curr;
	curr = strtok(path, delim);
	while(curr != NULL) {
		currInode = findInodeInDir(currInode, curr);
		if (currInode == NULL){
			//There was no file/directory with the specified name
			return ENOENT;
		}
		curr = strtok(NULL, delim);
	}
}

struct ext2_inode findInodeInDir(struct ext2_inode inode, char *path){
	//ensure inode is a directory
	if (inode->i_mode & EXT2_S_IFDIR) == EXT2_S_IFDIR){
		int i;
		struct ext2_dir_entry_2 dirEntry;
		//iterate over all the direct block pointers
		for(i = 0; i < NUMBER_OF_SINGLE_POINTERS; i++) {
			//check if the current block is valid
			if(inode->i_block[i]){
				//search the block for a dir entry with the specified path name
				dirEntry = findDirEntryInBlock(inode->i_block[i], path);
				if(dirEntry != NULL){
					//in this case, we found the directory entry, now we find and return the inode associated with that directory entry
					unsigned int inodeIndex = (dirEntry->inode) - 1;
					struct ext2_inode *inode = (struct ext2_inode *)(disk + INODE_TABLE_BLOCK * BLOCK_SIZE) + (INODE_SIZE * inodeIndex);
					return inode;
				}
			}	
		}
	}
	return NULL;
}

struct ext2_dir_entry_2 findDirEntryInBlock(int blockNum, char *name){
	int offset = 0;
	//iterate over all the directory entries in the block
	while(offset < BLOCK_SIZE){
		struct ext2_dir_entry_2 *dirEntry = (struct ext2_dir_entry_2 *)(disk + (BLOCK_SIZE * blockNum) + offset);
		char *entryName = dirEntry->name;
		//strcmp returns 0 when the two str are equal
		if(strcmp(entryName, name) == 0){
			return dirEntry;
		}
		//increment the offset by the size of the previous dirEntry
		offset = offset + dirEntry->rec_len;
	}
	return NULL;
}

int main(int argc, char **argv){
	if(argc != 3) {
        fprintf(stderr, "Usage: ext2_ls <image file name> <absolute path>\n");
        exit(1);
    }
	int fd = open(argv[1], O_RDWR);
	disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	absolutePath = argv[2];
	printPath(absolutePath);
}
