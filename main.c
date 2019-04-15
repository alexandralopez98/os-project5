/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#define MAX 100

struct frameTracker {
	int page;
	int availability;
};

char *algorithm;
int nframes;
struct frameTracker frameTable[MAX]; 
struct disk *disk;
char *physmem;
int page_faults = 0;
int disk_reads = 0;
int disk_writes = 0;

/* FIFO array variables */
int fifoArray[MAX];
int size = 0; // keeps track of how many items have been pushed to the FIFO array
int head = -1;
int tail = -1;

void printQueue() {
	int i;
	printf("FIFO QUEUE\n");
	for (i = 0; i < nframes; i++){
		printf("%d ", fifoArray[i]);
	}
	printf("\n");
}

// Push Frame Table index to FIFO queue
void enqueue(int index) {
	if (size < nframes) {
		fifoArray[size++] = index;
	}
}

// Pop Frame Table index from FIFO queue
int dequeue(){
	int index = fifoArray[0];
	int i;
	for (i = 0; i < nframes-1; i++) {
		fifoArray[i] = fifoArray[i+1];
	}
	fifoArray[nframes-1] = 0;
	size--;
	return index;
}

int findFreeFrame() {
	int i;
	for (i = 0; i < nframes; i++) {
		
		// Frame is open
		if (frameTable[i].availability == 0) {
			frameTable[i].availability = 1;
			return i;
		} 
	}
	
	// Frame table is full
	return -1;
}

int fifo_replacement() {
	// update FIFO queue
	int index = dequeue();
	enqueue(index);

	return index;	
}

int rand_replacement() {
	int index = rand() % nframes;
	return index;
}

int custom_replacement(struct page_table *pt) {

	int index = rand_replacement();

	int i;
	int bits, frame;
	int page = frameTable[index].page;
	page_table_get_entry(pt, page, &frame, &bits);
	for (i = 0; i < 100; i++) {
		if (bits & PROT_WRITE) {
			index = rand_replacement();
			page = frameTable[index].page;
			page_table_get_entry(pt, page, &frame, &bits);
		}
	}

	return index;



}

void page_fault_handler( struct page_table *pt, int page )
{
	page_faults++;
	int frame;
	int bits;

	page_table_get_entry(pt, page, &frame, &bits);

	// The page has no permissions, and thus is not in physical memory
	if (bits == 0) {
		int free_frame = findFreeFrame(); // frame where new data will be stored

		// Frame Table is full; replacement policy must be called
		if (free_frame == -1) {
			if (strcmp(algorithm, "fifo") == 0) {
				free_frame = fifo_replacement();
			}
			else if (strcmp(algorithm, "rand") == 0) {
				free_frame = rand_replacement();
			}
			else if (strcmp(algorithm, "custom") == 0) {
				free_frame = custom_replacement(pt);
			}
			else {
				printf("main: Invalid algorithm!\n");
				exit(1);
			}

			int page_to_be_removed = frameTable[free_frame].page;

			int frame_read;
			int bits_read;

			// Get page table entry of page that will be removed
			page_table_get_entry(pt, page_to_be_removed, &frame_read, &bits_read);

			// Data at index is modified and must be written back to disk
			if (bits_read & PROT_WRITE) {
				disk_write(disk, page_to_be_removed, &physmem[frame_read*PAGE_SIZE]);
				disk_writes++;
			}

			// Update page table for page we removed
			page_table_set_entry(pt, page_to_be_removed, 0, 0);
		}
		// Frame Table is not full; add frame to FIFO array
		else {
			enqueue(free_frame);
			// printQueue();
		}
		
		// Read in new data and update its entry in the page table
		disk_read(disk, page, &physmem[free_frame*PAGE_SIZE]);
		disk_reads++;
		page_table_set_entry(pt, page, free_frame, PROT_READ);
		frameTable[free_frame].page = page;

	}
	// The page has PROT_READ, so we add the write bit
	else {
		page_table_set_entry(pt, page, frame, PROT_READ|PROT_WRITE);
	}

	return;
}

int main( int argc, char *argv[] )
{
	if(argc!=5) {
		printf("use: virtmem <npages> <nframes> <rand|fifo|custom> <alpha|beta|gamma|delta>\n");
		return 1;
	}

	int npages = atoi(argv[1]);
	nframes = atoi(argv[2]);
	algorithm = argv[3];
	const char *program = argv[4];

	disk = disk_open("myvirtualdisk",npages);
	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}


	struct page_table *pt = page_table_create( npages, nframes, page_fault_handler );
	if(!pt) {
		fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
		return 1;
	}

	char *virtmem = page_table_get_virtmem(pt);

	physmem = page_table_get_physmem(pt);

	if(!strcmp(program,"alpha")) {
		alpha_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"beta")) {
		beta_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"gamma")) {
		gamma_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"delta")) {
		delta_program(virtmem,npages*PAGE_SIZE);

	} else {
		fprintf(stderr,"unknown program: %s\n",argv[3]);
		return 1;
	}

	page_table_delete(pt);
	disk_close(disk);

	printf("Page faults: %d\n", page_faults);
	printf("Disk reads: %d\n", disk_reads);
	printf("Disk writes: %d\n", disk_writes);

	return 0;
}
