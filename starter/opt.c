#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

int *page_list;
int *pages;
int index;

/* Page to evict is chosen using the optimal (aka MIN) algorithm. 
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int opt_evict() {
	
	int frame = -1;
	int greatest_index = 0;
	int page_list_size = sizeof(page_list)/sizeof(page_list[0]);
	int i = 0;
	while((pages[i] != -1) && (i != memsize) {
		//first index where pages[i] appears in the future references
		int appearence = -1;
		//try to find pages[i] in the furture references
		for(j = index; j < page_list_size; j++) {
			if (pages[i] == page_list[j]) {
				appearence = j;
				//break out of loop
				j == page_list_size;
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
	pages[frame_num] = page_list[index];
	index = index ++;
	return;
}

/* Initializes any data structures needed for this
 * replacement algorithm.
 */
void opt_init() {

}

