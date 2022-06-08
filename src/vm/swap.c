#include "vm/swap.h"
#include "devices/block.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include <bitmap.h>


static struct lock swap_lock;
static struct block *swap;
static struct bitmap *swap_bitmap;

void swap_init(){
	swap = block_get_role(BLOCK_SWAP);
	swap_bitmap = bitmap_create ((block_size(swap)*BLOCK_SECTOR_SIZE)/PGSIZE);	
	
	lock_init(&swap_lock);
}


void swap_in (void *kaddr, size_t swap_slot){

	swap = block_get_role(BLOCK_SWAP);
	lock_acquire(&swap_lock);
	
	if (!bitmap_test(swap_bitmap, swap_slot)){
		lock_release(&swap_lock);
		exit(-1);	
	}

	else{
		for (int i=0; i<PGSIZE/BLOCK_SECTOR_SIZE; i=i+1){		
			block_read(swap, (PGSIZE/BLOCK_SECTOR_SIZE)*swap_slot + i, kaddr);
			kaddr += BLOCK_SECTOR_SIZE;
		
		}
		bitmap_reset(swap_bitmap, swap_slot);
		lock_release(&swap_lock);
	}
	
}

size_t swap_out (void *kaddr){

	size_t swap_slot;
	swap = block_get_role(BLOCK_SWAP);
	lock_acquire (&swap_lock);
	swap_slot = bitmap_scan_and_flip(swap_bitmap, 0, 1, false);
	if (swap_slot == BITMAP_ERROR){
		lock_release(&swap_lock);
		exit(-1);
	}
	else{
		for (int i=0; i<PGSIZE/BLOCK_SECTOR_SIZE; i=i+1){
			block_write(swap, (PGSIZE/BLOCK_SECTOR_SIZE)*swap_slot+i, kaddr);
			kaddr += BLOCK_SECTOR_SIZE;
		}

		bitmap_set (swap_bitmap, swap_slot, true);
		lock_release(&swap_lock);
	}
	
	return swap_slot;

}
