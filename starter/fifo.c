#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

//Maintains a list of frame numbers of the pages that have been referenced
int queue[memsize];

//Number of times we have evicted. 
int numberOfEvicts = 0;

//Number of times we have inserted into the queue;
int numberOfInserts = 0;

/* Page to evict is chosen using the fifo algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int fifo_evict() {
	int frameToEvict = queue[numberOfEvicts];
	numberOfEvicts += 1;
	if (numberOfEvicts == memsize){
		numberOfEvicts = 0;
	}
	return frameToEvict;
}

/* This function is called on each access to a page to update any information
 * needed by the fifo algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void fifo_ref(pgtbl_entry_t *p) {
	if ((p->frame & PG_VALID) == 0) {
		//This means the page is not in our queue, so we add it. 
		queue[numberOfInserts] = (p->frame >> PAGE_SHIFT);
		numberOfInserts += 1;
		if (numberOfInserts == memsize){
			numberOfInserts = 0;
		}
	}
	return;
}

/* Initialize any data structures needed for this 
 * replacement algorithm 
 */
void fifo_init() {
}
