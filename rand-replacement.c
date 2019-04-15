#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* Intructions:
    -1. Page fault occurs and this handler is called
    -2. Randomly choose a free frame in the Virtual Memory array
    -3. Adjust Page Table to now map to this frame
    -4. Load the page from disk into page 3 of physical memory
*/

int diskReads = 0;

void rand_replacement_handler(struct page_table *pt, int page) {

	int frame, bits;
	page_table_get_entry(pt, page, &frame, &bits); // get current frame and bits

	int frInd; // track the index of the frame

	if(!bits) {
		bits = PROT_READ;

		//check for a free frame
		frInd = check_free_frame();


    // if no free index, we conduct this random handler
		if(find < 0)
		{
			//randomly select a frame and remove that page
			frInd = (int) lrand48() % nframes;
			remove_page(pt, frIndex);
		}

		disk_read(disk, page, &physmem[frInd*PAGE_SIZE]);
		diskReads++;
	} else if(bits & PROT_READ) {
    // if bits are prot_read make them dirty
		// give write priveledges and set frInd to the new frame
		bits = PROT_READ | PROT_WRITE;
		frInd = frame;
	} else {
		printf("Random page fault failed\n");
		exit(1);
	}

	// update page table and frame table
	page_table_set_entry(pt, page, frInd, bits);

	frameTable[frInd].page = page;
	frameTable[frInd].bits = bits;
}

void page_fault_handler( struct page_table *pt, int page )
{
  page_table_set_entry(pt,page,page,PROT_READ|PROT_WRITE);

  // call the random algorithm
  rand_replacement_handler(pt, page);

	// printf("page fault on page #%d\n",page);
	// exit(1);
}

//remove a page
void remove_page(struct page_table *pt, int frameNumber)
{
	//if there is a dirty bit then write back to disk
	if(frameTable[frameNumber].bits & PROT_WRITE)
	{
		disk_write(disk, frameTable[frameNumber].page, &physmem[frameNumber*PAGE_SIZE]);
		diskWrites++;
	}
	//set the entry to not be writen; update the frame table to represent that and update the front "pointer"
	page_table_set_entry(pt, frameTable[frameNumber].page, frameNumber, 0);
	frameTable[frameNumber].bits = 0;
	front=(front+1)%nframes;

}

int main( int argc, char *argv[] )
{
  if(argc!=5) {
		printf("use: virtmem <npages> <nframes> <rand|fifo|custom> <alpha|beta|gamma|delta>\n");
		return 1;
	}

	int npages = atoi(argv[1]);
	int nframes = atoi(argv[2]);
	char *algorithm = argv[3];
	const char *program = argv[4];

	struct disk *disk = disk_open("myvirtualdisk",npages);
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

	char *physmem = page_table_get_physmem(pt);

	if(!strcmp(program,"alpha")) {
		alpha_program(virtmem, npages*PAGE_SIZE);

	} else if(!strcmp(program,"beta")) {
		beta_program(virtmem, npages*PAGE_SIZE);

	} else if(!strcmp(program,"gamma")) {
		gamma_program(virtmem, npages*PAGE_SIZE);

	} else if(!strcmp(program,"delta")) {
		delta_program(virtmem, npages*PAGE_SIZE);

	} else {
		fprintf(stderr,"unknown program: %s\n",argv[3]);
		return 1;
	}

	page_table_delete(pt);
	disk_close(disk);

	return 0;
}
