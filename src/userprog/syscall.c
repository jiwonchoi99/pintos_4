#include "userprog/syscall.h"
#include <stdio.h>
#include <string.h>
#include "userprog/process.h"
#include "filesys/filesys.h"
#include <syscall-nr.h>
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "threads/malloc.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/input.h"
#include "vm/page.h"
#include "userprog/pagedir.h"
#include "threads/synch.h"
#include "filesys/file.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
//Pin//
extern struct lock lru_list_lock;

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  lock_init(&filesys_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}


static void
syscall_handler (struct intr_frame *f UNUSED) 
{

  switch (*(uint32_t *)(f->esp)){
	case SYS_HALT:
	  halt();
	  break;

	case SYS_EXIT:	
	  validate_user_vaddr(f->esp+4);
	  exit(*(uint32_t *)(f->esp+4));
	  break;

	case SYS_EXEC:
	  validate_user_vaddr(f->esp+4);	 
	  f->eax = exec((const char *)*(uint32_t *)(f->esp+4));
	  break;

	case SYS_WAIT:
	  validate_user_vaddr (f->esp+4);
	  f->eax = wait((pid_t *)*(uint32_t *)(f->esp+4));
	  break;

	case SYS_CREATE:
	  validate_user_vaddr (f->esp+4);
	  validate_user_vaddr (f->esp+8);
	  f->eax = create((const char *)*(uint32_t *)(f->esp+4), (unsigned)*(uint32_t *)(f->esp+8));
	  break;

	case SYS_REMOVE:
	  validate_user_vaddr (f->esp+4);
	  f->eax = remove((const char *)*(uint32_t *)(f->esp+4));
	  break;

	case SYS_OPEN:
	  validate_user_vaddr (f->esp+4);
	  f->eax = open((const char *)*(uint32_t *)(f->esp+4));
	  break;

	case SYS_FILESIZE:
	  validate_user_vaddr (f->esp+4);
	  f->eax = filesize((int)*(uint32_t *)(f->esp+4));
	  break;

	case SYS_READ:
	  validate_user_vaddr (f->esp+4);
	  validate_user_vaddr (f->esp+8);
	  validate_user_vaddr (f->esp+12);
	  f->eax = read((int)*(uint32_t *)(f->esp+4), (void *)*(uint32_t *)(f->esp+8), (unsigned)*(uint32_t *)(f->esp+12));
	  break;

	case SYS_WRITE:
	  validate_user_vaddr (f->esp+4);
	  validate_user_vaddr (f->esp+8);
	  validate_user_vaddr (f->esp+12);
	  f->eax = write((int)*(uint32_t *)(f->esp+4), (void *)*(uint32_t *)(f->esp+8), (unsigned)*((uint32_t *)(f->esp+12)));
	  break;

	case SYS_SEEK:
	  validate_user_vaddr (f->esp+4);
	  validate_user_vaddr (f->esp+8);
	  seek((int)*(uint32_t *)(f->esp+4), (unsigned)*(uint32_t *)(f->esp+8));
	  break;

	case SYS_TELL:
	  validate_user_vaddr (f->esp+4);
	  f->eax = tell((int)*(uint32_t *)(f->esp+4));
	  break;

	case SYS_CLOSE:
	  validate_user_vaddr (f->esp+4);
	  close((int)*(uint32_t *)(f->esp+4));
	  break;

	case SYS_SIGACTION:
	  validate_user_vaddr (f->esp+4);
	  validate_user_vaddr (f->esp+8);

	  sigaction((int)*(uint32_t *)(f->esp+4), (void *)*(uint32_t *)(f->esp+8));

	  break;

	case SYS_SENDSIG:
	  validate_user_vaddr (f->esp+4);
	  validate_user_vaddr (f->esp+8);	  

	  sendsig((pid_t)*(uint32_t *)(f->esp+4), (int)*(uint32_t *)(f->esp+8));

	  break;

	case SYS_YIELD:
	  thread_yield();
	  break;

	case SYS_MMAP:
		validate_user_vaddr(f->esp+4);
		validate_user_vaddr(f->esp+8);
		f->eax = mmap((int)*(uint32_t *)(f->esp+4), (void *)*(uint32_t *)(f->esp+8));
		break;
	case SYS_MUNMAP:
		validate_user_vaddr(f->esp+4);
		munmap((int)*(uint32_t * )(f->esp+4));
		break;
	
	case SYS_ISDIR:
		validate_user_vaddr (f->esp+4);
		f->eax = isdir((int)*(uint32_t *)(f->esp+4));
		break;

   case SYS_CHDIR:
    	validate_user_vaddr(f->esp+4);
      	f->eax = chdir((const char *)*(uint32_t *)(f->esp+4));
      	break;
   
   case SYS_MKDIR:
      	validate_user_vaddr(f->esp+4);
      	f->eax = mkdir((const char *)*(uint32_t *)(f->esp+4));
      	break;


	case SYS_READDIR:
		validate_user_vaddr(f->esp+4);
		validate_user_vaddr(f->esp+8);
		f->eax = readdir((int)*(uint32_t *)(f->esp+4), (char*)*(uint32_t *)(f->esp+8));
		break;
	
	case SYS_INUMBER:
		validate_user_vaddr(f->esp+4);
     	f->eax = inumber((int)*(uint32_t *)(f->esp+4));
      	break;	
	
	default:
		exit(-1);

  }
}

