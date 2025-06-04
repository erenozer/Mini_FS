#ifndef FS_H

#include "disk.h"

#define BLOCK_SIZE 1024

#define BITMAP_BLOCK 1
#define INODE_START_BLOCK 2
#define DATA_START_BLOCK 11
#define MAX_DIR_ENTRIES (BLOCK_SIZE / sizeof(DirectoryEntry))

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
    int direct_blocks[4]; // direct block pointers
    int is_directory; // 0=file, 1=directory
    int owner_id; // Student ID number (150240719)
} Inode;

typedef struct {
    int inode_number; 
    char name[28]; // File or directory name (27 chars + null terminator)
} DirectoryEntry;

// Filesystem operations
void mkfs(const char *diskfile);
int mkdir_fs(const char *path);
int create_fs(const char *path);
int write_fs(const char *path, const char *data);
int read_fs(const char *path, char *buf, int bufsize); 
int delete_fs(const char *path);
int rmdir_fs(const char *path); 
int ls_fs(const char *path, DirectoryEntry *entries , int max_entries);

// Helper functions for filesystem operations
int readBlock(FILE *fp, int block_index, void *buf);
int writeBlock(FILE *fp, int block_index, const void *buf);
int allocDataBlock(FILE *fp);
void freeDataBlock(FILE *fp, int block_index);
int allocInode(FILE *fp);
void freeInode(FILE *fp, int inode_index);
int readInode(FILE *fp, int inode_index, Inode *out);
int writeInode(FILE *fp, int inode_index, const Inode *in);
int resolvePath(FILE *fp, const char *path, int *parent_inode, char *name);
int findDirEntry(FILE *fp, int dir_inode_index, const char *name);
int addDirEntry(FILE *fp, int dir_inode_index, const char *name, int inode_index);
int removeDirEntry(FILE *fp, int dir_inode_index, const char *name);

#endif // !FS_H