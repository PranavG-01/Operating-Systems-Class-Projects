#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ext2fs/ext2fs.h>
#include <sys/stat.h>


//list of functions
void printInfo(char *disk);
void printRoot(char *disk);
void printDir(char *disk);
void printFileContent(char *disk, char *path);
void traverseDir(int fd, struct ext2_super_block *SB, struct ext2_group_desc *GD, struct ext2_inode *inode, char *currentPath);


void printInfo(char *disk) {
    struct ext2_super_block SB;

    int fd = open(disk, O_RDONLY);
    //reading the superblock within the disk image

    unsigned int blockSize = 1024 << SB.s_log_block_size;
    unsigned int totalBlocks = SB.s_blocks_count;
    unsigned long long diskSize = (unsigned long long)blockSize * totalBlocks;
    unsigned int inodeSize = SB.s_inode_size;
    unsigned int totalInodes = SB.s_inodes_count;
    unsigned int numberOfGroups = (totalBlocks + SB.s_blocks_per_group - 1) / SB.s_blocks_per_group;


    printf("--General File System Information--\n");
    printf("Block Size in Bytes: %u\n", blockSize);
    printf("Total Number of Blocks: %u\n", totalBlocks);
    printf("Disk Size in Bytes: %llu\n", diskSize);
    printf("Maximum Number of Blocks Per Group: %u\n", blockSize * 8);
    printf("Inode Size in Bytes: %u\n", inodeSize);
    printf("Number of Inodes Per Group: %u\n", totalInodes);
    printf("Number of Inode Blocks Per Group: %u\n", (inodeSize * totalInodes / blockSize));
    printf("Number of Groups: %u\n", numberOfGroups);

    printf("--Individual Group Information--\n");
    for (unsigned int group = 0; group < numberOfGroups; group ++) {
        struct ext2_group_desc GD;
        
        //pointing to the group descriptor table
        lseek(fd, 1024 + blockSize + group * sizeof(GD), SEEK_SET);

        unsigned int firstBlock = SB.s_first_data_block + group * (blockSize * 8);
        unsigned int lastBlock = firstBlock + blockSize * 8 - 1;
        unsigned int blockMap = GD.bg_block_bitmap;
        unsigned int inodeMap = GD.bg_inode_bitmap;
        unsigned int inodeTable = GD.bg_inode_table;
        unsigned int freeBlocks = GD.bg_free_blocks_count;
        unsigned int freeInodes = GD.bg_free_inodes_count;
        unsigned int numberOfDirectories = GD.bg_used_dirs_count;


        printf("-Group %u-\n", group);
        printf("Block IDs: %u - %u\n", firstBlock, lastBlock);
        printf("Block Bitmap Block ID: %u\n", blockMap);
        printf("Inode Bitmap Block ID: %u\n", inodeMap);
        printf("Inode Table Block IDs: %u\n", inodeTable);
        printf("Number of Free Blocks: %u\n", freeBlocks);
        printf("Number of Free Inodes: %u\n", freeInodes);
        printf("Number of Directories: %u\n", numberOfDirectories);
        printf("Free Block IDs: ");
        //looping through bitmap to find all the free blocks and inodes
        //but how to print them in ranges instead of one-by-one?
            //we will print the list directly in the for loop, and use a while loop if there is a range
        //pointing to the block bitmap
        lseek(fd, blockMap * blockSize, SEEK_SET);
        unsigned char *bitMap = malloc(blockSize);
        //In C, you cannot directly index a bit, so you need to read byte by byte
        //and then check each bit within the byte
        read(fd, bitMap, blockSize);
        for (unsigned int byte = 0; byte < blockSize/8; byte++) {
            unsigned char i = bitMap[byte];
            for (int j = 0; j < 8; j++) {
                unsigned int valueOfBit = (i >> j) & 1;

                //print the first free block
                if (valueOfBit == 0 && ((i >> (j+1))&1) == 1) {
                    //j and i are just numbers you can't reference them directly within the bitmap array
                    unsigned int bitIndex = firstBlock + byte * 8 + j;
                    printf("%u, ", bitIndex);
                }
                if (valueOfBit == 0 && ((i >> (j+1))&1) == 0) {
                    unsigned int bitIndex = firstBlock + byte * 8 + j;
                    printf("%u-", bitIndex);
                    while (((i >> (j+1))&1) == 0) {
                        j++;
                    }
                    bitIndex = firstBlock + byte * 8 + j;
                    printf("%u, ", bitIndex);
                }

            }
        }
        free(bitMap);

        printf("\n");
        //repeaing the loop from free blocks, for free inodes
        printf("Free Inode IDs: " );

        lseek(fd, inodeMap * blockSize, SEEK_SET);
        unsigned char *inodeBitMap = malloc(blockSize);
        read(fd, inodeBitMap, blockSize);
        for (unsigned int byte = 0; byte < blockSize/8; byte++) {
            unsigned char i = inodeBitMap[byte];
           for (int j =0; j < 8; j++ ) {
                unsigned int valueOfBit = (i >> j)&1;

                if (valueOfBit == 0 && ((i >> (j+1))&1) == 1) {
                    //j and i are just numbers you can't reference them directly within the bitmap array
                    unsigned int bitIndex = firstBlock + byte * 8 + j;
                    printf("%u, ", bitIndex);;
                }
                if (valueOfBit == 0 && ((i >> (j+1))&1) == 0) {
                    unsigned int bitIndex = firstBlock + byte * 8 + j;
                    printf("%u-", bitIndex);
                    while (((i >> (j+1))&1) == 0) {
                        j++;
                    }
                    bitIndex = firstBlock + byte * 8 + j;
                    printf("%u, ", bitIndex);
                }
            }
        }
        free(inodeBitMap);
        printf("\n");
    }
    
    close(fd);
}

