#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "filesys/buffer_cache.h"
#include "threads/thread.h"
#include "lib/string.h"

#define PATH_MAX 256
//NAME_MAX 14


/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) 
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");
  bc_init();
  inode_init ();
  free_map_init ();

  if (format) 
    do_format ();

  free_map_open ();

  thread_current ()->current_dir = dir_open_root ();

}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void) 
{
  free_map_close ();
  bc_term();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size) 
{
  block_sector_t inode_sector = 0;

  char file_name [NAME_MAX+1];
  struct dir *dir = parse_path(name, file_name);
  //struct dir *dir = dir_open_root ();


  bool success = (dir != NULL
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector, initial_size, 0)
                  && dir_add (dir, file_name, inode_sector));
  if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);
  dir_close (dir);

  return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{
  // char file_name [NAME_MAX+1];
  // struct dir *dir = parse_path (name, file_name);
  // //struct dir *dir = dir_open_root ();
  // struct inode *inode = NULL;

  // if (dir != NULL)
  //   dir_lookup (dir, file_name, &inode);
  // else
  //   return NULL;
  // dir_close (dir);

  // return file_open (inode);

  char file_name[NAME_MAX+1]; 
  struct inode *inode;
  char temp[NAME_MAX+1];
  
  struct dir *dir = parse_path (name, file_name);
  //struct dir *dir = dir_open_root ();
  if (dir == NULL){
    //dir_close(dir);
    return NULL;
  }
  
  if (!dir_lookup (dir, file_name, &inode)){
    dir_close(dir);
    return NULL;
  }
  //dir_lookup (dir, file_name, &inode);
  dir_close (dir); 
  return file_open (inode);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name) 
{
  bool success = false;
  char file_name[NAME_MAX+1]; 
  struct inode *inode;
  struct dir *current_dir = NULL;
  char temp[NAME_MAX+1];
  
  struct dir *dir = parse_path (name, file_name);

  if (dir == NULL){
    return success;
  }

  if (!dir_lookup (dir, file_name, &inode)){
    dir_close(dir);
    return success;
  }

  if (inode_is_dir(inode) == 1){ // directory
    
    struct dir * t_dir = thread_current()->current_dir;
    struct inode * inode_ = dir_get_inode(t_dir);

    if(inode_ == inode){
      dir_close(dir);
      return success;
    }
    
    int cnt = inode_open_cnt(inode);
    if (cnt>1){
      dir_close(dir);
      return success;
    }
    
    current_dir = dir_open(inode);
    if (current_dir == NULL){
      dir_close(dir);
      return success;
    }
    
    if (dir_readdir(current_dir, temp) == 0){
      if (dir_remove (dir, file_name)){
        success = true;
      }
    }

    dir_close (current_dir);
  }

  else{ // file
    if(dir_remove (dir, file_name)){
      success = true;
    }
  }

  dir_close (dir); 
  return success;
}


/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");
  
  struct dir *root_dir = dir_open_root();
  dir_add(root_dir, ".", ROOT_DIR_SECTOR);
  dir_add(root_dir, "..", ROOT_DIR_SECTOR);
  dir_close (root_dir);

  free_map_close ();
  printf ("done.\n");
}




struct dir *parse_path (char *path_name, char *file_name){

  struct dir *dir;

  if (path_name == NULL || file_name == NULL){
    //goto fail;
    return NULL;
  }

  if (strlen (path_name) == 0)
    return NULL;


  char path_copy [PATH_MAX];
  strlcpy (path_copy, path_name, PATH_MAX+1);

  if (path_copy[0] == '/')
    dir = dir_open_root ();
  else
    dir = dir_reopen (thread_current ()->current_dir);


  char *token, *nextToken, *savePtr;

  token = strtok_r (path_copy, "/", &savePtr);
  nextToken = strtok_r (NULL, "/", &savePtr);

  // if (token == NULL){
  //   strlcpy(file_name, ".", 2);
  //   return dir;
  // }

  if (token != NULL && nextToken == NULL && strlen(token) > NAME_MAX + 1){
    dir_close(dir);
    return NULL;
  }


  while (token != NULL && nextToken != NULL){
    struct inode *inode = NULL;

    if(strlen(token) >NAME_MAX + 1 || strlen(nextToken) > NAME_MAX +1){
      dir_close(dir);
      return NULL;
    }

    if (!dir_lookup(dir, token, &inode)){
      dir_close(dir);
      return NULL;
    }

    dir_close(dir);
    if (!inode_is_dir(inode)){
      return NULL;
    }

    dir = dir_open(inode);
    //token = nextToken;
    strlcpy(token, nextToken, PATH_MAX);
    nextToken = strtok_r (NULL, "/", &savePtr);
  }
  
  if (token==NULL)
    strlcpy(file_name, ".", 2);
  
