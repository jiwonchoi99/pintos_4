
#include <bitmap.h>
#include <debug.h>
#include <string.h>
#include "vm/frame.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "vm/swap.h"
#include "userprog/syscall.h"
/*
static struct list_elem* next_lru(void);

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
	void *kaddr = palloc_get_page(flags);
	while (kaddr == NULL){
		lru_victim_select();
		kaddr = palloc_get_page(flags);
	}
	struct page *p = (struct page *)malloc(sizeof(struct page));
	memset (p, 0, sizeof (struct page));
	p -> thread = thread_current();
	p->kaddr = kaddr;
	lru_page_insertion(p);
	return p;
}

void kaddr_page_free(void *kaddr){
	struct list_elem *e;
	struct page *p;
	for (e = list_begin(&lru_list); e != list_end(&lru_list); e = list_next(e)){
		p = list_entry(e, struct page, lru);
		if (p->kaddr == kaddr)
			break;
		else
			p = NULL;
	}
	if (p!=NULL)
		lru_page_free(p);
}

void lru_page_free(struct page * page){
	lru_page_deletion(page);
	pagedir_clear_page(page->thread->pagedir, pg_round_down(page->vme->virtual_addr));
	palloc_free_page(page->kaddr);
	free(page);
}


static struct list_elem * next_lru(void){
	if (list_empty(&lru_list)){
		return NULL;
	}
	if (lru_clock_elem == NULL || lru_clock_elem==list_end(&lru_list))
		return list_begin(&lru_list);
	if (list_next(lru_clock_elem) == list_end(&lru_list))
		return list_begin(&lru_list);
	return list_next(lru_clock_elem);
}

void lru_victim_select(enum palloc_flags flags){
   lru_clock_elem = next_lru();
   struct page * p = list_entry(lru_clock_elem, struct page, lru);
   
   while(pagedir_is_accessed(p->thread->pagedir, p->vme->virtual_addr)){
      pagedir_set_accessed(p->thread->pagedir, p->vme->virtual_addr, 0);
      lru_clock_elem = next_lru();
      p = list_entry(lru_clock_elem, struct page, lru);
   }
   lru_page_deletion(p);

   switch (p->vme->page_type){
      case VM_BIN:
         if(pagedir_is_dirty(p->thread->pagedir, p->vme->virtual_addr)){
            //p->vme->swap_slot = swap_out(p->kaddr);
            p->vme->page_type = VM_ANON;
         }
         break;

      case VM_FILE:
         if(pagedir_is_dirty(p->thread->pagedir, p->vme->virtual_addr))
            file_write_at(p->vme->file, p->vme->virtual_addr, p->vme->data_size, p->vme->offset);
         break;
      case VM_ANON:
         //p->vme->swap_slot = swap_out(p->kaddr);
         break;
   }
   p->vme->load_flag = 0;

   lru_page_free(p);

   //pagedir_clear_page(p->thread->pagedir, p->vme->virtual_addr);
   //palloc_free_page(p->kaddr);
   //free(p);
}
*/