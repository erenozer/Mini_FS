#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fs.h"
#include "disk.h"


void mkfs(const char *diskfile) {
    FILE *fp = fopen(diskfile, "wb+");
    if (!fp) {
        fprintf(stderr, "Error: Unable to create disk image.\n");
        return;
    }

    // Step 1: Allocate 1MB space for disk
    char zero_block[BLOCK_SIZE] = {0};
    for (int i = 0; i < NUM_BLOCKS; i++) {
        fwrite(zero_block, 1, BLOCK_SIZE, fp);
    }

    SuperBlock sb = {
        .magic_number = MAGIC_NUMBER,
        .num_blocks = NUM_BLOCKS,
        .num_inodes = NUM_INODES,
        .bitmap_start = 1, // Assuming block 0 is the superblock
        .inode_start = 2, // Next block for inodes
        .data_start = 11 // First data block
    };

    fseek(fp, 0, SEEK_SET);
    fwrite(&sb, sizeof(SuperBlock), 1, fp);

    // Step 3: Clear bitmap (block 1)
    char bitmap[BLOCK_SIZE] = {0};
    // Reserve the first data block (11) for root directory
    int root_data_block = sb.data_start;
    int root_block_index = root_data_block - sb.data_start; // relative to data_start
    bitmap[root_block_index / 8] |= (1 << (root_block_index % 8));
    fseek(fp, BLOCK_SIZE * sb.bitmap_start, SEEK_SET);
    fwrite(bitmap, 1, BLOCK_SIZE, fp);

    // Step 4: Zero out inode table (blocks 2–10)
    Inode inode;
    memset(&inode, 0, sizeof(Inode));
    fseek(fp, BLOCK_SIZE * sb.inode_start, SEEK_SET);
    for (int i = 0; i < NUM_INODES; i++) {
        fwrite(&inode, sizeof(Inode), 1, fp);
    }

    // Step 5: Initialize root directory (inode 0)
    Inode root_inode;
    root_inode.is_valid = 1;
    root_inode.size = 0;
    root_inode.is_directory = 1;
    root_inode.direct_blocks[0] = root_data_block;
    for (int i = 1; i < 4; i++) {
        root_inode.direct_blocks[i] = -1;
    }
    root_inode.owner_id = 0; // You can use student ID if required

    fseek(fp, BLOCK_SIZE * sb.inode_start, SEEK_SET);
    fwrite(&root_inode, sizeof(Inode), 1, fp); // write as inode 0

    fclose(fp);
}