  else
    strlcpy (file_name, token, PATH_MAX);
  return dir;
}



bool filesys_change_dir(const char *dir){

  if (strlen (dir) == 0)
    return false;
  char cp_dir[257];
  strlcpy(cp_dir, dir, 256);
  strlcat(cp_dir, "/0", 256);
  char file_name[NAME_MAX + 1];
  bool success = false;
  struct inode *inode;
  struct dir *directory;
  struct dir *change_directory;
  
  directory = parse_path (cp_dir, file_name);
  
  if (directory == NULL){
    return success;
  }
  
  if(!dir_lookup(directory,file_name, &inode))
    return success;

  change_directory = dir_open(inode);
  if (thread_current()->current_dir == NULL)
    thread_current()->current_dir = change_directory;
  else{
    dir_close(thread_current()->current_dir);
    thread_current()->current_dir = change_directory;
  }
  // dir_close (thread_current()->current_dir);
  
  // thread_current()->current_dir = directory;
  success = true;
  return success; 
}


bool filesys_create_dir(const char *name){

  block_sector_t inode_sector = 0;

  char file_name [NAME_MAX+1];
  struct dir *dir = parse_path(name, file_name);
  //struct dir *dir = dir_open_root ();


  bool success = (dir != NULL
                  && free_map_allocate (1, &inode_sector)
                  && dir_create (inode_sector, 16)
                  && dir_add (dir, file_name, inode_sector));
  if (!success && inode_sector != 0){ 
    free_map_release (inode_sector, 1);
  }

  if (success){
    struct inode *inode = inode_open (inode_sector);
    struct dir *directory = dir_open (inode);

    dir_add (directory, ".", inode_sector);

    struct inode * inode_ = dir_get_inode(dir);
    block_sector_t sector = inode_get_inumber(inode_);

    dir_add (directory, "..", sector);
    dir_close (directory);
  }
  dir_close (dir);
  return success;
}

////// Similar with vm hash table implemented in page.c //////

void dentry_init (struct hash *dentry_cache){
	hash_init (dentry_cache, dentry_hash_function, dentry_less_function, NULL); 
}

void dentry_destroy (struct hash *dentry_cache){
	hash_destroy(dentry_cache, dentry_destructor);
}

void dentry_destructor (struct hash_elem *e, void *aux){
	struct dc_entry *dce = hash_entry (e, struct dc_entry, h_elem);
	free(dce);
}

static unsigned dentry_hash_function(const struct hash_elem *e, void *aux){
  
  struct dc_entry *dce = hash_entry(e, struct dc_entry, h_elem);
  unsigned hash_val = hash_string(dce->path);
  return hash_val;

}

static bool dentry_less_function (const struct hash_elem *e_a, const struct hash_elem *e_b, void *aux){

	struct dc_entry *dce_a;
	struct dc_entry *dce_b;
	
	dce_a = hash_entry (e_a, struct dc_entry, h_elem);
	dce_b = hash_entry (e_b, struct dc_entry, h_elem);

	char * path_a = dce_a->path;
	char * path_b = dce_b->path;

  if (strcmp(path_a, path_b)<0)
		return true;
	else
		return false;
}

bool dentry_insertion (struct hash *dentry_cache, struct dc_entry *dce){
	
	struct hash_elem *e;
	e = &dce->h_elem;	
	struct hash_elem *old = hash_insert(dentry_cache, e);
	return (old == NULL);
}
 
bool dentry_deletion (struct hash *dentry_cache, struct dc_entry *dce){
	struct hash_elem *e;
	e = &dce->h_elem;

	struct hash_elem *found = hash_delete(dentry_cache, e);
  if (found!=NULL){
    free(dce->path);
    free(dce);
  }
	return (found != NULL);
}

struct dc_entry *dentry_search (const char *path){

	struct dc_entry dce;

	dce.path = path;
	struct hash_elem *e = hash_find (&dentry_cache, &(dce.h_elem));   

	if (!e){
		return NULL;
  }

	return hash_entry(e, struct dc_entry, h_elem);
}

struct dc_entry *dentry_parent_search (const char *path){

  int i;
  int path_length;
	struct dc_entry dce;
  
  path_length = strlen(path);

  char *path_temp = malloc(path_length+1);
  

  strlcpy(path_temp, path, path_length+1);
  
  for (i=path_length; i>=0; i--){
    if(path[i]=='/')
      break;
  }
  char *path_parent = malloc(i+1);
  strlcpy(path_parent, path_temp, i+1);

	dce.path = path_parent;
	struct hash_elem *e = hash_find (&dentry_cache, &(dce.h_elem));   

	if (!e){
    free(path_temp);
    free(path_parent);
		return NULL;
  }
  free(path_temp);
  free(path_parent);

	return hash_entry(e, struct dc_entry, h_elem);
}

