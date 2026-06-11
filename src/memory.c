#include <stdio.h>
#include <string.h>
#include "oslab.h"

#define MAX_PART 32
#define MAX_PAGES 128
#define MAX_FRAMES 16

typedef struct {
    int start;
    int size;
    int pid;
} Partition;

static void print_partitions(Partition *parts, int count) {
    puts("Start  Size  State");
    for (int i = 0; i < count; ++i) {
        if (parts[i].pid == -1) {
            printf("%-6d %-5d Free\n", parts[i].start, parts[i].size);
        } else {
            printf("%-6d %-5d P%d\n", parts[i].start, parts[i].size, parts[i].pid);
        }
    }
}

static int allocate_partition(Partition *parts, int *count, int pid, int size, int best_fit) {
    int idx = -1;
    for (int i = 0; i < *count; ++i) {
        if (parts[i].pid != -1 || parts[i].size < size) continue;
        if (idx == -1 || (!best_fit) || parts[i].size < parts[idx].size) {
            idx = i;
            if (!best_fit) break;
        }
    }
    if (idx == -1) return 0;
    if (parts[idx].size == size) {
        parts[idx].pid = pid;
    } else if (*count < MAX_PART) {
        for (int j = *count; j > idx + 1; --j) parts[j] = parts[j - 1];
        parts[idx + 1].start = parts[idx].start + size;
        parts[idx + 1].size = parts[idx].size - size;
        parts[idx + 1].pid = -1;
        parts[idx].size = size;
        parts[idx].pid = pid;
        (*count)++;
    } else {
        return 0;
    }
    return 1;
}

static void merge_free(Partition *parts, int *count) {
    for (int i = 0; i + 1 < *count;) {
        if (parts[i].pid == -1 && parts[i + 1].pid == -1) {
            parts[i].size += parts[i + 1].size;
            for (int j = i + 1; j + 1 < *count; ++j) parts[j] = parts[j + 1];
            (*count)--;
        } else {
            i++;
        }
    }
}

static int free_partition(Partition *parts, int *count, int pid) {
    for (int i = 0; i < *count; ++i) {
        if (parts[i].pid == pid) {
            parts[i].pid = -1;
            merge_free(parts, count);
            return 1;
        }
    }
    return 0;
}

static void partition_demo(int best_fit) {
    Partition parts[MAX_PART] = {{0, 256, -1}};
    int count = 1;
    int ops[][3] = {
        {1, 1, 64}, {1, 2, 32}, {1, 3, 80}, {0, 2, 0},
        {1, 4, 24}, {0, 1, 0}, {1, 5, 48}
    };
    printf("\nDynamic partition demo (%s)\n", best_fit ? "Best Fit" : "First Fit");
    for (int i = 0; i < 7; ++i) {
        if (ops[i][0]) {
            printf("Allocate P%d size %d: %s\n", ops[i][1], ops[i][2],
                   allocate_partition(parts, &count, ops[i][1], ops[i][2], best_fit) ? "OK" : "FAIL");
        } else {
            printf("Free P%d: %s\n", ops[i][1],
                   free_partition(parts, &count, ops[i][1]) ? "OK" : "FAIL");
        }
        print_partitions(parts, count);
        puts("");
    }
}

static int page_index(int *frames, int frame_count, int page) {
    for (int i = 0; i < frame_count; ++i) {
        if (frames[i] == page) return i;
    }
    return -1;
}

static void print_frames(int *frames, int frame_count) {
    printf("[");
    for (int i = 0; i < frame_count; ++i) {
        if (frames[i] == -1) printf(" -");
        else printf(" %d", frames[i]);
    }
    printf(" ]");
}

