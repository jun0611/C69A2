#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

extern char* tracefile;

#define buff_size 256
int *page_list;
int *pages;
int page_list_size = 0;
int page_list_index = 0;

/* Page to evict is chosen using the optimal (aka MIN) algorithm. 
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int opt_evict() {
	
	int frame = -1;
	int greatest_index = 0;
	int i = 0;
	while((pages[i] != -1) && (i != memsize)) {
		//first index where pages[i] appears in the future references
		int appearence = -1;
		//try to find pages[i] in the furture references
		for(int j = page_list_index; j < page_list_size; j++) {
			if (pages[i] == page_list[j]) {
				appearence = j;
				//break out of loop
				j = page_list_size;
			}
		}
		//if pages[i] is no longer referenced
		if(appearence == -1) {
			return i;
		}
		//if pages[i] currently is closest to never being referenced
		if(appearence > greatest_index) {
			frame = i;
			greatest_index = appearence;
		}
		i ++;
	}
	return frame;
}

/* This function is called on each access to a page to update any information
 * needed by the opt algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void opt_ref(pgtbl_entry_t *p) {

	int frame_num = p->frame >> PAGE_SHIFT;
	pages[frame_num] = page_list[page_list_index];
	page_list_index += 1;
	return;
}

/* Initializes any data structures needed for this
 * replacement algorithm.
 */
void opt_init() {

	pages = malloc(memsize * sizeof(int));
	for (int i = 0; i < memsize; i++) {
		pages[i] = -1;
	}

	FILE *fp;
	fp = fopen(tracefile, "r");

	//read tracefile to get length of page_list
	if (fp == NULL) {
        printf ("Error opening the file\n\n'");
        exit(1);
    } else {
    	// initialize the size of page_list
    	char buff[buff_size];
    	while(fgets(buff, buff_size, fp) != NULL) {
    		page_list_size ++;
    	}
    	fclose(fp);
    }

    page_list = malloc(page_list_size * sizeof(int));

    //open tracefile for reading again to fill page_list
    fp = fopen(tracefile, "r");
	if (fp == NULL) {
        printf ("Error opening the file\n\n'");
        exit(1);
    } else {
    	char type;
    	int v_addr;
    	int count = 0;
    	char buff[buff_size];
    	while(fgets(buff, buff_size, fp) != NULL) {
    		fscanf(fp, "%c %x", &type, &v_addr);
    		page_list[count] = v_addr;
    		count ++;
    	}
    	fclose(fp);
    }
}

