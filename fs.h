#ifndef FS_H

#include "disk.h"

#define BLOCK_SIZE 1024

// Superblock
typedef struct { 
    int magic_number; // Filesystem identifier
    int num_blocks; // Total blocks (1024)
    int num_inodes; // Total inodes (e.g., 128)
    int bitmap_start; // Block index of free-block bitmap
    int inode_start; // Block index of inode table
    int data_start; // Block index of first data block
} SuperBlock;

// Inode
typedef struct { 
    int is_valid;  // 0=free, 1=used
    int size;      // bytes (file) or entry count (directory)
    int direct_blocks [4]; // direct block pointers
    int is_directory; // 0=file, 1=directory
    int owner_id; // Student ID number (150240719)
} Inode;

typedef struct {
    int inode_number; 
    char name[28]; // File or directory name (27 chars + null terminator)
} DirectoryEntry;

void mkfs(const char *diskfile);
int mkdir_fs(const char *path);
int create_fs(const char *path);
int write_fs(const char *path, const char *data);
int read_fs(const char *path, char *buf, int bufsize); 
int delete_fs(const char *path);
int rmdir_fs(const char *path); 
int ls_fs(const char *path, DirectoryEntry *entries , int max_entries);

#endif // !FS_H