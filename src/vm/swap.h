#ifndef VM_SWAP_H
#define VM_SWAP_H

#include <stddef.h>
#include <bitmap.h>
#include "vm/page.h"
#include "vm/frame.h"
#include "threads/synch.h"

void swap_init (void);
void swap_in (void *kaddr, size_t swap_slot);
size_t swap_out (void *kaddr);



#endif
