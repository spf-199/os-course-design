#include <stdio.h>
#include <string.h>
#include "oslab.h"

#define BLOCK_SIZE 32
#define BLOCK_COUNT 64
#define MAX_FILES 16
#define MAX_NAME 24
#define DIRECT_BLOCKS 8

typedef struct {
    int used;
    char name[MAX_NAME];
    int size;
    int blocks[DIRECT_BLOCKS];
    int block_count;
} FileEntry;

typedef struct {
    unsigned char data[BLOCK_COUNT][BLOCK_SIZE];
    int bitmap[BLOCK_COUNT];
    FileEntry files[MAX_FILES];
} MiniFS;

static void fs_init(MiniFS *fs) {
    memset(fs, 0, sizeof(*fs));
    for (int i = 0; i < MAX_FILES; ++i) {
        for (int j = 0; j < DIRECT_BLOCKS; ++j) fs->files[i].blocks[j] = -1;
    }
}

static int fs_find_file(MiniFS *fs, const char *name) {
    for (int i = 0; i < MAX_FILES; ++i) {
        if (fs->files[i].used && strcmp(fs->files[i].name, name) == 0) return i;
    }
    return -1;
}

static int fs_find_free_entry(MiniFS *fs) {
    for (int i = 0; i < MAX_FILES; ++i) {
        if (!fs->files[i].used) return i;
    }
    return -1;
}

static int fs_alloc_block(MiniFS *fs) {
    for (int i = 0; i < BLOCK_COUNT; ++i) {
        if (!fs->bitmap[i]) {
            fs->bitmap[i] = 1;
            memset(fs->data[i], 0, BLOCK_SIZE);
            return i;
        }
    }
    return -1;
}

static void fs_free_blocks(MiniFS *fs, FileEntry *file) {
    for (int i = 0; i < file->block_count; ++i) {
        if (file->blocks[i] >= 0) fs->bitmap[file->blocks[i]] = 0;
        file->blocks[i] = -1;
    }
    file->block_count = 0;
    file->size = 0;
}

static int fs_create(MiniFS *fs, const char *name) {
    if (fs_find_file(fs, name) >= 0) return 0;
    int idx = fs_find_free_entry(fs);
    if (idx < 0) return 0;
    fs->files[idx].used = 1;
    strncpy(fs->files[idx].name, name, MAX_NAME - 1);
    fs->files[idx].name[MAX_NAME - 1] = '\0';
    fs->files[idx].size = 0;
    fs->files[idx].block_count = 0;
    for (int i = 0; i < DIRECT_BLOCKS; ++i) fs->files[idx].blocks[i] = -1;
    return 1;
}

