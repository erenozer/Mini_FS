#ifndef DISK_H

#define BLOCK_SIZE 1024 // Size of each block in bytes
#define NUM_BLOCKS 1024 // Total number of blocks in the filesystem
#define NUM_INODES 128 // Total number of inodes, using 128 as in the example
#define MAGIC_NUMBER 0x4D465359  // "MFSY" (Mini File System)

#define DISK_SIZE (NUM_BLOCKS * BLOCK_SIZE)


#endif // !DISK_H