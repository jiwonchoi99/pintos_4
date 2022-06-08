#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "filesys/file.h"
#include "threads/synch.h"
#include "vm/page.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

bool handle_mm_fault(struct vm_entry *vme);
struct list_elem *execute_munmap(struct mmap_file *mmap_file); 
bool expand_stack(void *addr);

#endif /* userprog/process.h */
