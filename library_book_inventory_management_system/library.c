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

// ---------- storage ----------

// Grow the array if full. Returns 0 on allocation failure.
static int ensure_capacity(Library *lib) {
    if (lib->count < lib->capacity)
        return 1;
    int new_cap = lib->capacity == 0 ? 8 : lib->capacity * 2;
    Book *tmp = realloc(lib->items, new_cap * sizeof(Book));
    if (!tmp) {
        printf("Error: memory allocation failed.\n");
        return 0;
    }
    lib->items = tmp;
    lib->capacity = new_cap;
    return 1;
}

// Return index of a book by ID, or -1 if not found.
static int find_by_id(Library *lib, int id) {
    for (int i = 0; i < lib->count; i++)
        if (lib->items[i].id == id)
            return i;
    return -1;
}

static void save_file(Library *lib) {
    FILE *f = fopen(DATA_FILE, "wb");
    if (!f) {
        printf("Error: could not open %s for writing.\n", DATA_FILE);
        return;
    }
    fwrite(&lib->count, sizeof(int), 1, f);
    fwrite(lib->items, sizeof(Book), lib->count, f);
    fclose(f);
}

static void load_file(Library *lib) {
    FILE *f = fopen(DATA_FILE, "rb");
    if (!f)
        return; // no file yet: start empty
    int count = 0;
    if (fread(&count, sizeof(int), 1, f) != 1 || count < 0) {
        fclose(f);
        return;
    }
    if (count == 0) { // empty file: nothing to load
        fclose(f);
        return;
    }
    lib->items = malloc(count * sizeof(Book));
    if (!lib->items) {
        printf("Error: memory allocation failed while loading.\n");
        fclose(f);
        return;
    }
    lib->count = (int)fread(lib->items, sizeof(Book), count, f);
    lib->capacity = count;
    fclose(f);
    printf("Loaded %d book(s) from %s.\n", lib->count, DATA_FILE);
}

