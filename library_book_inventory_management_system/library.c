// Library Book Inventory Management System
// Menu-driven inventory of books stored in a dynamically grown array and
// persisted to a binary file. Supports add/update/delete, search, sort,
// and simple reports.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TITLE 100
#define MAX_AUTHOR 100
#define MAX_CATEGORY 50
#define DATA_FILE "library.dat"

typedef struct {
    int id;
    char title[MAX_TITLE];
    char author[MAX_AUTHOR];
    char category[MAX_CATEGORY];
    int copies;
} Book;

typedef struct {
    Book *items;
    int count;
    int capacity;
} Library;

// ---------- input helpers ----------

// Read a line into buf, stripping the trailing newline.
static void read_line(const char *prompt, char *buf, int size) {
    printf("%s", prompt);
    if (!fgets(buf, size, stdin)) {
        buf[0] = '\0';
        return;
    }
    buf[strcspn(buf, "\n")] = '\0';
}

// Read a whole integer, re-prompting until valid and >= min.
static int read_int(const char *prompt, int min) {
    char buf[64];
    char *end;
    long val;
    while (1) {
        read_line(prompt, buf, sizeof(buf));
        val = strtol(buf, &end, 10);
        if (end != buf && *end == '\0' && val >= min)
            return (int)val;
        printf("Invalid number, try again.\n");
    }
}

// Read a non-empty string.
static void read_nonempty(const char *prompt, char *buf, int size) {
    do {
        read_line(prompt, buf, size);
        if (buf[0] == '\0')
            printf("Cannot be empty, try again.\n");
    } while (buf[0] == '\0');
}

