#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2.h"

unsigned char *disk;

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
    for(int i = 0; i < (index + 1); i++){
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
    for(int i = index + 1; i < path_len; i++) {
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
int find_free_block(void *block_bitmap) {
    for (i = 0; i < BLOCK_BITMAP_BYTES; i++) {
        int j;
        unsigned int b = block_bitmap[i];
        for(j = 0; j < 8; j++) {
            if(!(b >> j & 1)) {
                //i byte, j bits is the free block
                return (i * 8 + j + 1)
            }
        }
    }
    return -1;
}

/*
* find free inode using block bitmap
*/
int find_free_block(void *inode_bitmap) {
    for (i = 0; i < INODE_BITMAP_BYTES; i++) {
        int j;
        unsigned int b = inode_bitmap[i];
        for(j = 0; j < 8; j++) {
            if(!(b >> j & 1)) {
                //i byte, j bits is the free inode
                return (i * 8 + j + 1)
            }
        }
    }
    return -1;
}