#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "helper_functions.h"

unsigned char *disk;
char *absolutePath;

void removeDirEntry(struct ext2_inode *inode, int blockNum, char *dirEntryName) {
	struct ext2_dir_entry_2 *parentEntry;
	struct ext2_dir_entry_2 *dirEntry = (struct ext2_dir_entry_2 *)(disk + (BLOCK_SIZE * blockNum));
	int offset = dirEntry->rec_len;
	
	//if the first entry is the entry we want, then we have to create a black directory entry
	char entryName[dirEntry->name_len + 1];
	memcpy(entryName, &dirEntry->name, dirEntry->name_len);
	entryName[dirEntry->name_len] = '\0';
	if(strcmp(entryName, dirEntryName) == 0) {
		//make the dirEntry blank
		int recLen = dirEntry->rec_len;
		memset(dirEntry, 0, dirEntry->rec_len);
		struct ext2_dir_entry_2 *blankEntry = (struct ext2_dir_entry_2 *)(disk + (BLOCK_SIZE * blockNum));
		blankEntry->rec_len = recLen;
	}
	else {
		while(offset < BLOCK_SIZE) {
			parentEntry = dirEntry;
			dirEntry = (struct ext2_dir_entry_2 *)(disk + (BLOCK_SIZE * blockNum) + offset);
			char name[dirEntry->name_len + 1];
			memcpy(name, &dirEntry->name, dirEntry->name_len);
			name[dirEntry->name_len] = '\0';
			//strcmp returns 0 when the two str are equal
			if(strcmp(name, dirEntryName) == 0){
				parentEntry->rec_len = parentEntry->rec_len + dirEntry->rec_len;
			}
			//increment the offset by the size of the previous dirEntry
			offset = offset + dirEntry->rec_len;
		}
	}
}

void iterateDirectoryEntries(struct ext2_inode *inode, char *dirEntryName){
	int i;
	for(i = 0; i < NUMBER_OF_SINGLE_POINTERS; i++){
		if(inode->i_block[i]){
			removeDirEntry(inode, inode->i_block[i], dirEntryName);
		}
	}
}

void removeDirectoryEntry(char *entryName){
	//get path of parent directory
	int lastSlash = last_sep_index(absolutePath);
	char *parentDir = parent_path(lastSlash, absolutePath);

	//get name of parent directory
	lastSlash = last_sep_index(absolutePath);
	char *parentName = last_file_name(lastSlash, parentDir);

	unsigned int inodeNum = walkPath(disk, parentDir);
	struct ext2_inode *inode = (struct ext2_inode *)(disk + INODE_TABLE_BLOCK * BLOCK_SIZE + INODE_SIZE * (inodeNum - 1));
	iterateDirectoryEntries(inode, entryName);
}

void setBlocksFree(struct ext2_inode *inode) {
	int i;
	unsigned char *blockBitmap = malloc(BLOCK_BITMAP_BYTES);
    blockBitmap = (unsigned char *)(disk + BLOCK_SIZE*BLOCK_BITMAP_BLOCK);
	for (i = 0; i < 12; i++) {
		//free all direct pointers
		if(inode->i_block[i]) {
			set_bitmap(blockBitmap, inode->i_block[i], 0);
		}
	}
	unsigned int indirect = inode->i_block[12];
	if (indirect){
		int *singleIndirectBlock = (int *)(disk + BLOCK_SIZE*indirect);
		for (i = 0; i < NUMBER_OF_INDIRECT_POINTERS; i++) {
			if(singleIndirectBlock[i]) {
				set_bitmap(blockBitmap, singleIndirectBlock[i], 0);
			}
		}
	}
}

int main(int argc, char **argv){	
	if(argc != MIN_ARGUMENTS) {
        fprintf(stderr, "Usage: ext2_rm <image file name> <absolute path>\n");
        exit(1);
    }
	
	int fd = open(argv[FIRST_ARGUMENT], O_RDWR);
	if(fd < 0) {
		//Could not open file
		fprintf(stderr, "Invalid image file\n");
        exit(1);
	}
	disk = mmap(NULL, INODE_SIZE * BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	absolutePath = argv[SECOND_ARGUMENT];
	
	unsigned int inodeNumber = walkPath(disk, absolutePath);
	
	if(inodeNumber == 0){
		//error case
		printf("No such file or directory\n");
		return ENOENT;
	}

	struct ext2_inode *inode = (struct ext2_inode *)(disk + INODE_TABLE_BLOCK * BLOCK_SIZE + INODE_SIZE * (inodeNumber - 1));

	if ((inode->i_mode & EXT2_S_IFDIR) == EXT2_S_IFDIR) {
		//if the final inode is a directory return an error
		printf("can't delete a directory");
		return ENOENT;
	}
	//decrement link count
	inode->i_links_count = inode->i_links_count - 1;
	if(inode->i_links_count <= 0) {
		inode->i_dtime = (unsigned int)time(NULL);
		setBlocksFree(inode);
		//remove directory entry
		int lastSlash = last_sep_index(absolutePath);
		char *fileName = last_file_name(lastSlash, absolutePath);
		removeDirectoryEntry(fileName);
	}
}
