#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "fs.h"
#include "disk.h"

int rmdir_fs(const char *path) {
    // Check if the path is absolute
    if(!path || path[0] != '/') {
        fprintf(stderr, "Error: Only absolute paths are supported.\n");
        return -1;
    }
    
    // Make sure the path is not empty or root
    if (!path || strcmp(path, "/") == 0) {
        fprintf(stderr, "Error: Cannot remove root.\n");
        return -1;
    }

    // Open the disk image file for reading and writing
    FILE *fp = fopen("disk.img", "rb+");
    if (!fp) {
        fprintf(stderr, "Error: Could not open disk image.\n");
        return -1;
    }

    // Resolve the path to find the directory's inode and its parent
    int parentInode = -1;  // Will store the parent directory's inode index
    char name[28];         // Will store the directory name (last component of path)
    int dirInodeIndex = resolvePath(fp, path, &parentInode, name);
    
    // Check if the directory exists
    if (dirInodeIndex == -1) {
        fprintf(stderr, "Error: Directory not found.\n");
        fclose(fp);
        return -1;
    }

    // Read the directory's inode and verify it's actually a directory
    Inode dirInode;
    if (readInode(fp, dirInodeIndex, &dirInode) != 0 || !dirInode.is_directory) {
        fprintf(stderr, "Error: Path is not a directory.\n");
        fclose(fp);
        return -1;
    }

    // Check if directory is empty (ignore "." and ".." in pathnames)
    DirectoryEntry entries[MAX_DIR_ENTRIES];
    
    // Check all data blocks allocated to this directory
    for (int i = 0; i < 4; i++) {
        // Skip unallocated blocks
        if (dirInode.direct_blocks[i] == -1) continue;
        
        // Read the directory entries from this block
        if (readBlock(fp, dirInode.direct_blocks[i], entries) != 0) {
            fprintf(stderr, "Error: Failed to read directory block.\n");
            fclose(fp);
            return -1;
        }
        
        // Check each entry in the block
        for (int j = 0; j < MAX_DIR_ENTRIES; j++) {
            // If we find any entry that's not "." or "..", directory is not empty
            if (entries[j].inode_number != -1 &&
                strcmp(entries[j].name, ".") != 0 &&
                strcmp(entries[j].name, "..") != 0) {
                fprintf(stderr, "Error: Directory is not empty.\n");
                fclose(fp);
                return -1;
            }
        }
    }

    // Directory is empty, can be removed, first free all allocated data blocks
    for (int i = 0; i < 4; i++) {
        if (dirInode.direct_blocks[i] != -1) {
            freeDataBlock(fp, dirInode.direct_blocks[i]);
        }
    }

    // Free the directory's inode to make it available for reuse
    freeInode(fp, dirInodeIndex);

    // Remove the directory entry from its parent directory
    if (removeDirEntry(fp, parentInode, name) != 0) {
        fprintf(stderr, "Error: Failed to remove directory entry from parent.\n");
        fclose(fp);
        return -1;
    }

    // Close the disk image file and return
    fclose(fp);
    return 0;
}


int delete_fs(const char *path) {
    // Validate input: ensure path exists and is absolute
    if (!path || path[0] != '/') {
        fprintf(stderr, "Error: Only absolute paths are supported.\n");
        return -1;
    }

    // Open the disk image file for reading and writing
    FILE *fp = fopen("disk.img", "rb+");
    if (!fp) {
        fprintf(stderr, "Error: Could not open disk image.\n");
        return -1;
    }

    // Resolve the path to find the file's inode and its parent directory
    int parentInode = -1;  // Will store the parent directory's inode index
    char name[28];         // Will store the file name (last component of path)
    int inodeIndex = resolvePath(fp, path, &parentInode, name);
    
    // Check if the file exists
    if (inodeIndex == -1) {
        fprintf(stderr, "Error: File not found.\n");
        fclose(fp);
        return -1;
    }

    // Read the file's inode and verify it's actually a file (not a directory)
    Inode fileInode;
    if (readInode(fp, inodeIndex, &fileInode) != 0 || fileInode.is_directory) {
        fprintf(stderr, "Error: Path is not a file.\n");
        fclose(fp);
        return -1;
    }

    // Free all data blocks allocated to this file
    for (int i = 0; i < 4; ++i) {
        // Check if this direct block is allocated
        if (fileInode.direct_blocks[i] != -1) {
            // Free the data block and mark it as unallocated
            freeDataBlock(fp, fileInode.direct_blocks[i]);
            fileInode.direct_blocks[i] = -1;  // Mark as freed
        }
    }

    // Free the file's inode to make it available for reuse
    freeInode(fp, inodeIndex);

    // Remove the file entry from its parent directory
    if (removeDirEntry(fp, parentInode, name) != 0) {
        fprintf(stderr, "Error: Failed to remove directory entry.\n");
        fclose(fp);
        return -1;
    }

    // Close the disk image file and return success
    fclose(fp);
    return 0;
}