void validate_user_vaddr(const void *vaddr){
	if (!is_user_vaddr(vaddr) || vaddr == NULL){
		exit(-1);
	}
}

void halt(void){
	shutdown_power_off();
}

void exit(int status){
	struct thread *cur = thread_current();
	struct list_elem *e;
	
	cur -> exit_status = status;
	printf("%s: exit(%d)\n", cur->name, status);

	for (e = list_begin(&cur->child); e!=list_end(&cur->child); e=list_next(e)){
		struct thread *t = list_entry (e, struct thread, child_elem);
		wait(t->tid);
	}

	for (int i=0; i<10; i++){
		if (cur->save_signal[i]==NULL)
			break;
		free(cur->save_signal[i]);
	}
	thread_exit();
}

pid_t exec(const char *command){
	char *file_name[128];
	memcpy(file_name, command, strlen(command)+1);
	pid_t pid = process_execute(file_name);
	
	return pid;
}

int wait(pid_t pid){
	return process_wait(pid);
}


bool create (const char *file, unsigned initial_size){
	if (file == NULL)
		exit(-1);
	return filesys_create(file, initial_size);
}

bool remove (const char *file){
	if (file == NULL)
		exit(-1);
	return filesys_remove(file);
}

// int open (const char *file){
// 	struct thread *cur = thread_current();
// 	int fd;
	
// 	if (file == NULL)
// 		exit(-1);
// 	lock_acquire(&filesys_lock);
// 	struct file *open_file = filesys_open(file);
//     lock_release(&filesys_lock);
// 	if (open_file == NULL){
// 		return -1;
// 	}
// 	else{
// 		int next_fd = cur->next_fd;
// 		if (next_fd>=2 && next_fd <64){
// 			if (strcmp(cur->name, file)==0)
// 				file_deny_write(open_file);
// 			cur->fdt[next_fd] = open_file;
// 			thread_current()->next_fd = next_fd + 1;
// 			return next_fd;
// 		}
// 	}
// 	return -1;
// }
int open(const char *file)
{
    int fd;
    struct file *retval;
    struct thread *cur = thread_current();

    lock_acquire(&filesys_lock);
    retval = filesys_open(file);
    if (retval != NULL)
    {
        if (strcmp(cur->name, file) == 0)
        {
            file_deny_write(retval);
        }
        fd = cur->next_fd;
        if(fd>64){
            return -1;
        }
        cur->fdt[fd] = retval;
    }
    else
    {
        fd = -1;
    }
    for(int i = 3; i < 64; i++){
        if(cur->fdt[i] == NULL){
            cur->next_fd = i;
            break;
        }
    }
    lock_release(&filesys_lock);

    return fd;
}

int filesize (int fd){

	struct file *file = thread_current()->fdt[fd];
	if (file == NULL)
		return -1;
	off_t length = file_length(file);
	return length;
}

