/*
#ifdef VM_FRAME_H
#define VM_FRAME_H

#include <list.h>
#include "threads/thread.h"
#include "threads/palloc.h"
#include "vm/page.h"


struct page{
	void *kaddr;
	struct thread *thread;
	struct vm_entry *vme;
	struct list_elem lru;
	bool pinned;
};

void lru_list_init(void);
void lru_page_insertion(struct page*);
void lru_page_deletion(struct page* );
struct page * page_allocation(enum palloc_flags);
void kaddr_page_free(void *);
void lru_page_free(struct page*);
void lru_victim_select(void);







#endif
*/