int read_fs(const char *path, char *buf, int bufSize) {
    // Check input, ensure path is absolute and buffer is valid
    if (!path || path[0] != '/' || !buf || bufSize <= 0) {
        fprintf(stderr, "Error: Invalid arguments to read_fs.\n");
        return -1;
    }

    // Open the disk image file for reading
    FILE *fp = fopen("disk.img", "rb");
    if (!fp) {
        fprintf(stderr, "Error: Could not open disk image.\n");
        return -1;
    }

    // Resolve the path to find the file's inode
    int inodeIndex = resolvePath(fp, path, NULL, NULL);
    if (inodeIndex == -1) {
        fprintf(stderr, "Error: File not found.\n");
        fclose(fp);
        return -1;
    }

    // Read the inode for the file while checking if it's a file
    Inode inode;
    if (readInode(fp, inodeIndex, &inode) != 0 || inode.is_directory) {
        fprintf(stderr, "Error: Path is not a file.\n");
        fclose(fp);
        return -1;
    }

    int toRead = (inode.size < bufSize) ? inode.size : bufSize;
    int readBytes = 0;

    for (int i = 0; i < 4 && readBytes < toRead; i++) {
        if (inode.direct_blocks[i] == -1) continue;

        char block[BLOCK_SIZE] = {0};
        if (readBlock(fp, inode.direct_blocks[i], block) != 0) {
            fprintf(stderr, "Error: Failed to read data block.\n");
            fclose(fp);
            return -1;
        }

        // Calculate how much to copy from this block 
        int copyLen = (toRead - readBytes > BLOCK_SIZE) ? BLOCK_SIZE : (toRead - readBytes);
        // Copy the data from the block to the buffer
        memcpy(buf + readBytes, block, copyLen);
        // Update the number of bytes read
        readBytes += copyLen;
    }

    fclose(fp);
    return readBytes;
}