static int simulate_pages(const int *refs, int ref_count, int frame_count, int lru, int verbose) {
    int frames[MAX_FRAMES];
    int last_used[MAX_FRAMES];
    int next_fifo = 0;
    int faults = 0;
    for (int i = 0; i < frame_count; ++i) {
        frames[i] = -1;
        last_used[i] = -1;
    }
    for (int t = 0; t < ref_count; ++t) {
        int page = refs[t];
        int hit = page_index(frames, frame_count, page);
        if (hit >= 0) {
            last_used[hit] = t;
            if (verbose) {
                printf("ref %-2d hit   ", page);
                print_frames(frames, frame_count);
                puts("");
            }
            continue;
        }
        faults++;
        int victim = page_index(frames, frame_count, -1);
        if (victim < 0 && lru) {
            victim = 0;
            for (int i = 1; i < frame_count; ++i) {
                if (last_used[i] < last_used[victim]) victim = i;
            }
        } else if (victim < 0) {
            victim = next_fifo;
            next_fifo = (next_fifo + 1) % frame_count;
        }
        frames[victim] = page;
        last_used[victim] = t;
        if (verbose) {
            printf("ref %-2d fault ", page);
            print_frames(frames, frame_count);
            puts("");
        }
    }
    if (verbose) {
        printf("%s faults: %d, fault rate: %.2f%%\n",
               lru ? "LRU" : "FIFO", faults, 100.0 * faults / ref_count);
    }
    return faults;
}

static void page_demo(void) {
    int refs[] = {7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2};
    int n = (int)(sizeof(refs) / sizeof(refs[0]));
    puts("\nPage replacement demo, frames = 3");
    simulate_pages(refs, n, 3, 0, 1);
    simulate_pages(refs, n, 3, 1, 1);
}

void memory_demo(void) {
    puts("\n==== Memory Management Demo ====");
    partition_demo(0);
    partition_demo(1);
    page_demo();
    puts("\nAnalysis: First Fit is fast but may leave small fragments near low addresses; Best Fit reduces immediate waste but can create many tiny holes. FIFO is simple and may suffer Belady anomaly; LRU better reflects locality but needs extra access tracking.");
}

void memory_menu(void) {
    int choice;
    puts("\n==== Memory Management ====");
    puts("1. Dynamic partition interactive");
    puts("2. Page replacement interactive");
    puts("3. Demo");
    printf("Choose: ");
    if (scanf("%d", &choice) != 1) return;
    if (choice == 1) {
        Partition parts[MAX_PART] = {{0, 0, -1}};
        int count = 1, best_fit = 0, op = 0;
        printf("Total memory size: ");
        scanf("%d", &parts[0].size);
        printf("Algorithm 0=First Fit, 1=Best Fit: ");
        scanf("%d", &best_fit);
        while (1) {
            printf("op 1=alloc 2=free 0=quit: ");
            if (scanf("%d", &op) != 1 || op == 0) break;
            if (op == 1) {
                int pid, size;
                printf("pid size: ");
                scanf("%d %d", &pid, &size);
                printf("%s\n", allocate_partition(parts, &count, pid, size, best_fit) ? "OK" : "FAIL");
            } else if (op == 2) {
                int pid;
                printf("pid: ");
                scanf("%d", &pid);
                printf("%s\n", free_partition(parts, &count, pid) ? "OK" : "FAIL");
            }
            print_partitions(parts, count);
        }
    } else if (choice == 2) {
        int refs[MAX_PAGES], ref_count, frame_count;
        printf("Reference count: ");
        scanf("%d", &ref_count);
        if (ref_count <= 0 || ref_count > MAX_PAGES) return;
        printf("References: ");
        for (int i = 0; i < ref_count; ++i) scanf("%d", &refs[i]);
        printf("Frame count (1-%d): ", MAX_FRAMES);
        scanf("%d", &frame_count);
        if (frame_count <= 0 || frame_count > MAX_FRAMES) return;
        simulate_pages(refs, ref_count, frame_count, 0, 1);
        simulate_pages(refs, ref_count, frame_count, 1, 1);
    } else {
        memory_demo();
    }
}

int memory_selftest(void) {
    int refs[] = {1, 2, 3, 1, 4, 5};
    return simulate_pages(refs, 6, 3, 0, 0) == 5 && simulate_pages(refs, 6, 3, 1, 0) == 5;
}