int read (int fd, void *buffer, unsigned size){
    validate_user_vaddr(buffer);
	int return_val;

	uint8_t *addr = pg_round_down(buffer);
	
	lock_acquire(&filesys_lock);
	pin_bunch(addr, size);
	if (fd == 0){
		int count = 0;
		while (count < size){
		*(uint8_t *)(buffer+count) = input_getc();
		count++;
		}
		return_val = count;
	}
	else{
		struct file *file = thread_current()->fdt[fd];
		if (file == NULL){
			lock_release(&filesys_lock);
			unpin_bunch(addr, size);
			return -1;
		}
		return_val = file_read(file, buffer, size);
	}
	unpin_bunch(addr, size);
	lock_release(&filesys_lock);

	return return_val;
}


int write (int fd, const void *buffer, unsigned size){
	lock_acquire(&filesys_lock);
    int return_val = -1;

	uint8_t *addr = pg_round_down(buffer);
	pin_bunch(addr, size);

	if(fd==1){
		putbuf(buffer, size);
		return_val = size;
    }

	else{
		struct file *f_path = thread_current()->fdt[fd];
		if (f_path == NULL){
			unpin_bunch(addr, size);
			lock_release(&filesys_lock);
			return -1 ;
		}
		return_val = file_write(f_path, buffer, size);
	}
	unpin_bunch(addr, size);
	lock_release (&filesys_lock);
	
	return return_val;
}

void seek(int fd, unsigned position){
   struct file * f_path = thread_current() -> fdt[fd];
   if (f_path == NULL){
      return ;
   }
   return file_seek(f_path, position);
}

unsigned tell(int fd){
   struct file * f_path = thread_current() -> fdt[fd];
   if (f_path == NULL){
      return -1;
   }
   return file_tell(f_path);
}

void close(int fd){
   struct file * f_path = thread_current() -> fdt[fd];
   if (f_path == NULL){
      return ;
   }
   lock_acquire(&filesys_lock);
   file_close(f_path);
   lock_release(&filesys_lock);
   thread_current() ->fdt[fd] = NULL;

}

void sched_yield (void){
	thread_yield();
}

void sigaction (int signum, void (*handler)(void)){
	struct thread *cur = thread_current();
	
	int i = 0;
	while(cur->save_signal[i]!=NULL)
		i++;

	struct signal* sig_struct = (struct signal *)malloc(sizeof(struct signal));
	
	sig_struct->signum = signum;
    sig_struct->sig_handler = handler;
	(cur -> save_signal[i]) = sig_struct;
}


void sendsig (pid_t pid, int signum){
	sendsig_thread (pid, signum);
}

int mmap(int fd, void *addr){
	if (addr == NULL)
		return -1;
	if(pg_ofs(addr) != 0)
		return -1;
	if(fd < 2)
		return -1;

	struct file *file = thread_current() ->fdt[fd];
	if (file ==NULL)
		return -1;
	file = file_reopen(file);

	uint32_t read_bytes = file_length(file);

	if (read_bytes == 0){
		lock_acquire(&filesys_lock);
		file_close(file);
		lock_release(&filesys_lock);
		return -1;
	}


	struct mmap_file *mmap_f = malloc(sizeof(struct mmap_file));
	if (mmap_f == NULL){
		lock_acquire(&filesys_lock);
		file_close(file);
		lock_release(&filesys_lock);
		return -1;
	}

	
	memset(mmap_f, 0, sizeof(struct mmap_file));
	mmap_f -> file = file;
	mmap_f -> mapid = thread_current()->next_mapid++;

	list_init(&mmap_f -> vme_list);

	list_push_back(&thread_current() -> mmap_list, &mmap_f ->elem);

	off_t ofs = 0;
	while (read_bytes > 0){

      size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
      size_t page_zero_bytes = PGSIZE - page_read_bytes;

	  struct vm_entry *vme;	  
	  vme = vme_search(addr);
	  
	  if(vme!=NULL){
	
		execute_munmap(mmap_f);
		return -1;
	  }	
	  
	  vme = (struct vm_entry *)malloc(sizeof(struct vm_entry));
	  if (vme == NULL){

		execute_munmap(mmap_f);
		return -1;
	  }
	  
	  memset(vme, 0, sizeof(struct vm_entry));

	  vme->virtual_addr = addr;
	  vme->page_type = VM_FILE;
	  vme->file = file;
	  vme->offset = ofs;
	  vme->data_size = page_read_bytes;
	  vme->zero_size = page_zero_bytes;
	  vme->load_flag = 0;
	  vme->can_write = 1;	

	  vme_insertion(&thread_current()->vm, vme);

	  list_push_back(&mmap_f->vme_list, &vme->elem_mmap);
	  
      read_bytes -= page_read_bytes;
	  ofs += page_read_bytes;
	  addr += PGSIZE;
    }
	return mmap_f -> mapid;
}


