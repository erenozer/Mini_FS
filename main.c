#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fs.h"
#include "disk.h"

int main(int argc, char *argv[]) {
    if (argc == 1) {
        /* Example sequence:
        • Create a directory.
        • Create/write a file.
        • Read the file.
        • List directory contents.
        • Delete file and directory.
        • Demonstrate bitmap/inode reuse.
        */

        mkfs("disk.img");
        printf("Disk formatted successfully.\n");

        if (mkdir_fs("/kovan") == 0) {
            printf("Directory /kovan created successfully.\n");
        }

        if (create_fs("/kovan/hey.txt") == 0) {
            printf("File /kovan/hey.txt created successfully.\n");
        }

        if (write_fs("/kovan/hey.txt", "Operating Systems - MiniFS Project") >= 0) {
            printf("Data written to /kovan/hey.txt successfully.\n");
        }

        char buf[BLOCK_SIZE * 4] = {0};
        int bytes = read_fs("/kovan/hey.txt", buf, sizeof(buf));
        if (bytes >= 0) {
            printf("Contents of /kovan/hey.txt:\n\"%s\"\n", buf);
        }

        DirectoryEntry entries[10];
        int count = ls_fs("/kovan", entries, 10);
        if (count >= 0) {
            printf("Contents of /kovan:\n");
            for (int i = 0; i < count; ++i) {
                printf("%s\n", entries[i].name);
            }
        }
        
        // Delete the contents of /kovan
        if (delete_fs("/kovan/hey.txt") == 0) {
            printf("File /kovan/hey.txt deleted successfully.\n");
        }

        if (rmdir_fs("/kovan") == 0) {
            printf("Directory /kovan removed successfully.\n");
        }

        if (create_fs("/newfile.txt") == 0) {
            if (write_fs("/newfile.txt", "Reusing freed blocks/inodes.") >= 0) {
                printf("File /newfile.txt created successfully.\n");
            }

            char buffer[BLOCK_SIZE * 4] = {0};
            int readNew = read_fs("/newfile.txt", buffer, sizeof(buffer));
            if (readNew >= 0) {
                printf("Contents of /newfile.txt:\n\"%s\"\n", buffer);
            }
        }

        printf("Example main sequence finished.\n");

    } else {
        // Interface for MiniFS that handles from terminal directly
        const char *cmd = argv[1];

        if (strcmp(cmd, "mkfs") == 0 && argc == 2) {
            mkfs("disk.img");
            printf("Disk formatted successfully.\n");
            return 0;
        } else if (strcmp(cmd, "mkdir_fs") == 0 && argc == 3) {
            if (mkdir_fs(argv[2]) == 0) {
                printf("Directory %s created successfully.\n", argv[2]);
                return 0;
            } else return 1;
        } else if (strcmp(cmd, "create_fs") == 0 && argc == 3) {
            if (create_fs(argv[2]) == 0) {
                printf("File %s created successfully.\n", argv[2]);
                return 0;
            } else return 1;
        } else if (strcmp(cmd, "write_fs") == 0 && argc == 4) {
            if (write_fs(argv[2], argv[3]) >= 0) {
                printf("Data written to %s successfully.\n", argv[2]);
                return 0;
            } else return 1;
        } else if (strcmp(cmd, "read_fs") == 0 && argc == 3) {
            char buf[BLOCK_SIZE * 4] = {0};
            int bytes = read_fs(argv[2], buf, sizeof(buf) - 1);
            if (bytes >= 0) {
                buf[bytes] = '\0';
                printf("%s\n", buf);
                return 0;
            } else return 1;
        } else if (strcmp(cmd, "delete_fs") == 0 && argc == 3) {
            if (delete_fs(argv[2]) == 0) {
                printf("File %s deleted successfully.\n", argv[2]);
                return 0;
            } else return 1;
        } else if (strcmp(cmd, "rmdir_fs") == 0 && argc == 3) {
            if (rmdir_fs(argv[2]) == 0) {
                printf("Directory %s removed successfully.\n", argv[2]);
                return 0;
            } else return 1;
        } else if (strcmp(cmd, "ls_fs") == 0 && argc == 3) {
            DirectoryEntry entries[BLOCK_SIZE / sizeof(DirectoryEntry)] = {0};
            int max_entries = sizeof(entries) / sizeof(entries[0]);
            int count = ls_fs(argv[2], entries, max_entries);
            if (count >= 0) {
                for (int i = 0; i < count; ++i) {
                    printf("%s\n", entries[i].name);
                }
                return 0;
            } else return 1;
        } else {
            fprintf(stderr, "Error: Unknown command or syntax usage.\n");
            return 1;
        }
    }
}
