#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "fs.h"
#include "disk.h"

int create_fs(const char *path) {
    if (!path || path[0] != '/') {
        fprintf(stderr, "Error: Only absolute paths are supported.\n");
        return -1;
    }

    FILE *fp = fopen("disk.img", "rb+");
    if (!fp) {
        fprintf(stderr, "Error: Could not open disk image.\n");
        return -1;
    }

    int parentInode = -1;
    char name[28];
    if (resolvePath(fp, path, &parentInode, name) != -1) {
        fprintf(stderr, "Error: File already exists.\n");
        fclose(fp);
        return -1;
    }

    if (parentInode == -1) {
        fprintf(stderr, "Error: Parent directory does not exist.\n");
        fclose(fp);
        return -1;
    }

    int newInode = allocInode(fp);
    if (newInode == -1) {
        fprintf(stderr, "Error: No free inodes available.\n");
        fclose(fp);
        return -1;
    }

    Inode fileInode = {
        .is_valid = 1,
        .size = 0,
        .is_directory = 0,
        .owner_id = 150240719
    }

    for (int i = 0; i < 4; ++i) {
        dirInode.direct_blocks[i] = -1;
    }

    if (writeInode(fp, newInode, &fileInode) != 0) {
        fprintf(stderr, "Error: Failed to write file inode.\n");
        freeInode(fp, newInode);
        fclose(fp);
        return -1;
    }

    if (addDirEntry(fp, parentInode, name, newInode) != 0) {
        fprintf(stderr, "Error: Failed to link file to parent directory.\n");
        freeInode(fp, newInode);
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}


int mkdir_fs(const char *path) {
    if (!path || path[0] != '/') {
        fprintf(stderr, "Error: Only absolute paths are supported.\n");
        return -1;
    }

    FILE *fp = fopen("disk.img", "rb+");
    if (!fp) {
        fprintf(stderr, "Error: Could not open disk image.\n");
        return -1;
    }

    // Try to resolve the parent directory
    int parentInode = -1;
    char name[28];
    if (resolvePath(fp, path, &parentInode, name) != -1) {
        fprintf(stderr, "Error: Directory already exists.\n");
        fclose(fp);
        return -1;
    }

    // If parentInode is -1, it means the parent directory does not exist
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

    // Initialize the first direct block
    for (int i = 1; i < 4; ++i) {
        dirInode.direct_blocks[i] = -1;
    }

    if (writeInode(fp, newInode, &dirInode) != 0) {
        fprintf(stderr, "Error: Failed to write new directory inode.\n");
        freeDataBlock(fp, newBlock);
        freeInode(fp, newInode);
        fclose(fp);
        return -1;
    }
    // Initialize directory entries for "." and ".."
    DirectoryEntry selfAndParent[MAX_DIR_ENTRIES];
    
    // Zero out all entries
    memset(selfAndParent, 0, sizeof(selfAndParent));

    // Mark unused entries with inode_number = -1
    for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        selfAndParent[i].inode_number = -1;
    }

    // Add "." and ".."
    strncpy(selfAndParent[0].name, ".", 2);
    selfAndParent[0].inode_number = newInode;

    strncpy(selfAndParent[1].name, "..", 3);
    selfAndParent[1].inode_number = parentInode;

    // "." entry → points to self
    strncpy(selfAndParent[0].name, ".", 2);
    selfAndParent[0].inode_number = newInode;

    // ".." entry → points to parent
    strncpy(selfAndParent[1].name, "..", 3);
    selfAndParent[1].inode_number = parentInode;

    // Write the entry block to the new directory's data block
    if (writeBlock(fp, newBlock, selfAndParent) != 0) {
        fprintf(stderr, "Error: Failed to write initial directory entries.\n");
        freeDataBlock(fp, newBlock);
        freeInode(fp, newInode);
        fclose(fp);
        return -1;
    }
    
    if (addDirEntry(fp, parentInode, name, newInode) != 0) {
        fprintf(stderr, "Error: Failed to link directory to parent.\n");
        freeDataBlock(fp, newBlock);
        freeInode(fp, newInode);
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}


void mkfs(const char *diskfile) {
    FILE *fp = fopen(diskfile, "wb+");
    if (!fp) {
        fprintf(stderr, "Error: Unable to create disk image.\n");
        return;
    }

    // Allocate 1MB space for disk
    char zero_block[BLOCK_SIZE] = {0};
    for (int i = 0; i < NUM_BLOCKS; i++) {
        fwrite(zero_block, 1, BLOCK_SIZE, fp);
    }

    SuperBlock sb = {
        .magic_number = MAGIC_NUMBER,
        .num_blocks = NUM_BLOCKS,
        .num_inodes = NUM_INODES,
        .bitmap_start = BITMAP_BLOCK, // Assuming block 0 is the superblock
        .inode_start = INODE_START_BLOCK, // Next block for inodes
        .data_start = DATA_START_BLOCK // First data block
    };

    fseek(fp, 0, SEEK_SET);
    fwrite(&sb, sizeof(SuperBlock), 1, fp);

    // Clear bitmap (block 1)
    char bitmap[BLOCK_SIZE] = {0};
    // Reserve the first data block for root directory
    int root_data_block = sb.data_start;
    int root_block_index = root_data_block - sb.data_start;

    bitmap[root_block_index / 8] |= (1 << (root_block_index % 8));
    fseek(fp, BLOCK_SIZE * sb.bitmap_start, SEEK_SET);
    fwrite(bitmap, 1, BLOCK_SIZE, fp);

    // Zero out inode table (blocks 2–10)
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
        .owner_id = 150240719 // Eren Özer - Student ID
    };

    for (int i = 1; i < 4; i++) {
        root_inode.direct_blocks[i] = -1;
    }

    fseek(fp, BLOCK_SIZE * sb.inode_start, SEEK_SET);
    fwrite(&root_inode, sizeof(Inode), 1, fp); // This is the root inode, inode 0
    

    fclose(fp);
}


