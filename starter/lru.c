#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

int *frameStack;

int numberOfFrames = 0;

/* Page to evict is chosen using the accurate LRU algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int lru_evict() {
	int frameToEvict = frameStack[0];
	//shift elements in list one position to the left
	for (int i = 0; i < memsize - 1; i++) {
		frameStack[i] = frameStack[i+1]; 
	}
	numberOfFrames -= 1;
	return frameToEvict;
}


/* This function is called on each access to a page to update any information
 * needed by the lru algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void lru_ref(pgtbl_entry_t *p) {
	int frameNumber = p->frame >> PAGE_SHIFT;
	int frameInStack = -1;
	//check if this frame number is already in the stack
	for(int i = 0; i < memsize; i++){
		if(frameStack[i] == frameNumber){
			frameInStack = i;
			break;
		}
	}

	if(frameInStack == -1){
		//Frame is not in stack, so we add it
		frameStack[numberOfFrames] = frameNumber;
		numberOfFrames += 1;
		if (numberOfFrames == memsize){
			numberOfFrames = 0;
		}
	}

	else {
		//We know that the stack contains frameNumber at index frameInStack
		//We want to move that frame to the top of the stack, and shift everything over to the left
		for(int i = frameInStack; i < memsize - 1; i++){
			frameStack[i] = frameStack[i+1];
		}
		frameStack[memsize-1] = frameNumber;
	}

	return;
}


/* Initialize any data structures needed for this 
 * replacement algorithm 
 */
void lru_init() {
	frameStack = (int *)malloc(sizeof(int) * memsize);
	if(frameStack == NULL) {
		//We have an error
	}
}