static int fs_write(MiniFS *fs, const char *name, const char *content) {
    int idx = fs_find_file(fs, name);
    if (idx < 0) return 0;
    FileEntry *file = &fs->files[idx];
    int len = (int)strlen(content);
    int need = (len + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if (need > DIRECT_BLOCKS) return 0;

    fs_free_blocks(fs, file);
    for (int i = 0; i < need; ++i) {
        int block = fs_alloc_block(fs);
        if (block < 0) {
            fs_free_blocks(fs, file);
            return 0;
        }
        file->blocks[file->block_count++] = block;
        int remain = len - i * BLOCK_SIZE;
        int copy = remain < BLOCK_SIZE ? remain : BLOCK_SIZE;
        memcpy(fs->data[block], content + i * BLOCK_SIZE, (size_t)copy);
    }
    file->size = len;
    return 1;
}

static int fs_read(MiniFS *fs, const char *name, char *out, int cap) {
    int idx = fs_find_file(fs, name);
    if (idx < 0 || cap <= 0) return 0;
    FileEntry *file = &fs->files[idx];
    int pos = 0;
    for (int i = 0; i < file->block_count && pos < cap - 1; ++i) {
        int remain = file->size - i * BLOCK_SIZE;
        int copy = remain < BLOCK_SIZE ? remain : BLOCK_SIZE;
        if (copy > cap - 1 - pos) copy = cap - 1 - pos;
        memcpy(out + pos, fs->data[file->blocks[i]], (size_t)copy);
        pos += copy;
    }
    out[pos] = '\0';
    return 1;
}

static int fs_delete(MiniFS *fs, const char *name) {
    int idx = fs_find_file(fs, name);
    if (idx < 0) return 0;
    fs_free_blocks(fs, &fs->files[idx]);
    memset(&fs->files[idx], 0, sizeof(fs->files[idx]));
    for (int i = 0; i < DIRECT_BLOCKS; ++i) fs->files[idx].blocks[i] = -1;
    return 1;
}

static void fs_ls(MiniFS *fs) {
    int free_blocks = 0;
    puts("Name                    Size  Blocks");
    for (int i = 0; i < MAX_FILES; ++i) {
        if (!fs->files[i].used) continue;
        printf("%-23s %-5d ", fs->files[i].name, fs->files[i].size);
        for (int j = 0; j < fs->files[i].block_count; ++j) printf("%d ", fs->files[i].blocks[j]);
        puts("");
    }
    for (int i = 0; i < BLOCK_COUNT; ++i) {
        if (!fs->bitmap[i]) free_blocks++;
    }
    printf("Free blocks: %d/%d\n", free_blocks, BLOCK_COUNT);
}

static void fs_print_bitmap(MiniFS *fs) {
    printf("Bitmap: ");
    for (int i = 0; i < BLOCK_COUNT; ++i) {
        printf("%d", fs->bitmap[i]);
        if ((i + 1) % 8 == 0) printf(" ");
    }
    puts("");
}

void filesystem_demo(void) {
    MiniFS fs;
    char out[256];
    fs_init(&fs);
    puts("\n==== File System Demo ====");
    printf("create notes: %s\n", fs_create(&fs, "notes") ? "OK" : "FAIL");
    printf("write notes: %s\n", fs_write(&fs, "notes", "Operating systems manage CPU, memory, processes and files.") ? "OK" : "FAIL");
    printf("create log: %s\n", fs_create(&fs, "log") ? "OK" : "FAIL");
    printf("write log: %s\n", fs_write(&fs, "log", "block bitmap allocation demo") ? "OK" : "FAIL");
    fs_ls(&fs);
    fs_print_bitmap(&fs);
    if (fs_read(&fs, "notes", out, sizeof(out))) printf("read notes: %s\n", out);
    printf("delete log: %s\n", fs_delete(&fs, "log") ? "OK" : "FAIL");
    fs_ls(&fs);
    fs_print_bitmap(&fs);
    puts("\nAnalysis: the directory table stores metadata, the bitmap records free blocks, and read/write maps logical file offsets to direct data blocks.");
}

void filesystem_menu(void) {
    MiniFS fs;
    fs_init(&fs);
    puts("\n==== Mini File System ====");
    for (;;) {
        int op;
        char name[MAX_NAME];
        char content[256];
        char out[256];
        printf("op 1=create 2=write 3=read 4=delete 5=ls 6=bitmap 0=quit: ");
        if (scanf("%d", &op) != 1 || op == 0) break;
        if (op == 1) {
            printf("name: ");
            scanf("%23s", name);
            printf("%s\n", fs_create(&fs, name) ? "OK" : "FAIL");
        } else if (op == 2) {
            printf("name: ");
            scanf("%23s", name);
            printf("content(no spaces): ");
            scanf("%255s", content);
            printf("%s\n", fs_write(&fs, name, content) ? "OK" : "FAIL");
        } else if (op == 3) {
            printf("name: ");
            scanf("%23s", name);
            if (fs_read(&fs, name, out, sizeof(out))) printf("%s\n", out);
            else puts("FAIL");
        } else if (op == 4) {
            printf("name: ");
            scanf("%23s", name);
            printf("%s\n", fs_delete(&fs, name) ? "OK" : "FAIL");
        } else if (op == 5) {
            fs_ls(&fs);
        } else if (op == 6) {
            fs_print_bitmap(&fs);
        }
    }
}

int filesystem_selftest(void) {
    MiniFS fs;
    char out[64];
    fs_init(&fs);
    return fs_create(&fs, "a") && fs_write(&fs, "a", "hello") &&
           fs_read(&fs, "a", out, sizeof(out)) && strcmp(out, "hello") == 0 &&
           fs_delete(&fs, "a");
}
