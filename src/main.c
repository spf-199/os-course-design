#include <stdio.h>
#include <string.h>
#include "oslab.h"

static void print_menu(void) {
    puts("\n==== OS Course Design Lab ====");
    puts("1. Processor scheduling");
    puts("2. Memory management");
    puts("3. Process synchronization");
    puts("4. File system");
    puts("5. Run all demos");
    puts("0. Exit");
    printf("Choose: ");
}

static int run_selftests(void) {
    int ok = 1;
    ok &= scheduler_selftest();
    ok &= memory_selftest();
    ok &= sync_selftest();
    ok &= filesystem_selftest();
    printf("\nSelftest result: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}

static void run_all_demos(void) {
    scheduler_demo();
    memory_demo();
    sync_demo();
    filesystem_demo();
}

int main(int argc, char **argv) {
    if (argc > 1) {
        if (strcmp(argv[1], "--demo") == 0) {
            run_all_demos();
            return 0;
        }
        if (strcmp(argv[1], "--test") == 0) {
            return run_selftests();
        }
        if (strcmp(argv[1], "--help") == 0) {
            puts("Usage: oslab [--demo|--test|--help]");
            return 0;
        }
    }

    for (;;) {
        int choice = -1;
        print_menu();
        if (scanf("%d", &choice) != 1) {
            puts("Invalid input.");
            return 1;
        }
        switch (choice) {
            case 1: scheduler_menu(); break;
            case 2: memory_menu(); break;
            case 3: sync_menu(); break;
            case 4: filesystem_menu(); break;
            case 5: run_all_demos(); break;
            case 0: return 0;
            default: puts("Unknown choice."); break;
        }
    }
}
