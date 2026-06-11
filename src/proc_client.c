#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROC_PATH "/proc/oslab_monitor"

static int read_proc(void) {
    FILE *fp = fopen(PROC_PATH, "r");
    char line[256];
    if (!fp) {
        perror("open " PROC_PATH);
        return 1;
    }
    while (fgets(line, sizeof(line), fp)) {
        fputs(line, stdout);
    }
    fclose(fp);
    return 0;
}

static int write_proc(const char *text) {
    FILE *fp = fopen(PROC_PATH, "w");
    if (!fp) {
        perror("open " PROC_PATH);
        return 1;
    }
    fprintf(fp, "%s\n", text);
    fclose(fp);
    return 0;
}

int main(int argc, char **argv) {
    if (argc == 1) return read_proc();
    if (argc == 3 && strcmp(argv[1], "--write") == 0) return write_proc(argv[2]);
    fprintf(stderr, "Usage: %s [--write reset|note=text]\n", argv[0]);
    return 1;
}