int readBlock(FILE *fp, int block_index, void *buf) {
    fseek(fp, block_index * BLOCK_SIZE, SEEK_SET);
    return fread(buf, 1, BLOCK_SIZE, fp) == BLOCK_SIZE ? 0 : -1;
}

int writeBlock(FILE *fp, int block_index, const void *buf) {
    fseek(fp, block_index * BLOCK_SIZE, SEEK_SET);
    return fwrite(buf, 1, BLOCK_SIZE, fp) == BLOCK_SIZE ? 0 : -1;
}

// -------- Bitmap Operations --------
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

void freeDataBlock(FILE *fp, int block_index) {
    int rel_index = block_index - DATA_START_BLOCK;
    uint8_t bitmap[BLOCK_SIZE];
    fseek(fp, BITMAP_BLOCK * BLOCK_SIZE, SEEK_SET);
    fread(bitmap, 1, BLOCK_SIZE, fp);
    bitmap[rel_index / 8] &= ~(1 << (rel_index % 8));
    fseek(fp, BITMAP_BLOCK * BLOCK_SIZE, SEEK_SET);
    fwrite(bitmap, 1, BLOCK_SIZE, fp);
}

// -------- Inode Operations --------
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

void freeInode(FILE *fp, int inode_index) {
    Inode inode = {0};
    fseek(fp, (INODE_START_BLOCK * BLOCK_SIZE) + inode_index * sizeof(Inode), SEEK_SET);
    fwrite(&inode, sizeof(Inode), 1, fp);
}

int readInode(FILE *fp, int inode_index, Inode *out) {
    fseek(fp, (INODE_START_BLOCK * BLOCK_SIZE) + inode_index * sizeof(Inode), SEEK_SET);
    return fread(out, sizeof(Inode), 1, fp) == 1 ? 0 : -1;
}

int writeInode(FILE *fp, int inode_index, const Inode *in) {
    fseek(fp, (INODE_START_BLOCK * BLOCK_SIZE) + inode_index * sizeof(Inode), SEEK_SET);
    return fwrite(in, sizeof(Inode), 1, fp) == 1 ? 0 : -1;
}

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
            return -1; // entry not found yet
        }
        prev_inode = current_inode;
        current_inode = findDirEntry(fp, current_inode, token);
        if (current_inode == -1) return -1;
        token = next;
    }
    return current_inode;
}

int findDirEntry(FILE *fp, int dir_inode_index, const char *name) {
    Inode dir_inode;
    if (readInode(fp, dir_inode_index, &dir_inode) != 0 || !dir_inode.is_directory) return -1;

    DirectoryEntry entries[MAX_DIR_ENTRIES];
    for (int i = 0; i < 4; i++) {
        if (dir_inode.direct_blocks[i] == -1) continue;
        readBlock(fp, dir_inode.direct_blocks[i], entries);
        for (int j = 0; j < MAX_DIR_ENTRIES; j++) {
            if (entries[j].inode_number != -1 && strcmp(entries[j].name, name) == 0) {
                return entries[j].inode_number;
            }
        }
    }
    return -1;
}

int addDirEntry(FILE *fp, int dir_inode_index, const char *name, int inode_index) {
    Inode dir_inode;
    if (readInode(fp, dir_inode_index, &dir_inode) != 0 || !dir_inode.is_directory) return -1;

    DirectoryEntry entries[MAX_DIR_ENTRIES];
    for (int i = 0; i < 4; i++) {
        if (dir_inode.direct_blocks[i] == -1) {
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
        readBlock(fp, dir_inode.direct_blocks[i], entries);
        for (int j = 0; j < MAX_DIR_ENTRIES; j++) {
            if (entries[j].inode_number == -1) {
                strncpy(entries[j].name, name, 27);
                entries[j].name[27] = '\0';
                entries[j].inode_number = inode_index;
                writeBlock(fp, dir_inode.direct_blocks[i], entries);
                dir_inode.size++;
                writeInode(fp, dir_inode_index, &dir_inode);
                return 0;
            }
        }
    }
    return -1; // Directory full
}

int removeDirEntry(FILE *fp, int dir_inode_index, const char *name) {
    Inode dir_inode;
    if (readInode(fp, dir_inode_index, &dir_inode) != 0 || !dir_inode.is_directory) return -1;

    DirectoryEntry entries[MAX_DIR_ENTRIES];
    for (int i = 0; i < 4; i++) {
        if (dir_inode.direct_blocks[i] == -1) continue;
        readBlock(fp, dir_inode.direct_blocks[i], entries);
        for (int j = 0; j < MAX_DIR_ENTRIES; j++) {
            if (entries[j].inode_number != -1 && strcmp(entries[j].name, name) == 0) {
                entries[j].inode_number = -1;
                entries[j].name[0] = '\0';
                writeBlock(fp, dir_inode.direct_blocks[i], entries);
                dir_inode.size--;
                writeInode(fp, dir_inode_index, &dir_inode);
                return 0;
            }
        }
    }
    return -1; 
}
