#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "oslab.h"

#define MAX_PROC 64
#define MAX_SEG 512

typedef struct {
    char name[16];
    int arrival;
    int burst;
    int priority;
    int remaining;
    int start;
    int finish;
    int turnaround;
    int waiting;
} Process;

typedef struct {
    char name[16];
    int start;
    int end;
} Segment;

static void copy_processes(Process *dst, const Process *src, int n) {
    memcpy(dst, src, sizeof(Process) * (size_t)n);
    for (int i = 0; i < n; ++i) {
        dst[i].remaining = dst[i].burst;
        dst[i].start = -1;
        dst[i].finish = 0;
        dst[i].turnaround = 0;
        dst[i].waiting = 0;
    }
}

static void add_segment(Segment *segments, int *count, const char *name, int start, int end) {
    if (start == end || *count >= MAX_SEG) return;
    if (*count > 0 && strcmp(segments[*count - 1].name, name) == 0 && segments[*count - 1].end == start) {
        segments[*count - 1].end = end;
        return;
    }
    strncpy(segments[*count].name, name, sizeof(segments[*count].name) - 1);
    segments[*count].name[sizeof(segments[*count].name) - 1] = '\0';
    segments[*count].start = start;
    segments[*count].end = end;
    (*count)++;
}

static int all_finished(const Process *p, int n) {
    for (int i = 0; i < n; ++i) {
        if (p[i].remaining > 0) return 0;
    }
    return 1;
}

static void finalize_metrics(Process *p, int n) {
    for (int i = 0; i < n; ++i) {
        p[i].turnaround = p[i].finish - p[i].arrival;
        p[i].waiting = p[i].turnaround - p[i].burst;
    }
}

static void print_result(const char *title, Process *p, int n, Segment *segments, int segment_count) {
    double total_turnaround = 0.0;
    double total_waiting = 0.0;

    printf("\n[%s]\n", title);
    printf("Run order: ");
    for (int i = 0; i < segment_count; ++i) {
        printf("%s(%d-%d)%s", segments[i].name, segments[i].start, segments[i].end,
               i + 1 == segment_count ? "" : " -> ");
    }
    puts("");
    puts("Name  Arrive  Burst  Pri  Finish  Turnaround  Waiting");
    for (int i = 0; i < n; ++i) {
        total_turnaround += p[i].turnaround;
        total_waiting += p[i].waiting;
        printf("%-5s %-7d %-6d %-4d %-7d %-11d %d\n",
               p[i].name, p[i].arrival, p[i].burst, p[i].priority,
               p[i].finish, p[i].turnaround, p[i].waiting);
    }
    printf("Average turnaround: %.2f, average waiting: %.2f\n",
           total_turnaround / n, total_waiting / n);
}

static int pick_next(const Process *p, int n, int now, int mode) {
    int best = -1;
    for (int i = 0; i < n; ++i) {
        if (p[i].remaining <= 0 || p[i].arrival > now) continue;
        if (best == -1) {
            best = i;
            continue;
        }
        if (mode == 0) {
            if (p[i].arrival < p[best].arrival ||
                (p[i].arrival == p[best].arrival && strcmp(p[i].name, p[best].name) < 0)) {
                best = i;
            }
        } else if (mode == 1) {
            if (p[i].burst < p[best].burst ||
                (p[i].burst == p[best].burst && p[i].arrival < p[best].arrival)) {
                best = i;
            }
        } else {
            if (p[i].priority < p[best].priority ||
                (p[i].priority == p[best].priority && p[i].arrival < p[best].arrival)) {
                best = i;
            }
        }
    }
    return best;
}

static int next_arrival(const Process *p, int n, int now) {
    int next = -1;
    for (int i = 0; i < n; ++i) {
        if (p[i].remaining > 0 && p[i].arrival > now && (next == -1 || p[i].arrival < next)) {
            next = p[i].arrival;
        }
    }
    return next;
}

static void run_nonpreemptive(const char *title, const Process *input, int n, int mode) {
    Process p[MAX_PROC];
    Segment segments[MAX_SEG];
    int segment_count = 0;
    int now = 0;
    copy_processes(p, input, n);

    while (!all_finished(p, n)) {
        int idx = pick_next(p, n, now, mode);
        if (idx == -1) {
            int next = next_arrival(p, n, now);
            add_segment(segments, &segment_count, "IDLE", now, next);
            now = next;
            continue;
        }
        p[idx].start = now;
        add_segment(segments, &segment_count, p[idx].name, now, now + p[idx].remaining);
        now += p[idx].remaining;
        p[idx].remaining = 0;
        p[idx].finish = now;
    }
    finalize_metrics(p, n);
    print_result(title, p, n, segments, segment_count);
}

