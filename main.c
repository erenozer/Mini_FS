/*
Provide in main.c a sequence to: 
• Create a directory.
• Create/write a file.
• Read the file.
• List directory contents.
• Delete file and directory.
• Demonstrate bitmap/inode reuse. Include run log.txt and screenshots/.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fs.h"
#include "disk.h"

int main(int argc, char *argv[]) {
    if (argc == 1) {
        // Demo program for explained sequence
        
        mkfs("disk.img");
        printf("Disk formatted successfully.\n");

        mkdir_fs("/demo");
        printf("Directory /demo created.\n");

        create_fs("/demo/hello.txt");
        printf("File /demo/hello.txt created.\n");

        DirectoryEntry entries[10];
        int count = ls_fs("/demo", entries, 10);
        printf("Contents of /demo:\n");
        for (int i = 0; i < count; ++i) {
            printf("%s\n", entries[i].name);
        }
        printf("\n");

        
        write_fs("/demo/hello.txt", "MiniFS says hi!");
        printf("Data written to /demo/hello.txt.\n");

        char buf[BLOCK_SIZE * 4] = {0};
        int bytes = read_fs("/demo/hello.txt", buf, sizeof(buf));
        if (bytes >= 0) {
            buf[bytes] = '\0'; // Ensure null termination
            printf("Contents of /demo/hello.txt: %s\n", buf);
        } else {
            fprintf(stderr, "Error reading file.\n");
        }

        write_fs("/demo/hello.txt", "ghost hi!");
        buf[BLOCK_SIZE * 4] = 0;
        bytes = read_fs("/demo/hello.txt", buf, sizeof(buf));
        if (bytes >= 0) {
            buf[bytes] = '\0'; // Ensure null termination
            printf("Contents of /demo/hello.txt: %s\n", buf);
        } else {
            fprintf(stderr, "Error reading file.\n");
        }
        /*

        DirectoryEntry entries[10];
        int count = ls_fs("/demo", entries, 10);
        printf("Contents of /demo:\n");
        for (int i = 0; i < count; ++i) {
            printf("- %s\n", entries[i].name);
        }

        delete_fs("/demo/hello.txt");
        printf("File /demo/hello.txt deleted.\n");

        rmdir_fs("/demo");
        printf("Directory /demo removed.\n");

        printf("Example main sequence finished.\n");
        */

    } // Else, handle command line arguments

    /*
    const char *cmd = argv[1];

    if (strcmp(cmd, "mkfs") == 0) {
        if (argc == 2) {
            mkfs("disk.img");
            return 0;      // Success for mkfs command
        } else {
            fprintf(stderr, "Usage: %s mkfs\n", argv[0]);
            return 1;
        }
    } else if (strcmp(cmd, "mkdir_fs") == 0) {
        if (argc == 3) {
            return mkdir_fs(argv[2]);
        } else {
            fprintf(stderr, "Usage: %s mkdir_fs <path>\n", argv[0]);
            return 1;
        }
    } else if (strcmp(cmd, "create_fs") == 0) {
        if (argc == 3) {
            return create_fs(argv[2]);
        } else {
            fprintf(stderr, "Usage: %s create_fs <path>\n", argv[0]);
            return 1;
        }
    } else if (strcmp(cmd, "write_fs") == 0) {
        if (argc == 4) {
            return write_fs(argv[2], argv[3]);
        } else {
            fprintf(stderr, "Usage: %s write_fs <path> <data>\n", argv[0]);
            return 1;
        }
    } else if (strcmp(cmd, "read_fs") == 0) {
        if (argc == 3) {
            char buf[BLOCK_SIZE * 4]; // Keep original buffer size
            memset(buf, 0, sizeof(buf)); // Initialize buffer
            // Read up to sizeof(buf) - 1 to leave space for null terminator
            int bytes_read = read_fs(argv[2], buf, sizeof(buf) - 1);
            if (bytes_read >= 0) {
                buf[bytes_read] = '\0'; // Ensure null termination
                printf("%s\n", buf);    // Print to stdout
                return 0;               // Exit with 0 on success
            } else {
                // read_fs should print its own error message to stderr
                return 1; // Exit with 1 on failure
            }
        } else {
            fprintf(stderr, "Usage: %s read_fs <path>\n", argv[0]);
            return 1;
        }
    } else if (strcmp(cmd, "delete_fs") == 0) {
        if (argc == 3) {
            return delete_fs(argv[2]);
        } else {
            fprintf(stderr, "Usage: %s delete_fs <path>\n", argv[0]);
            return 1;
        }
    } else if (strcmp(cmd, "rmdir_fs") == 0) {
        if (argc == 3) {
            return rmdir_fs(argv[2]);
        } else {
            fprintf(stderr, "Usage: %s rmdir_fs <path>\n", argv[0]);
            return 1;
        }
    } else if (strcmp(cmd, "ls_fs") == 0) {
        if (argc == 3) {
            DirectoryEntry entries[BLOCK_SIZE / sizeof(DirectoryEntry)]; // Keep original array size
            int max_entries = sizeof(entries) / sizeof(entries[0]);
            int count = ls_fs(argv[2], entries, max_entries);
            if (count >= 0) { // ls_fs returns number of entries, or <0 on error
                for (int i = 0; i < count; ++i) {
                    printf("%s/\n", entries[i].name); // Keep original printing format
                }
                return 0; // Success
            } else {
                // ls_fs should print its own error message to stderr
                return 1; // Exit with 1 on failure
            }
        } else {
            fprintf(stderr, "Usage: %s ls_fs <path>\n", argv[0]);
            return 1;
        }

    } else {
        fprintf(stderr, "Error: Unknown command '%s'.\n", cmd);
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        fprintf(stderr, "Available commands: mkfs, mkdir_fs, create_fs, write_fs, read_fs, delete_fs, rmdir_fs, ls_fs\n");
        return 1;
    }

    return 0;
    */

}