int write_fs(const char *path, const char *data) {
    // Ensure path is absolute 
    if (!path || path[0] != '/') {
        fprintf(stderr, "Error: Only absolute paths are supported.\n");
        return -1;
    }
    
    // Empty data will be written as an empty file, so not checking for size 0
    int dataLen = strlen(data);

    // Check if data length exceeds the maximum allowed size
    if (dataLen > BLOCK_SIZE * 4) {
        fprintf(stderr, "Error: File too large.\n");
        return -1;
    }

    // Open the disk image file
    FILE *fp = fopen("disk.img", "rb+");
    if (!fp) {
        fprintf(stderr, "Error: Could not open disk image.\n");
        return -1;
    }

    // Resolve the path to find the file's inode and its parent directory
    int fileInodeIndex = resolvePath(fp, path, NULL, NULL);
    if (fileInodeIndex == -1) {
        fprintf(stderr, "Error: File does not exist.\n");
        fclose(fp);
        return -1;
    }

    // Read the inode for the file and check if it is a file 
    Inode fileInode;
    if (readInode(fp, fileInodeIndex, &fileInode) != 0 || fileInode.is_directory) {
        fprintf(stderr, "Error: Target is not a file.\n");
        fclose(fp);
        return -1;
    }

    // Free any previously allocated data blocks
    for (int i = 0; i < 4; ++i) {
        if (fileInode.direct_blocks[i] != -1) {
            freeDataBlock(fp, fileInode.direct_blocks[i]);
            fileInode.direct_blocks[i] = -1;
        }
    }

    int remaining = dataLen;
    const char *ptr = data;
    int blocksUsed = 0;

    // Allocate new data blocks for the file
    while (remaining > 0 && blocksUsed < 4) {
        int blk = allocDataBlock(fp);
        if (blk == -1) {
            fprintf(stderr, "Error: No space to allocate data blocks.\n");
            fclose(fp);
            return -1;
        }

        // Calculate how much data to write in this block
        int toWrite = remaining > BLOCK_SIZE ? BLOCK_SIZE : remaining;
        
        // Create a temporary block buffer and copy only the data required
        char block[BLOCK_SIZE] = {0};
        memcpy(block, ptr, toWrite);
        
        // Use writeBlock to write the data to the allocated block
        if (writeBlock(fp, blk, block) != 0) {
            fprintf(stderr, "Error: Failed to write to block.\n");
            fclose(fp);
            return -1;
        }

        // Update the inode's direct block pointers
        fileInode.direct_blocks[blocksUsed] = blk;
        ptr += toWrite;
        remaining -= toWrite;
        blocksUsed++;
    }

    fileInode.size = dataLen;

    // Update the inode with the new size and block pointers
    if (writeInode(fp, fileInodeIndex, &fileInode) != 0) {
        fprintf(stderr, "Error: Failed to update inode.\n");
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return dataLen;
}


int ls_fs(const char *path, DirectoryEntry *entries, int max_entries) {
    // Ensure path exists and is absolute
    if (!path || path[0] != '/') {
        fprintf(stderr, "Error: Only absolute paths are supported.\n");
        return -1;
    }

    // Open the disk image file 
    FILE *fp = fopen("disk.img", "rb");
    if (!fp) {
        fprintf(stderr, "Error: Could not open disk image.\n");
        return -1;
    }

    // Resolve the path to find the directory's inode
    int dirInodeIndex = resolvePath(fp, path, NULL, NULL);
    if (dirInodeIndex == -1) {
        fprintf(stderr, "Error: Directory not found.\n");
        fclose(fp);
        return -1;
    }

    // Read the inode and check that if it is a directory
    Inode dirInode;
    if (readInode(fp, dirInodeIndex, &dirInode) != 0 || !dirInode.is_directory) {
        fprintf(stderr, "Error: Path is not a directory.\n");
        fclose(fp);
        return -1;
    }

    // Initialize counters and temporary storage for directory entries
    int count = 0;
    DirectoryEntry blockEntries[MAX_DIR_ENTRIES];

    // Iterate through all data blocks allocated to this directory
    for (int i = 0; i < 4 && count < max_entries; i++) {
        // Skip unallocated blocks
        if (dirInode.direct_blocks[i] == -1) continue;

        // Read the directory entries from this data block
        if (readBlock(fp, dirInode.direct_blocks[i], blockEntries) != 0) {
            fprintf(stderr, "Error: Failed to read directory block.\n");
            fclose(fp);
            return -1;
        }

        // Process each entry in the block, filtering out "." and ".." entries
        for (int j = 0; j < MAX_DIR_ENTRIES && count < max_entries; j++) {
            // Only include valid entries that are not the special "." and ".." directories
            if (blockEntries[j].inode_number != -1 && 
                strcmp(blockEntries[j].name, ".") != 0 && 
                strcmp(blockEntries[j].name, "..") != 0) {
                // Copy the valid entry to the output array
                entries[count++] = blockEntries[j];
            }
        }
    }

    // Close the disk image file and return the number of entries found
    fclose(fp);
    return count;
}


int create_fs(const char *path) {
    // Ensure path exists and is absolute
    if (!path || path[0] != '/') {
        fprintf(stderr, "Error: Only absolute paths are supported.\n");
        return -1;
    }

    // Open the disk image file
    FILE *fp = fopen("disk.img", "rb+");
    if (!fp) {
        fprintf(stderr, "Error: Could not open disk image.\n");
        return -1;
    }

    // Try to resolve the path to check if file already exists
    // If successful, the file exists. If not, we get the parent directory info
    int parentInode = -1;
    char name[28];
    
    if (resolvePath(fp, path, &parentInode, name) != -1) {
        // File already exists at this path
        fprintf(stderr, "Error: File already exists.\n");
        fclose(fp);
        return -1;
    }

    // Check if the parent directory exists, resolvePath should find it
    if (parentInode == -1) {
        fprintf(stderr, "Error: Parent directory does not exist.\n");
        fclose(fp);
        return -1;
    }

    // Allocate a new inode for the file
    int newInode = allocInode(fp);
    if (newInode == -1) {
        fprintf(stderr, "Error: No free inodes available.\n");
        fclose(fp);
        return -1;
    }

    // Initialize the file inode with default values
    Inode fileInode = {
        .is_valid = 1,          // Mark inode as valid/in use
        .size = 0,              // New file starts with zero size
        .is_directory = 0,      // This is a file, not a directory
        .owner_id = 150240719   // My Student ID
    };

    // Initialize all direct block pointers to -1 (unallocated)
    for (int i = 0; i < 4; ++i) {
        fileInode.direct_blocks[i] = -1;
    }

    // Write the new inode to disk
    if (writeInode(fp, newInode, &fileInode) != 0) {
        fprintf(stderr, "Error: Failed to write file inode.\n");
        // Free the allocated inode since writing failed
        freeInode(fp, newInode);
        fclose(fp);
        return -1;
    }

    // Add the new file entry to its parent directory
    if (addDirEntry(fp, parentInode, name, newInode) != 0) {
        fprintf(stderr, "Error: Failed to link file to parent directory.\n");
        //Free the allocated inode since linking failed
        freeInode(fp, newInode);
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}


int mkdir_fs(const char *path) {
    // Ensure path exists and is absolute
    if (!path || path[0] != '/') {
        fprintf(stderr, "Error: Only absolute paths are supported.\n");
        return -1;
    }

    // Open the disk image file
    FILE *fp = fopen("disk.img", "rb+");
    if (!fp) {
        fprintf(stderr, "Error: Could not open disk image.\n");
        return -1;
    }

    // Try to resolve the path to check if directory already exists
    // If successful, the directory exists; if not, we get the parent directory info
    int parentInode = -1;
    char name[28];
    
    if (resolvePath(fp, path, &parentInode, name) != -1) {
        // Directory already exists at this path
        fprintf(stderr, "Error: Directory already exists.\n");
        fclose(fp);
        return -1;
    }

    // Check if the parent directory exists (resolvePath should have found it)
    if (parentInode == -1) {
        fprintf(stderr, "Error: Parent directory does not exist.\n");
        fclose(fp);
        return -1;
    }

    // Try to allocate a new inode for the directory
    int newInode = allocInode(fp);
    if (newInode == -1) {
        fprintf(stderr, "Error: No free inodes available.\n");
        fclose(fp);
        return -1;
    }

    // Try to allocate a data block for the new directory
    int newBlock = allocDataBlock(fp);
    if (newBlock == -1) {
        fprintf(stderr, "Error: No free data blocks available.\n");
        // Free the allocated inode since we couldn't get a data block
        freeInode(fp, newInode);
        fclose(fp);
        return -1;
    }

    Inode dirInode = {
        .is_valid = 1,
        .size = 0,
        .is_directory = 1,
        .owner_id = 150240719, 
        .direct_blocks[0] = newBlock
    };

    // Initialize remaining direct block pointers to -1 (unallocated)
    for (int i = 1; i < 4; ++i) {
        dirInode.direct_blocks[i] = -1;
    }

    // Write the directory inode to disk
    if (writeInode(fp, newInode, &dirInode) != 0) {
        fprintf(stderr, "Error: Failed to write new directory inode.\n");
        // Clean up, free both allocated resources
        freeDataBlock(fp, newBlock);
        freeInode(fp, newInode);
        fclose(fp);
        return -1;
    }
    
    // Initialize directory entries array for "." and ".." entries
    DirectoryEntry selfAndParent[MAX_DIR_ENTRIES];

    // Zero out all entries in the array
    memset(selfAndParent, 0, sizeof(selfAndParent));

    // Mark unused entries with inode_number = -1
    for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        selfAndParent[i].inode_number = -1;
    }

    // Create the "." entry (current directory) - points to itself
    strncpy(selfAndParent[0].name, ".", 2);
    selfAndParent[0].inode_number = newInode;

    // Create the ".." entry (parent directory) - points to parent
    strncpy(selfAndParent[1].name, "..", 3);
    selfAndParent[1].inode_number = parentInode;

    // Write the initial directory entries to the allocated data block
    if (writeBlock(fp, newBlock, selfAndParent) != 0) {
        fprintf(stderr, "Error: Failed to write initial directory entries.\n");
        // Clean up, free both allocated resources
        freeDataBlock(fp, newBlock);
        freeInode(fp, newInode);
        fclose(fp);
        return -1;
    }
    
    // Add the new directory entry to its parent directory
    if (addDirEntry(fp, parentInode, name, newInode) != 0) {
        fprintf(stderr, "Error: Failed to link directory to parent.\n");
        // Clean up, free both allocated resources
        freeDataBlock(fp, newBlock);
        freeInode(fp, newInode);
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}


void mkfs(const char *diskfile) {
    // Create/open the disk image file
    FILE *fp = fopen(diskfile, "wb+");
    if (!fp) {
        fprintf(stderr, "Error: Unable to create disk image.\n");
        return;
    }

    // Initialize the entire disk with zeros (1MB = 1024 blocks of 1KB each)
    char zero_block[BLOCK_SIZE] = {0};
    for (int i = 0; i < NUM_BLOCKS; i++) {
        fwrite(zero_block, 1, BLOCK_SIZE, fp);
    }

    // Create and initialize the superblock with filesystem metadata
    SuperBlock sb = {
        .magic_number = MAGIC_NUMBER, // Filesystem identifier
        .num_blocks = NUM_BLOCKS, // Total number of blocks (1024)
        .num_inodes = NUM_INODES, // Total number of inodes (512)
        .bitmap_start = BITMAP_BLOCK, // Bitmap for data block allocation
        .inode_start = INODE_START_BLOCK, // Start of inode table
        .data_start = DATA_START_BLOCK // Start of data blocks
    };

    // Write the superblock to block 0
    fseek(fp, 0, SEEK_SET);
    fwrite(&sb, sizeof(SuperBlock), 1, fp);

    // Initialize the data block bitmap (block 1)
    char bitmap[BLOCK_SIZE] = {0};
    // Reserve the first data block for root directory
    int root_data_block = sb.data_start;
    int root_block_index = root_data_block - sb.data_start;

    // Set the bit for the root directory's data block as allocated
    bitmap[root_block_index / 8] |= (1 << (root_block_index % 8));
    
    // Write the bitmap to disk
    fseek(fp, BLOCK_SIZE * sb.bitmap_start, SEEK_SET);
    fwrite(bitmap, 1, BLOCK_SIZE, fp);

    // Initialize the inode table (blocks 2-10) with empty inodes
    Inode inode;
    memset(&inode, 0, sizeof(Inode));
    fseek(fp, BLOCK_SIZE * sb.inode_start, SEEK_SET);
    for (int i = 0; i < NUM_INODES; i++) {
        fwrite(&inode, sizeof(Inode), 1, fp);
    }

    // Initialize root directory as stated
    Inode root_inode = {
        .is_valid = 1,
        .size = 0,
        .is_directory = 1,
        .direct_blocks[0] = root_data_block,
        .owner_id = 150240719 // My Student ID
    };

    // Initialize remaining direct block pointers to -1 (unallocated)
    for (int i = 1; i < 4; i++) {
        root_inode.direct_blocks[i] = -1;
    }

    // Write the root directory inode to position 0 in the inode table
    fseek(fp, BLOCK_SIZE * sb.inode_start, SEEK_SET);
    fwrite(&root_inode, sizeof(Inode), 1, fp);

    // Initialize root directory data block with empty entries
    DirectoryEntry root_entries[MAX_DIR_ENTRIES];
    for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        root_entries[i].inode_number = -1;
        root_entries[i].name[0] = '\0';
    }
    
    // Write the empty directory entries to the root directory's data block
    fseek(fp, root_data_block * BLOCK_SIZE, SEEK_SET);
    fwrite(root_entries, sizeof(root_entries), 1, fp);
    
    fclose(fp);
}

// Read and write operations for blocks in the filesystem
int readBlock(FILE *fp, int block_index, void *buf) {
    fseek(fp, block_index * BLOCK_SIZE, SEEK_SET);
    return fread(buf, 1, BLOCK_SIZE, fp) == BLOCK_SIZE ? 0 : -1;
}

int writeBlock(FILE *fp, int block_index, const void *buf) {
    fseek(fp, block_index * BLOCK_SIZE, SEEK_SET);
    return fwrite(buf, 1, BLOCK_SIZE, fp) == BLOCK_SIZE ? 0 : -1;
}

// Allocates data blocks in the filesystem 
int allocDataBlock(FILE *fp) {
    uint8_t bitmap[BLOCK_SIZE];
    fseek(fp, BITMAP_BLOCK * BLOCK_SIZE, SEEK_SET);
    fread(bitmap, 1, BLOCK_SIZE, fp);

    for (int i = 0; i < 1013; ++i) { // 1024 - 11 data blocks
        int byte = i / 8;
        int bit = i % 8;
        if (!(bitmap[byte] & (1 << bit))) {
            bitmap[byte] |= (1 << bit);
            fseek(fp, BITMAP_BLOCK * BLOCK_SIZE, SEEK_SET);
            fwrite(bitmap, 1, BLOCK_SIZE, fp);
            return DATA_START_BLOCK + i;
        }
    }
    return -1;
}

// Frees data blocks in the filesystem 
void freeDataBlock(FILE *fp, int block_index) {
    int rel_index = block_index - DATA_START_BLOCK;
    uint8_t bitmap[BLOCK_SIZE];
    fseek(fp, BITMAP_BLOCK * BLOCK_SIZE, SEEK_SET);
    fread(bitmap, 1, BLOCK_SIZE, fp);
    bitmap[rel_index / 8] &= ~(1 << (rel_index % 8));
    fseek(fp, BITMAP_BLOCK * BLOCK_SIZE, SEEK_SET);
    fwrite(bitmap, 1, BLOCK_SIZE, fp);
}

// Allocates an inode in the filesystem
int allocInode(FILE *fp) {
    Inode inode;
    for (int i = 0; i < NUM_INODES; ++i) {
        fseek(fp, (INODE_START_BLOCK * BLOCK_SIZE) + i * sizeof(Inode), SEEK_SET);
        fread(&inode, sizeof(Inode), 1, fp);
        if (!inode.is_valid) {
            inode.is_valid = 1;
            fseek(fp, (INODE_START_BLOCK * BLOCK_SIZE) + i * sizeof(Inode), SEEK_SET);
            fwrite(&inode, sizeof(Inode), 1, fp);
            return i;
        }
    }
    return -1;
}

// Frees an inode in the filesystem
void freeInode(FILE *fp, int inode_index) {
    Inode inode = {0};
    fseek(fp, (INODE_START_BLOCK * BLOCK_SIZE) + inode_index * sizeof(Inode), SEEK_SET);
    fwrite(&inode, sizeof(Inode), 1, fp);
}

// Reads inodes in the filesystem
int readInode(FILE *fp, int inode_index, Inode *out) {
    fseek(fp, (INODE_START_BLOCK * BLOCK_SIZE) + inode_index * sizeof(Inode), SEEK_SET);
    return fread(out, sizeof(Inode), 1, fp) == 1 ? 0 : -1;
}

// Writes inodes in the filesystem
int writeInode(FILE *fp, int inode_index, const Inode *in) {
    fseek(fp, (INODE_START_BLOCK * BLOCK_SIZE) + inode_index * sizeof(Inode), SEEK_SET);
    return fwrite(in, sizeof(Inode), 1, fp) == 1 ? 0 : -1;
}

// Helper function to resolve an absolute path to its inode index
int resolvePath(FILE *fp, const char *path, int *parent_inode, char *name) {
    if (strcmp(path, "/") == 0) return 0;

    char temp[256];
    strncpy(temp, path, sizeof(temp));
    char *token = strtok(temp, "/");
    int current_inode = 0;
    int prev_inode = -1;

    while (token) {
        char *next = strtok(NULL, "/");
        if (!next) {
            if (parent_inode) *parent_inode = current_inode;
            if (name) strncpy(name, token, 28);
            if (!next) {
                // If this is the last token, return the current inode index
                int inodeIndex = findDirEntry(fp, current_inode, token);
                return inodeIndex;
            }
        }
        prev_inode = current_inode;
        // Find the directory entry for the current token
        current_inode = findDirEntry(fp, current_inode, token);
        if (current_inode == -1) return -1;
        token = next;
    }
    return current_inode;
}

// Finds a directory entry by name in a directory's inode
int findDirEntry(FILE *fp, int dir_inode_index, const char *name) {
    Inode dir_inode;
    if (readInode(fp, dir_inode_index, &dir_inode) != 0 || !dir_inode.is_directory) return -1;

    DirectoryEntry entries[MAX_DIR_ENTRIES];
    for (int i = 0; i < 4; i++) {
        // Skip unallocated blocks
        if (dir_inode.direct_blocks[i] == -1) continue;
        // Read the directory entries from this data block
        readBlock(fp, dir_inode.direct_blocks[i], entries);
        for (int j = 0; j < MAX_DIR_ENTRIES; j++) {
            // Check if the entry is valid and matches the name
            if (entries[j].inode_number != -1 && strcmp(entries[j].name, name) == 0) {
                return entries[j].inode_number;
            }
        }
    }
    return -1;
}

// Adds a directory entry to a directory's inode
int addDirEntry(FILE *fp, int dir_inode_index, const char *name, int inode_index) {
    Inode dir_inode;
    // Read the directory inode to ensure it exists and is a directory
    if (readInode(fp, dir_inode_index, &dir_inode) != 0 || !dir_inode.is_directory) return -1;

    DirectoryEntry entries[MAX_DIR_ENTRIES];
    // Iterate through all data blocks allocated to this directory
    for (int i = 0; i < 4; i++) {
        if (dir_inode.direct_blocks[i] == -1) {
            // Allocate a new data block if this one is empty
            int blk = allocDataBlock(fp);
            if (blk == -1) return -1;
            dir_inode.direct_blocks[i] = blk;
            memset(entries, 0xFF, sizeof(entries));
            strncpy(entries[0].name, name, 27);
            entries[0].name[27] = '\0';
            entries[0].inode_number = inode_index;
            writeBlock(fp, blk, entries);
            dir_inode.size++;
            writeInode(fp, dir_inode_index, &dir_inode);
            return 0;
        }
        // Read existing entries from the data block
        readBlock(fp, dir_inode.direct_blocks[i], entries);
        for (int j = 0; j < MAX_DIR_ENTRIES; j++) {
            // Find an empty slot to add the new entry
            if (entries[j].inode_number == -1) {
                // Found an empty slot, add the new entry
                strncpy(entries[j].name, name, 27);
                entries[j].name[27] = '\0';
                entries[j].inode_number = inode_index;
                // Write the updated entries back to the block
                writeBlock(fp, dir_inode.direct_blocks[i], entries);
                dir_inode.size++;
                writeInode(fp, dir_inode_index, &dir_inode);
                return 0;
            }
        }
    }
    return -1; // Directory is full
}

// Removes a directory entry from a directory's inode
int removeDirEntry(FILE *fp, int dir_inode_index, const char *name) {
    Inode dir_inode;
    // Read the directory inode to ensure it exists and is a directory
    if (readInode(fp, dir_inode_index, &dir_inode) != 0 || !dir_inode.is_directory) return -1;

    DirectoryEntry entries[MAX_DIR_ENTRIES];
    // Iterate through all data blocks allocated to this directory
    for (int i = 0; i < 4; i++) {
        // Skip unallocated blocks
        if (dir_inode.direct_blocks[i] == -1) continue;
        // Read existing entries from the data block
        readBlock(fp, dir_inode.direct_blocks[i], entries);
        for (int j = 0; j < MAX_DIR_ENTRIES; j++) {
            if (entries[j].inode_number != -1 && strcmp(entries[j].name, name) == 0) {
                // Found the entry to remove, mark it as unused
                entries[j].inode_number = -1;
                entries[j].name[0] = '\0';
                // Write the updated entries back to the block
                writeBlock(fp, dir_inode.direct_blocks[i], entries);
                dir_inode.size--;
                writeInode(fp, dir_inode_index, &dir_inode);
                return 0;
            }
        }
    }
    return -1; 
}