void munmap(int mapid){

	struct thread *cur = thread_current();
	struct list_elem *e;
	struct mmap_file *mmap_f;
	if(!list_empty(&cur->mmap_list)){
		for(e = list_begin(&cur->mmap_list); e != list_end(&cur->mmap_list);){

			mmap_f = list_entry(e, struct mmap_file, elem);
			if (mmap_f -> mapid == mapid){
				e = execute_munmap(mmap_f);
			
			}
			else
				e = list_next(e);
		}
	return ;
	}
}

void pin_bunch (void *offset, int size){
	void *addr;
	void *kaddr;
	struct vm_entry *vme;
	struct page *page;


	for (addr=offset; addr<offset+size; addr+=PGSIZE){
		vme = vme_search(addr);

		if(vme==NULL)
			continue;

		if(!vme->load_flag)
			handle_mm_fault(vme);	
		lock_acquire(&lru_list_lock);
		kaddr = pagedir_get_page(thread_current()->pagedir, addr);	
		
		page = page_search (kaddr);
		page -> pinned = true;
		lock_release(&lru_list_lock);
	
	}
}

void unpin_bunch (void *offset, int size){
	void *addr;
	void *kaddr;
	struct vm_entry *vme;
	struct page *page;


	for (addr=offset; addr<offset+size; addr+=PGSIZE){
		vme = vme_search(addr);

		if(vme==NULL)
			continue;
		if(vme->load_flag){
			lock_acquire(&lru_list_lock);
			kaddr = pagedir_get_page(thread_current()->pagedir, addr);	
			page = page_search (kaddr);
			page -> pinned = false;
			lock_release(&lru_list_lock);
		}
	}
}

bool
isdir (int fd)
{
	struct file *file;
  	struct inode *inode;
	bool isdir;
  
  	file = thread_current()->fdt[fd];
  	if (file == NULL)
    	exit (-1);

  	inode = file_get_inode (file);
  	isdir = inode_is_dir (inode);
  	return isdir;

}
/*
bool chdir(const char *dir){
	return filesys_change_dir(dir);

}
*/
bool
chdir (const char *path_o)
{
  char path[256 + 1];
  strlcpy (path, path_o, 256);
  strlcat (path, "/0", 256);

  char name[256 + 1];
  struct dir *dir = parse_path (path, name);
  if (!dir)
    return false;
  dir_close (thread_current ()->current_dir);
  thread_current ()->current_dir = dir;
  return true;
}

bool mkdir(const char *dir){
	return filesys_create_dir(dir);
}

bool readdir(int fd, char *name){
	bool success = false;

	struct file *file;
  	struct inode *inode;
	bool isdir;
  
  	file = thread_current()->fdt[fd];
  	if (file == NULL)
    	exit (-1);

  	inode = file_get_inode (file);
  	isdir = inode_is_dir (inode);

	if (isdir){
		success = dir_readdir((struct dir*)file, name);
	}
	
	return success;
}

int inumber(int fd){

	struct file *file;
  	struct inode *inode;
  
  	file = thread_current()->fdt[fd];
  	if (file == NULL)
    	exit (-1);

  	inode = file_get_inode (file);

   	int inum = inode_get_inumber(inode);
   	return inum;
}