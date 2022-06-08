#include "filesys/buffer_cache.h"
#include "threads/palloc.h"
#include <string.h>
#include <debug.h>
#include <stdio.h>

#define BUFFER_CACHE_ENTRY_NB 64

//static char p_buffer_cache[BUFFER_CACHE_ENTRY_NB*BLOCK_SECTOR_SIZE];
void *p_buffer_cache;

static struct buffer_head buffer_head_table[BUFFER_CACHE_ENTRY_NB];
static struct buffer_head *clock_hand;


bool bc_read(block_sector_t sector_idx, void *buffer, off_t bytes_read, int chunk_size, int sector_ofs)
{
    struct buffer_head *b_head = bc_lookup (sector_idx);
	
    if (b_head == NULL){

        b_head = buffer_head_table;
	    int i;
        
        for (i=0; i<BUFFER_CACHE_ENTRY_NB; i=i+1){
            if (b_head->usage==0){
                lock_acquire(&b_head->bc_lock);
                clock_hand = b_head;
                clock_hand++;
                break;
            }
            else
                b_head++;
        }       
        
        if (i==BUFFER_CACHE_ENTRY_NB){
            b_head = bc_select_victim ();
        }

        //b_head = bc_select_victim();
        //if (b_head->dirty){
        //    bc_flush_entry(b_head);
        //}
        b_head -> usage = true;
        b_head -> dirty = false;
        b_head -> sector = sector_idx;
    
        block_read(fs_device, sector_idx, b_head->data);    
    }

    memcpy(buffer+bytes_read, b_head->data+sector_ofs, chunk_size);
    
    lock_release (&b_head -> bc_lock);
    b_head->clock = true;
    return true;
}

bool bc_write (block_sector_t sector_idx, void *buffer, off_t bytes_written, int chunk_size, int sector_ofs){

    struct buffer_head *b_head = bc_lookup (sector_idx);
    if (b_head == NULL){



        b_head = buffer_head_table;
	    int i;
        for (i=0; i<BUFFER_CACHE_ENTRY_NB; i=i+1){
            if (b_head->usage==0){
                lock_acquire(&b_head->bc_lock);
                clock_hand = b_head;
                clock_hand++;
                break;
            }
            else
                b_head++;
        }

        
        if (i==BUFFER_CACHE_ENTRY_NB){
            b_head = bc_select_victim ();
        }

        //b_head = bc_select_victim();
        //if (b_head->dirty){
        //    bc_flush_entry(b_head);
        //}
        b_head -> usage = true;
        b_head -> dirty = false;
        b_head -> sector = sector_idx;

        block_read(fs_device, sector_idx, b_head->data);    
    }
    b_head -> clock = true;
    b_head -> dirty = true;
    memcpy(b_head->data + sector_ofs, buffer+bytes_written, chunk_size);
    lock_release (&b_head -> bc_lock);

    return true;
}


void bc_flush_entry (struct buffer_head *p_flush_entry){
    block_write(fs_device, p_flush_entry->sector, p_flush_entry->data);
    p_flush_entry -> dirty = 0;
}

void bc_init (void){

    struct buffer_head *b_head;
    //void *buffer_cache = p_buffer_cache;

    p_buffer_cache = malloc(64*BLOCK_SECTOR_SIZE);
    memset(p_buffer_cache, 0, 64*BLOCK_SECTOR_SIZE);

    int i;
    b_head = buffer_head_table;
    for (i=0; i<BUFFER_CACHE_ENTRY_NB; i=i+1){
        
        b_head -> data = p_buffer_cache + i*BLOCK_SECTOR_SIZE;
        b_head -> sector = -1;
        //memset (b_head, 0, sizeof(struct buffer_head));
        lock_init (&b_head->bc_lock);
        //b_head -> data = buffer_cache;

        b_head ++;
        //buffer_cache += BLOCK_SECTOR_SIZE;
    }
    clock_hand = buffer_head_table;
}

void bc_term (void){
    bc_flush_all_entries(); 
    free(p_buffer_cache);
}

void bc_flush_all_entries (void){
    struct buffer_head *b_head;
    int i;
    b_head = buffer_head_table;
    for (i=0; i<BUFFER_CACHE_ENTRY_NB; i=i+1){
        
        //if (b_head->dirty && b_head->usage){
        if (b_head->dirty){
            lock_acquire(&b_head->bc_lock);
            bc_flush_entry(b_head);
            lock_release(&b_head->bc_lock);
        }
        b_head++;
    } 
}

struct buffer_head *bc_select_victim (void){

    int i;

    for (i=0; i<5*BUFFER_CACHE_ENTRY_NB; i=i+1){

        if (clock_hand == buffer_head_table + BUFFER_CACHE_ENTRY_NB){
            clock_hand = buffer_head_table;
        }
        lock_acquire(&clock_hand->bc_lock);

        if (clock_hand->clock == 0){

            if (clock_hand->dirty){
                bc_flush_entry(clock_hand);
            }
            
            return clock_hand++;
        }
        clock_hand ->clock = false;
        lock_release(&clock_hand->bc_lock);
        clock_hand++;
    }
}

struct buffer_head *bc_lookup (block_sector_t sector){

    struct buffer_head *b_head;
    int i;
    b_head = buffer_head_table;
    for (i=0; i<BUFFER_CACHE_ENTRY_NB; i=i+1){
        
        if (b_head->sector == sector){
            lock_acquire(&b_head->bc_lock);

            return b_head;
        }
        b_head ++;
    }  
    return NULL;
}
