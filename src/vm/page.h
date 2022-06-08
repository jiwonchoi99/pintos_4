#ifndef VM_PAGE_H
#define VM_PAGE_H

#define VM_BIN	0
#define VM_FILE	1
#define VM_ANON	2 

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include <hash.h>
#include "filesys/file.h"
#include "threads/palloc.h"


struct vm_entry{
	
	struct hash_elem elem_hash;
	struct file *file;	

	uint8_t page_type;
	void *virtual_addr;
	
	bool can_write;
	bool load_flag;
	
	off_t offset;
	uint32_t data_size;
	uint32_t zero_size; 

	//Memory Mapped File
	struct list_elem elem_mmap;
	
	//Swapping
	size_t swap_slot;
};

struct page{
	void *kaddr;
	struct thread *thread;
	struct vm_entry *vme;
	struct list_elem lru;
	bool pinned;
};


struct mmap_file{
	int mapid;
	struct file * file;
	struct list_elem elem;
	struct list vme_list;
};


void vm_init (struct hash *vm);
void vm_destroy (struct hash *vm);
void vm_destructor (struct hash_elem *e, void *aux);
bool vme_insertion (struct hash *vm, struct vm_entry *vme);
bool vme_deletion (struct hash *vm, struct vm_entry *vme);
struct vm_entry *vme_search (void *virtual_addr);
bool load_file (void *kaddr, struct vm_entry *vme);


void lru_list_init(void);
void lru_page_insertion(struct page *page);
void lru_page_deletion(struct page * page);
struct page * page_allocation(enum palloc_flags flags);
void kaddr_page_free(void *kaddr);
void lru_page_free(struct page * page);
struct list_elem * next_lru(void);
void lru_victim_select(void);
struct page *page_search(void *);

#endif
