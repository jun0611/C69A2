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

int removeInode(struct ext2_inode *inode) {
	inode->i_dtime = get_seconds();
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
	
	
}
