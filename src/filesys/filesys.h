#ifndef FILESYS_FILESYS_H
#define FILESYS_FILESYS_H

#include <stdbool.h>
#include "filesys/off_t.h"
#include "lib/kernel/hash.h"
#include "lib/kernel/list.h"
#include "devices/block.h"

/* Sectors of system file inodes. */
#define FREE_MAP_SECTOR 0       /* Free map file inode sector. */
#define ROOT_DIR_SECTOR 1       /* Root directory file inode sector. */


struct hash dentry_cache;

struct dc_entry{
    char *path;
    block_sector_t inumber;
    struct hash_elem h_elem;
};


/* Block device that contains the file system. */
extern struct block *fs_device;

void filesys_init (bool format);
void filesys_done (void);
bool filesys_create (const char *name, off_t initial_size);
struct file *filesys_open (const char *name);
bool filesys_remove (const char *name);

struct dir *parse_path(char *path_name, char *file_name);
bool filesys_change_dir(const char *dir);
bool filesys_create_dir(const char *dir);

void dentry_init (struct hash *dentry_cache);
void dentry_destroy (struct hash *dentry_cache);
void dentry_destructor (struct hash_elem *e, void *aux);
static unsigned dentry_hash_function(const struct hash_elem *e, void *aux);
static bool dentry_less_function (const struct hash_elem *e_a, const struct hash_elem *e_b, void *aux);
bool dentry_insertion (struct hash *dentry_cache, struct dc_entry *dce);
bool dentry_deletion (struct hash *dentry_cache, struct dc_entry *dce);
struct dc_entry *dentry_search (const char *path);
struct dc_entry *dentry_parent_search (const char *path);

#endif /* filesys/filesys.h */