void printRoot(char *disk) {
    printf("--Root Directory Entries--\n");
    int fd = open(disk, O_RDONLY);

    struct ext2_super_block SB;
    lseek(fd, 1024, SEEK_SET);
    read(fd, &SB, sizeof(SB));
    unsigned int blockSize = 1024 << SB.s_log_block_size;
    unsigned int inodeSize = SB.s_inode_size;

    struct ext2_group_desc GD;
    lseek(fd, 1024 + blockSize, SEEK_SET);
    read(fd, &GD, sizeof(GD));
    unsigned int inodeTable = GD.bg_inode_table;

    unsigned int rootInode = inodeTable * blockSize + inodeSize;
    lseek(fd, rootInode, SEEK_SET);
    struct ext2_inode root;
    read(fd, &root, sizeof(root));

    unsigned int firstBlock = root.i_block[0];
    unsigned char *block = malloc(blockSize);
    lseek(fd, firstBlock * blockSize, SEEK_SET);
    read(fd, block, blockSize);

    int i = 0;
    while (i < blockSize) {
        struct ext2_dir_entry_2 *iter = (struct ext2_dir_entry_2 *)(block + i);

        if (iter -> inode == 0) {
            break;
        }

        printf("Inode: %u\n", iter->inode);
        printf("Entry Length: %u\n", iter->rec_len);
        printf("Name Length: %u\n", iter->name_len);
        printf("File Type: %u\n", iter->file_type);
        printf("Name: %.*s\n", iter->name_len, iter->name);

    }

    free(block);
    close(fd);

}

void printDir(char *disk) {
    //cannot just access the files like within a regular folder
    //have to go through each inode, then read the inode's blocks
    printf("/");

    int fd = open(disk, O_RDONLY);
    struct ext2_super_block SB;
    lseek(fd, 1024, SEEK_SET);
    read(fd, &SB, sizeof(SB));
    unsigned int blockSize = 1024 << SB.s_log_block_size;


    struct ext2_group_desc GD;
    off_t gd_offset = (blockSize == 1024) ? 2048 : blockSize;
    lseek(fd, 1024 + blockSize, SEEK_SET);
    read(fd, &GD, sizeof(GD));
    unsigned int inodeTable = GD.bg_inode_table;
    unsigned int inodeSize = SB.s_inode_size;

    struct ext2_inode inodeRoot;
    off_t rootInodeOffset = (off_t)inodeTable * blockSize + inodeSize;
    lseek(fd, rootInodeOffset, SEEK_SET);
    read(fd, &inodeRoot, sizeof(inodeRoot));

    traverseDir(fd, &SB, &GD, &inodeRoot, "/");

    close(fd);
}

