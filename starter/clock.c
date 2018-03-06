#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

int *clock;
int clock_hand = 0;

/* Page to evict is chosen using the clock algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int clock_evict() {

	int frame = -1;
	while(clock[clock_hand] != 0) {
		clock[clock_hand] = 0;
		clock_hand ++;
		if(clock_hand == memsize) {
			clock_hand = 0;
		}
	}
	frame = clock_hand;
	clock_hand ++;
	return frame;
}

/* This function is called on each access to a page to update any information
 * needed by the clock algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void clock_ref(pgtbl_entry_t *p) {

	int frame_num = p->frame >> PAGE_SHIFT;
	clock[frame_num] = 1;
}

/* Initialize any data structures needed for this replacement
 * algorithm. 
 */
void clock_init() {
	clock = (int *)malloc(sizeof(int) * memsize);
	if(clock == NULL){
		//we have an error
	}
	for (int i = 0; i < memsize; i++) {
		clock[i] = 0;
	}
}