static void run_rr(const Process *input, int n, int quantum) {
    Process p[MAX_PROC];
    Segment segments[MAX_SEG];
    int queue[MAX_SEG];
    int in_queue[MAX_PROC] = {0};
    int head = 0, tail = 0, segment_count = 0, finished = 0, now = 0;
    copy_processes(p, input, n);

    while (finished < n) {
        for (int i = 0; i < n; ++i) {
            if (!in_queue[i] && p[i].remaining > 0 && p[i].arrival <= now) {
                queue[tail++ % MAX_SEG] = i;
                in_queue[i] = 1;
            }
        }

        if (head == tail) {
            int next = next_arrival(p, n, now);
            add_segment(segments, &segment_count, "IDLE", now, next);
            now = next;
            continue;
        }

        int idx = queue[head++ % MAX_SEG];
        in_queue[idx] = 0;
        if (p[idx].start == -1) p[idx].start = now;
        int slice = p[idx].remaining < quantum ? p[idx].remaining : quantum;
        add_segment(segments, &segment_count, p[idx].name, now, now + slice);
        now += slice;
        p[idx].remaining -= slice;

        for (int i = 0; i < n; ++i) {
            if (!in_queue[i] && p[i].remaining > 0 && p[i].arrival <= now && i != idx) {
                queue[tail++ % MAX_SEG] = i;
                in_queue[i] = 1;
            }
        }

        if (p[idx].remaining > 0) {
            queue[tail++ % MAX_SEG] = idx;
            in_queue[idx] = 1;
        } else {
            p[idx].finish = now;
            finished++;
        }
    }
    finalize_metrics(p, n);
    print_result("RR", p, n, segments, segment_count);
}

static int read_processes(Process *p, int *n, int *quantum) {
    printf("Process count (1-%d): ", MAX_PROC);
    if (scanf("%d", n) != 1 || *n <= 0 || *n > MAX_PROC) return 0;
    puts("Input: name arrival burst priority(smaller means higher)");
    for (int i = 0; i < *n; ++i) {
        if (scanf("%15s %d %d %d", p[i].name, &p[i].arrival, &p[i].burst, &p[i].priority) != 4) return 0;
        if (p[i].arrival < 0 || p[i].burst <= 0) return 0;
    }
    printf("RR quantum: ");
    if (scanf("%d", quantum) != 1 || *quantum <= 0) return 0;
    return 1;
}

static void run_all_algorithms(const Process *p, int n, int quantum) {
    run_nonpreemptive("FCFS", p, n, 0);
    run_nonpreemptive("SJF", p, n, 1);
    run_rr(p, n, quantum);
    run_nonpreemptive("Priority", p, n, 2);
    puts("\nAnalysis: FCFS is simple but may cause convoy effect; SJF lowers average waiting time when burst times are known; RR improves response fairness with context-switch overhead; priority scheduling reflects urgency but needs aging to avoid starvation.");
}

void scheduler_demo(void) {
    Process p[] = {
        {"P1", 0, 7, 3, 0, -1, 0, 0, 0},
        {"P2", 2, 4, 1, 0, -1, 0, 0, 0},
        {"P3", 4, 1, 4, 0, -1, 0, 0, 0},
        {"P4", 5, 4, 2, 0, -1, 0, 0, 0},
    };
    puts("\n==== Processor Scheduling Demo ====");
    run_all_algorithms(p, 4, 3);
}

void scheduler_menu(void) {
    Process p[MAX_PROC];
    int n = 0, quantum = 0;
    puts("\n==== Processor Scheduling ====");
    if (!read_processes(p, &n, &quantum)) {
        puts("Input error.");
        return;
    }
    run_all_algorithms(p, n, quantum);
}

int scheduler_selftest(void) {
    Process p[] = {
        {"A", 0, 3, 2, 0, -1, 0, 0, 0},
        {"B", 1, 2, 1, 0, -1, 0, 0, 0},
    };
    Process q[MAX_PROC];
    copy_processes(q, p, 2);
    return q[0].remaining == 3 && q[1].remaining == 2;
}
