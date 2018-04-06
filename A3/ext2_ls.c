#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include "helper_functions.h"

unsigned char *disk;
char *absolutePath;

void printDirNames(int blockNum, char flag){
	int count = 0;
	int offset = 0;
	while(offset < BLOCK_SIZE){
		struct ext2_dir_entry_2 *dirEntry = 
		(struct ext2_dir_entry_2 *)(disk + BLOCK_SIZE * blockNum + offset);
		offset = offset + dirEntry->rec_len;

		if(flag || count > 1){
			printf("%.*s\n", dirEntry->name_len, dirEntry->name);
		}

		count += 1;
	}	
}

void printDirectoryEntries(struct ext2_inode *inode, char flag){
	int i;
	for(i = 0; i < NUMBER_OF_SINGLE_POINTERS; i++){
		if(inode->i_block[i]){
			printDirNames(inode->i_block[i], flag);	
		}
	}
}

int main(int argc, char **argv){
	char flag = 0; //1 if -a was provided, 0 otherwise
	if((argc == MAX_ARGUMENTS) && (strcmp(argv[SECOND_ARGUMENT], "-a") == 0)) {
		flag = 1;
	}		
	else if(argc != MIN_ARGUMENTS) {
        fprintf(stderr, "Usage: ext2_ls <image file name> <absolute path>\n");
        exit(1);
    }
	
	int fd = open(argv[FIRST_ARGUMENT], O_RDWR);
	if(fd < 0) {
		//Could not open file
		fprintf(stderr, "Invalid image file\n");
        exit(1);
	}
	disk = mmap(NULL, INODE_SIZE * BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
			
	if(flag){
		absolutePath = argv[THIRD_ARGUMENT];	
	}
	else {
		absolutePath = argv[SECOND_ARGUMENT];
	}

	//struct ext2_inode *inode = walkPath(disk, absolutePath);
	unsigned int inodeNumber = walkPath(disk, absolutePath);
	
	if(inodeNumber == 0){
		//error case
		printf("No such file or directory\n");
		return ENOENT;
	}
	struct ext2_inode *inode = (struct ext2_inode *)(disk + INODE_TABLE_BLOCK * BLOCK_SIZE + INODE_SIZE * (inodeNumber - 1));
	//if the final inode is a file or link, just print name of the last element in the path	
	if(((inode->i_mode & EXT2_S_IFREG) == EXT2_S_IFREG) || ((inode->i_mode & EXT2_S_IFLNK) == EXT2_S_IFLNK)) {
		int indexOfLastSeperator = last_sep_index(absolutePath);
		char *lastFile = last_file_name(indexOfLastSeperator, absolutePath);
		printf("%s\n", lastFile);
	}
	else if((inode->i_mode & EXT2_S_IFDIR) == EXT2_S_IFDIR) {
		//if the final inode is a direcory, print it's contents
		printDirectoryEntries(inode, flag);
	}
	
}
