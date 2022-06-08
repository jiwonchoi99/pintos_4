#include "vm/page.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "threads/interrupt.h"
#include "filesys/file.h"
#include "userprog/pagedir.h"
#include "threads/palloc.h"
#include <string.h>
#include "vm/swap.h"
#include <hash.h>
#include <debug.h>
#include <stdio.h>

static unsigned hash_function (const struct hash_elem *, void *);
static bool less_function (const struct hash_elem *, const struct hash_elem *, void *);

extern struct lock filesys_lock;

void vm_init (struct hash *vm){
	hash_init (vm, hash_function, less_function, NULL); 
}

void vm_destroy (struct hash *vm){

	hash_destroy(vm, vm_destructor);

}

void vm_destructor (struct hash_elem *e, void *aux){

	struct vm_entry *vme = hash_entry (e, struct vm_entry, elem_hash);
	free(vme);
}


static unsigned hash_function (const struct hash_elem *e, void *aux){
	struct vm_entry *vme;
	vme = hash_entry (e, struct vm_entry, elem_hash);
	unsigned hash_val = hash_int(vme->virtual_addr);
	return hash_val;
}

static bool less_function (const struct hash_elem *e_a, const struct hash_elem *e_b, void *aux){
	struct vm_entry *vme_a;
	struct vm_entry *vme_b;
	
	vme_a = hash_entry (e_a, struct vm_entry, elem_hash);
	vme_b = hash_entry (e_b, struct vm_entry, elem_hash);

	int vaddr_a = vme_a->virtual_addr;
	int vaddr_b = vme_b->virtual_addr;

	if (vaddr_a < vaddr_b)
		return true;
	else
		return false;
}

bool vme_insertion (struct hash *vm, struct vm_entry *vme){
	
	struct hash_elem *e;
	e = &vme->elem_hash;	
	struct hash_elem *old = hash_insert(vm, e);
	return (old == NULL);
	
}
 
bool vme_deletion (struct hash *vm, struct vm_entry *vme){
	struct hash_elem *e;
	e = &vme->elem_hash;

	struct hash_elem *found = hash_delete(vm, e);
	return (found != NULL);
}

struct vm_entry *vme_search (void *virtual_addr){

	struct thread *cur = thread_current();
	struct hash *vm = &cur->vm;
	struct vm_entry vme;

	vme.virtual_addr = pg_round_down(virtual_addr);

	struct hash_elem *e = hash_find (vm, &(vme.elem_hash));   

	if (!e){
		return NULL;
    }

	return hash_entry(e, struct vm_entry, elem_hash);
}


bool load_file (void *kaddr, struct vm_entry *vme){

	size_t read_byte = vme->data_size;
	bool check_lock = lock_held_by_current_thread(&filesys_lock);
	bool is_lock = false;

	if (!check_lock){
		lock_acquire(&filesys_lock);
		is_lock = true;
	}
	
	if (file_read_at(vme->file, kaddr, vme->data_size, vme->offset) != (int) read_byte){

		return false;
	}

	memset(kaddr + read_byte, 0, vme->zero_size);
	
	if(is_lock)
		lock_release(&filesys_lock);
	
	vme ->load_flag = 1;

	return true;
}

struct list lru_list;
struct lock lru_list_lock;
struct list_elem *lru_clock_elem;	



void lru_list_init(void){
	list_init(&lru_list);
	lock_init(&lru_list_lock);
	lru_clock_elem = NULL;
}

void lru_page_insertion(struct page * page){
	list_push_back(&lru_list, &page->lru);
}


void lru_page_deletion(struct page * page){
	struct list_elem *e;
	e = list_remove(&page->lru);
	
	if (&page->lru == lru_clock_elem){
		lru_clock_elem = e;
	}
	
}

struct page* page_allocation(enum palloc_flags flags){
	
	lock_acquire(&lru_list_lock);
	void *kaddr = palloc_get_page(flags);
	while (kaddr == NULL){
		lru_victim_select();
		kaddr = palloc_get_page(flags);
	}
	struct page *p = (struct page *)malloc(sizeof(struct page));
	memset(p, 0, sizeof(struct page));
	p -> thread = thread_current();
	p->kaddr = kaddr;
	p->vme = NULL;
	p->pinned = false;
	lru_page_insertion(p);
	lock_release(&lru_list_lock);
	return p;
}

void kaddr_page_free(void *kaddr){
	struct list_elem *e;
	struct page *p;
	lock_acquire(&lru_list_lock);
	for (e = list_begin(&lru_list); e != list_end(&lru_list); e = list_next(e)){
		p = list_entry(e, struct page, lru);
		if (p->kaddr == kaddr)
			break;
		else
			p = NULL;
	}
	if (p!=NULL)
		lru_page_free(p);
	lock_release(&lru_list_lock);
}

void lru_page_free(struct page * page){
	lru_page_deletion(page);
	pagedir_clear_page(page->thread->pagedir, pg_round_down(page->vme->virtual_addr));
	palloc_free_page(page->kaddr);
	free(page);
}

struct list_elem * next_lru(void){
	if (list_empty(&lru_list)){
		return NULL;
	}
	if (lru_clock_elem == NULL || lru_clock_elem==list_end(&lru_list))
		return list_begin(&lru_list);
	if (list_next(lru_clock_elem) == list_end(&lru_list))
		return list_begin(&lru_list);
	return list_next(lru_clock_elem);
}

void lru_victim_select(void){
	
	lru_clock_elem = next_lru();
	struct page * p = list_entry(lru_clock_elem, struct page, lru);

	while(true){
		if(p->vme->virtual_addr <= PHYS_BASE){
			if (!pagedir_is_accessed(p->thread->pagedir, p->vme->virtual_addr)){
				break;	
			}	
			pagedir_set_accessed(p->thread->pagedir, p->vme->virtual_addr, 0);
		}
		
		lru_clock_elem = next_lru();
		p = list_entry(lru_clock_elem, struct page, lru);
	}

	switch (p->vme->page_type){
		case VM_BIN:
			if(pagedir_is_dirty(p->thread->pagedir, p->vme->virtual_addr)){
				p->vme->swap_slot = swap_out(p->kaddr);
				p->vme->page_type = VM_ANON;
			}
			break;

		case VM_FILE:
			if(pagedir_is_dirty(p->thread->pagedir, p->vme->virtual_addr))
				file_write_at(p->vme->file, p->vme->virtual_addr, p->vme->data_size, p->vme->offset);
			break;
		case VM_ANON:
			p->vme->swap_slot = swap_out(p->kaddr);
			break;
	}
	p->vme->load_flag = 0;

	lru_page_free(p);

}


struct page *page_search(void *kaddr){
	struct list_elem *e;
	struct page *p;

	for (e = list_begin(&lru_list); e != list_end(&lru_list); e = list_next(e)){
		p = list_entry(e, struct page, lru);
		if (p->kaddr == kaddr)
			return p;
	}
}
