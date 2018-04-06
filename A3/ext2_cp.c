#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include "ext2.h"


unsigned char *disk;

int find_max_iblock(){

}
int main(int argc, char **argv) {
    // Checking for right amount of given input
    if(argc != 4) {
        fprintf(stderr, "\n");
        exit(1);
    }
    // Renaming given arguments
    char *format_disk = argv[1];
    char *source_path = argv[2];
    char *destination_path = argv[3];

    // Initializing the Block_Group and Super_Block
    struct ext2_super_block *super_block = (struct ext2_super_block *)(disk + BLOCK_SIZE);
    struct ext2_group_desc *block_group =  (struct ext2_group_desc *)(disk + BLOCK_SIZE * 2); 

    // Opening the ext2 disk
    int fd = open(format_disk, O_RDWR);
    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
	perror("mmap");
	exit(1);
    }

    // Opening file/directory in OS
    FILE *fp;
    if((fp = fopen(source_path, "r")) == NULL) {
        fprintf(stderr, "Can't open source file %s\n", source_path);
        exit(ENOENT);
    }

    // Parsing the Source Path to find last directory in the path
    char last_seperator_index = last_sep_index(source_path);
    char *source_file = last_file_name(last_seperator_index, source_path);

    // Parsing to find the Parent Path of the Destination Path
    char last_parent_sep_index = last_seperator_index(destination_path);
    char *parent_destination_file = parent_path(last_parent_sep_index, destination_path);

    // Check if inode of parent exist
    unsigned int destination_inode = walkPath(disk, destination_path);
    if(destination_inode != NULL) {
        fprintf(stderr, "Can't find Destination Path on the EXT2 disk %s\n", destination_path);
        exit(ENOENT);
    }

    unsigned int parent_destination_inode = walkPath(disk, parent_destination_file);
    if(parent_destination_inode == NULL) {
        fprintf(stderr, "Can't find Parent of Destination Path on the EXT2 disk %s\n", parent_destination_file);
        exit(ENOENT);
    }

    // Getting Inode Table of the directory
    unsigned int inode_table_block = block_group->bg_inode_table;
    
    // Getting to the destination inode
    struct ext2_inode *dest_inode = (struct ext2_inode *)(disk + (BLOCK_SIZE *inode_table_block) + parent_destination_inode);
    
    // Checking if Source File exist in the Directory
    int source_name_len = strlen(source_file);
    int found = 0;
    int *found_inode = 0; 
    int offset = 0;
    while(offset < BLOCK_SIZE){
        struct ext2_dir_entry_2 *dirEntry = (struct ext2_dir_entry_2 *)(disk + (BLOCK_SIZE *inode_table_block) + parent_destination_inode);
        offset = offset + dirEntry->rec_len;
        if((dirEntry->name_len == source_name_len) && strcmp(dirEntry->name, source_file)){
            found = 1;
            found_inode = dirEntry->inode;
            break;
        }
        count += 1;
    }
    
    // Destination Location has a file with same name
    if(found == 1){
        // Overwrite the content of the file on the Disk
        // Getting to the found_inode on the Inode Table
        struct ext2_inode *target_inode = (struct ext2_inode *)(disk + (BLOCK_SIZE *inode_table_block) + found_inode);
        // Destination Locaton File is a Directory
        int byte_read = 0;
        char buf[EXT2_BLOCK_SIZE];

        // Opening Source File to read
        int iblock_ptr_idx = 0;
        while((iblock_ptr_idx < 12) && (byte_got = fread(buf, 1, EXT2_BLOCK_SIZE, fp)) > 0){
            void *block_bitmap = (void *)(disk + blocks_bitmap_block * BLOCK_SIZE);
            int free_block = find_free_block(block_bitmap);
            new_inode -> iblock[iblock_ptr_idx] = free_block;
            char *block = disk + (free_block * EXT2_BLOCK_SIZE);
            memcpy(block, buf, byte_got);
            new_inode -> i_size += byte_got;
            iblock_ptr_idx++;
        }
        // Big File, require indirect allocation
        if((byte_got = fread(buf, 1, EXT2_BLOCK_SIZE, fp)) > 0){
            do{
                int indirect_iblock_ptr_idx = 0;
                *block_bitmap = (void *)(disk + blocks_bitmap_block * BLOCK_SIZE);
                int free_indirect_block = find_free_block(block_bitmap);
                new_inode -> iblock[iblock_ptr_idx][indirect_iblock_ptr_idx] = free_indirect_block;
                char *block = disk + (free_indirect_block * EXT2_BLOCK_SIZE);
                indirect_iblock_ptr_idx ++;
                }while((byte_got = fread(buf, 1, EXT2_BLOCK_SIZE, read_source_file) > 0) && (/*maximum pointer for 2 indirect pointer*/)); //!!!
            }
        else{
            // Signalling there is no more blocks of information after this iblock
            if(iblock_ptr_idx < 11){
                new_inode -> iblock[iblock_ptr_idx] = 0;
                }
            else{
                // The file does not need single indirect block allocation
                new_inode -> iblock[12][0] = 0;
                }
            // Finish copying a small file
            fclose(fp);
            return 0;
        }
    
        // The file required single indirect block allocation
        if(indirect_iblock_ptr_idx < 11){
            new_inode -> iblock[12][indirect_iblock_ptr_idx] = 0;
        }
        else{
            new_inode -> iblock[13][0][0] = 0;
        }
    }
    
    // Creating new file on the Disk
    
    // Name len must not exceed EXT2_NAME_LEN
    if(source_name_len > EXT2_NAME_LEN){
    fprintf(stderr, "Length of the Source File Name exceed given length %s\n", parent_destination_file);
    exit(1);
    }

    // Finding free blocks
    unsigned int blocks_bitmap_block = block_group->bg_block_bitmap;
    void *block_bitmap = (void *)(disk + blocks_bitmap_block * BLOCK_SIZE);
    int free_block_num = find_free_block(block_bitmap);
    unsigned int inodes_bitmap_block = block_group->bg_inode_bitmap;
    void *inode_bitmap = (void *)(disk + inodes_bitmap_block * BLOCK_SIZE);
    int free_inode_num = find_free_inode(inode_bitmap);
    block_group->bg_free_blocks_count = block_group->bg_free_blocks_count -= 1; 
    block_group->bg_free_inodes_count = block_group->bg_free_inodes_count -= 1;

    // Creating new Inode for Source File
    struct ext2_inode *new_inode = (struct ext2_inode *)(disk + (BLOCK_SIZE * inode_table_block) + free_inode_num);
    new_inode -> i_mode = EXT2_S_IFREG;
    new_inode -> i_size = INODE_SIZE;
    new_inode -> i_links_count = 1;
    new_inode -> i_dtime = NULL;
    sb -> s_inodes_count += 1;
    sb -> s_blocks_count += 1;

    // Creating New Directory for Source File
    struct ext2_dir_entry_2 *new_dir_entry = (struct ext2_dir_entry_2 *)(disk + (BLOCK_SIZE * (new_inode -> i_block[0])));
    new_dir_entry -> inode = free_inode_num;
    new_dir_entry -> rec_len = 12;
    new_dir_entry -> name_len = source_name_len;
    new_dir_entry -> file_type = EXT2_FT_REG_FILE;
    strcpy(new_dir_entry->name, source_path);

    // Copying Source File over to newly created Inode || NEED TO ADD
    int byte_read = 0;
    char buf[EXT2_BLOCK_SIZE];

    // Opening Source File to read
    int iblock_ptr_idx = 0;
    while((iblock_ptr_idx < 12) && (byte_got = fread(buf, 1, EXT2_BLOCK_SIZE, fp)) > 0){
        void *block_bitmap = (void *)(disk + blocks_bitmap_block * BLOCK_SIZE);
        int free_block = find_free_block(block_bitmap);
        new_inode -> iblock[iblock_ptr_idx] = free_block;
        char *block = disk + (free_block * EXT2_BLOCK_SIZE);
        memcpy(block, buf, byte_got);
        new_inode -> i_size += byte_got;
        iblock_ptr_idx++;
    }
    // Big File, require indirect allocation
    if((byte_got = fread(buf, 1, EXT2_BLOCK_SIZE, fp)) > 0){
        do{
            int indirect_iblock_ptr_idx = 0;
            *block_bitmap = (void *)(disk + blocks_bitmap_block * BLOCK_SIZE);
            int free_indirect_block = find_free_block(block_bitmap);
            new_inode -> iblock[iblock_ptr_idx][indirect_iblock_ptr_idx] = free_indirect_block;
            char *block = disk + (free_indirect_block * EXT2_BLOCK_SIZE);
            indirect_iblock_ptr_idx ++;
            }while((byte_got = fread(buf, 1, EXT2_BLOCK_SIZE, read_source_file) > 0) && (/*maximum pointer for 2 indirect pointer*/)); //!!!
        }
    else{
        // Signalling there is no more blocks of information after this iblock
        if(iblock_ptr_idx < 11){
            new_inode -> iblock[iblock_ptr_idx] = 0;
            }
        else{
            // The file does not need single indirect block allocation
            new_inode -> iblock[12][0] = 0;
            }
        // Finish copying a small file
        fclose(fp);
        return 0;
        }
    
    // The file required single indirect block allocation
    if(indirect_iblock_ptr_idx < 11){
        new_inode -> iblock[12][indirect_iblock_ptr_idx] = 0;
    }
    else{
        new_inode -> iblock[13][0][0] = 0;
    }
    fclose(fp);
    return 0;
}