void traverseDir(int fd, struct ext2_super_block *SB, struct ext2_group_desc *GD, struct ext2_inode *inode, char *currentPath){
    unsigned int blockSize = 1024 << SB->s_log_block_size;

    // Loop through all 12 direct blocks (EXT2 has 12 direct block pointers)
    for (int b = 0; b < 12; b++) {

        if (inode->i_block[b] == 0)
            continue;

        unsigned int blockNum = inode->i_block[b];
        unsigned char *block = malloc(blockSize);

        lseek(fd, (off_t)blockNum * blockSize, SEEK_SET);
        read(fd, block, blockSize);

        unsigned int offset = 0;

        while (offset < blockSize) {

            struct ext2_dir_entry_2 *entry = (struct ext2_dir_entry_2 *)(block + offset);

            // invalid entry → stop block
            if (entry->inode == 0)
                break;

            // extract name
            char name[256];
            memcpy(name, entry->name, entry->name_len);
            name[entry->name_len] = '\0';

            // Skip "." , ".." , and hidden files
            if (name[0] != '.') {

                char newPath[1024];
                if (strcmp(currentPath, "/") == 0)
                    sprintf(newPath, "/%s", name);
                else
                    sprintf(newPath, "%s/%s", currentPath, name);

                printf("%s\n", newPath);

                // ---------- RECURSE INTO DIRECTORIES ----------
                if (entry->file_type == EXT2_FT_DIR) {
                    if (strcmp(name, ".") != 0 && strcmp(name, "..") != 0) {
                        struct ext2_inode child;
                        off_t childOffset = (off_t)GD->bg_inode_table * blockSize + (entry->inode - 1) * SB->s_inode_size;
                        if (lseek(fd, childOffset, SEEK_SET) == -1) { perror("lseek child inode"); free(block); return; }
                        if (read(fd, &child, SB->s_inode_size) != SB->s_inode_size) { perror("read child inode"); free(block); return; }

                        traverseDir(fd, SB, GD, &child, newPath);
                    }
                }
            }

            offset += entry->rec_len;
        }

        free(block);
    }
}


void printFileContent(char *disk, char *path) {
    int fd = open(disk, O_RDONLY);

    struct ext2_super_block SB;
    lseek(fd, 1024, SEEK_SET);
    read(fd, &SB, sizeof(SB));
    unsigned int blockSize = 1024 << SB.s_log_block_size;

    struct ext2_group_desc GD;
    lseek(fd, 1024 + blockSize, SEEK_SET);
    read(fd, &GD, sizeof(GD));
    unsigned int inodeTable = GD.bg_inode_table;

    struct ext2_inode inode;
    off_t rootOffset = inodeTable * blockSize + SB.s_inode_size;
    lseek(fd, rootOffset, SEEK_SET);
    read(fd, &inode, sizeof(inode));

    char temp[1024];
    strcpy(temp, path);

    char *token = strtok(temp, "/");  

    while (token != NULL) {

        int found = 0;
        
        for (int b = 0; b < 12 && inode.i_block[b] != 0; b++) {
            unsigned int block = inode.i_block[b];
            unsigned char *data = malloc(blockSize);

            lseek(fd, block * blockSize, SEEK_SET);
            read(fd, data, blockSize);

            unsigned int i = 0;
            while (i < blockSize) {
                struct ext2_dir_entry_2 *entry =
                    (struct ext2_dir_entry_2 *)(data + i);

                if (entry->inode == 0)
                    break;

                char name[256];
                memcpy(name, entry->name, entry->name_len);
                name[entry->name_len] = '\0';

                if (strcmp(name, token) == 0) {
                    off_t inodeOffset =
                        inodeTable * blockSize +
                        (entry->inode - 1) * SB.s_inode_size;

                    lseek(fd, inodeOffset, SEEK_SET);
                    read(fd, &inode, sizeof(inode));
                    found = 1;
                    break;
                }

                i += entry->rec_len;
            }

            free(data);
            if (found) break;
        }

        if (!found) {
            printf("File or directory not found: %s\n", token);
            close(fd);
            return;
        }

        // Get next path component
        token = strtok(NULL, "/");
    }

    if (!S_ISREG(inode.i_mode)) {
        printf("Not a regular file.\n");
        close(fd);
        return;
    }

    unsigned int remaining = inode.i_size;

    for (int b = 0; b < 12 && inode.i_block[b] != 0 && remaining > 0; b++) {
        unsigned int block = inode.i_block[b];

        unsigned int toRead = remaining < blockSize ? remaining : blockSize;

        unsigned char *buf = malloc(blockSize);
        lseek(fd, block * blockSize, SEEK_SET);
        read(fd, buf, blockSize);

        fwrite(buf, 1, toRead, stdout);
        free(buf);

        remaining -= toRead;
    }

    close(fd);

}


int main(int argc, char *argv[]) {

    if (argc == 3 && strcmp(argv[2], "-info") == 0) {
        printInfo(argv[1]);
        return 0;
    }
    else if (argc == 3 && strcmp(argv[2], "-root") == 0) {
        printRoot(argv[1]);
        return 0;
    }
    else if (argc == 3 && strcmp(argv[2], "-traverse") == 0) {
        printDir(argv[1]);
        return 0;
    }
    else if (argc == 4 && strcmp(argv[2], "-file") == 0) {
        printFileContent(argv[1], argv[3]);
        return 0;
    }
    else {
        fprintf(stderr, "Usage: %s <directory> [-info|-root|-traverse|absolute-path]\n", argv[1]);
        return 1;
    }

}
