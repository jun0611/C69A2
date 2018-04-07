#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include "ext2.h"
#include "helper_functions.h"
#include <errno.h>

unsigned char *disk;


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
        target_inode -> i_size = INODE_SIZE;
        // Destination Locaton File is a Directory
        int byte_read = 0;
        char buf[BLOCK_SIZE];

        // Opening Source File to read
        int iblock_ptr_idx = 0;
        while((iblock_ptr_idx < 12) && (byte_got = fread(buf, 1, BLOCK_SIZE, fp)) > 0){
            unsigned int found_block;
            found_block = target_inode -> iblock[iblock_ptr_idx];
            char *block = disk + (free_block * (BLOCK_SIZE*inode_table_block) + found_block);
            memcpy(block, buf, byte_got);
            target_inode -> i_size += byte_got;
            iblock_ptr_idx++;
        }
        // Big File, require indirect allocation
        if((byte_got = fread(buf, 1, BLOCK_SIZE, fp)) > 0){
            int indirect_block_ptr = 0;
            unsigned int indirect = inode->i_block[12];
            int *singleIndirectBlock = (int *)(disk + BLOCK_SIZE*indirect);
            do{ 
                int block_num = singleIndirectBlock[indirect_iblock_ptr];
                char *block = disk + (BLOCK_SIZE*block_num);
                memcpy(block, buf, byte_got);
                target_inode -> i_size += byte_got;
                indirect_iblock_ptr++;
                }while((byte_got = fread(buf, 1, BLOCK_SIZE, fp) > 0) && (indirect_iblock_ptr < NUMBER_OF_INDIRECT_POINTERS)); 
            }
        fclose(fp);
        return 0;
    }

    // File was not found on the Disk
    // Name len must not exceed EXT2_NAME_LEN
    if(source_name_len > EXT2_NAME_LEN){
        fprintf(stderr, "Length of the Source File Name exceed given length %s\n", parent_destination_file);
        exit(1);
    }

    // Creating new file on the Disk
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

    // Add the new Directory to the Destination Directory
    int block_count = 0;
    while((parent_destination_inode -> i_block[block_count + 1]) && block_count != 15) {
        block_count ++;
    }
    // find end space in the block
    int total_rec_len = 0
    while(total_rec_len < BLOCK_SIZE) {
        struct ext2_dir_entry_2 *dir_entry = (struct ext2_dir_entry_2 *)(disk +
            (BLOCK_SIZE * (parent_destination_inode -> i_block[block_count])) + total_rec_len);
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
                } 
                else {
                    //use the next block
                    struct ext2_dir_entry_2 *new_entry = (struct ext2_dir_entry_2 *)(disk +
                   (BLOCK_SIZE * (parent_destination_inode -> i_block[block_count + 1])));    
                    new_entry -> inode = free_inode_num;
                    new_entry -> rec_len = 12;
                    new_entry -> name_len = source_name_len;
                    new_entry -> file_type = EXT2_FT_REG_FILE;
                    strcpy(new_dir_entry->name, source_path);
                    break;
                }
                } 
                else {
                    struct ext2_dir_entry_2 *new_entry = (struct ext2_dir_entry_2 *)(disk +
                   (BLOCK_SIZE * (parent_destination_inode -> i_block[block_count]) + total_rec_len + entry_size));
                    new_entry -> inode = free_inode_num;
                    new_entry -> rec_len = 12;
                    new_entry -> name_len = source_name_len;
                    new_entry -> file_type = EXT2_FT_REG_FILE;
                    strcpy(new_dir_entry->name, source_path);
                    // update previous entry rec_len
                    dir_entry -> rec_len = total_rec_len + entry_size;
                    break;
            }
        }
       total_rec_len = total_rec_len + (dir_entry -> rec_len);
    } 

    set_bitmap(inode_bitmap, free_inode_num);
    set_bitmap(block_bitmap, free_block_num);
    // Copying Source File over to newly created Inode
    int byte_read = 0;
    char buf[BLOCK_SIZE];

    // Opening Source File to read
    int iblock_ptr_idx = 0;
    while((iblock_ptr_idx < 12) && (byte_got = fread(buf, 1, BLOCK_SIZE, fp)) > 0){
        void *block_bitmap = (void *)(disk + blocks_bitmap_block * BLOCK_SIZE);
        int free_block = find_free_block(block_bitmap);
        new_inode -> iblock[iblock_ptr_idx] = free_block;
        char *block = disk + (free_block * BLOCK_SIZE);
        memcpy(block, buf, byte_got);
        new_inode -> i_size += byte_got;
        iblock_ptr_idx++;
        set_bitmap(block_bitmap, free_block_num);
    }
    // Big File, require indirect allocation
    if((byte_got = fread(buf, 1, BLOCK_SIZE, fp)) > 0){
        int indirect_iblock_ptr= 0;
        (void*) *block_bitmap = (void *)(disk + blocks_bitmap_block * BLOCK_SIZE);
        new_inode -> iblock[12] = free_indirect_block;
        int *singleIndirectBlock = (int *) (disk + BLOCK_SIZE * free_indirect_block);
        set_bitmap(block_bitmap, free_indirect);
        do{ 
            *block_bitmap = (void *) (disk + blocks_bitmap_block * BLOCK_SIZE);
            int free_indirect_direct_block = find_free_block(block_bitmap);
            singleIndirectBlock[indirect_iblock_ptr] = free_indirect_direct_block;
            memcpy(free_indirect_direct_block, buf, byte_got);
            new_inode -> i_size += byte_got;
            set_bitmap(block_bitmap, free_indirect_direct_block);
            indirect_iblock_ptr++;
            }while((byte_got = fread(buf, 1, BLOCK_SIZE, fp) > 0) && (indirect_iblock_ptr < NUMBER_OF_INDIRECT_POINTERS)); 
        }
    fclose(fp);
    return 0;
}
