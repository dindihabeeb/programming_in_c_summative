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

// ---------- printing ----------

static void print_header(void) {
    printf("\n%-6s %-30s %-20s %-15s %-7s\n",
           "ID", "Title", "Author", "Category", "Copies");
    printf("--------------------------------------------------------------------------------\n");
}

static void print_book(const Book *b) {
    printf("%-6d %-30s %-20s %-15s %-7d\n",
           b->id, b->title, b->author, b->category, b->copies);
}

// ---------- operations ----------

static void add_book(Library *lib) {
    int id = read_int("Book ID: ", 1);
    if (find_by_id(lib, id) != -1) {
        printf("Error: a book with ID %d already exists.\n", id);
        return;
    }
    if (!ensure_capacity(lib))
        return;

    Book *b = &lib->items[lib->count];
    b->id = id;
    read_nonempty("Title: ", b->title, MAX_TITLE);
    read_nonempty("Author: ", b->author, MAX_AUTHOR);
    read_nonempty("Category: ", b->category, MAX_CATEGORY);
    b->copies = read_int("Copies available: ", 0);
    lib->count++;

    save_file(lib);
    printf("Book added.\n");
}

static void display_all(Library *lib) {
    if (lib->count == 0) {
        printf("No books in inventory.\n");
        return;
    }
    print_header();
    for (int i = 0; i < lib->count; i++)
        print_book(&lib->items[i]);
}

static void update_book(Library *lib) {
    int id = read_int("Book ID to update: ", 1);
    int i = find_by_id(lib, id);
    if (i == -1) {
        printf("No book with ID %d.\n", id);
        return;
    }
    Book *b = &lib->items[i];
    printf("Enter new details (ID stays %d).\n", id);
    read_nonempty("Title: ", b->title, MAX_TITLE);
    read_nonempty("Author: ", b->author, MAX_AUTHOR);
    read_nonempty("Category: ", b->category, MAX_CATEGORY);
    b->copies = read_int("Copies available: ", 0);

    save_file(lib);
    printf("Book updated.\n");
}

static void delete_book(Library *lib) {
    int id = read_int("Book ID to delete: ", 1);
    int i = find_by_id(lib, id);
    if (i == -1) {
        printf("No book with ID %d.\n", id);
        return;
    }
    // shift the tail down to fill the gap
    for (int j = i; j < lib->count - 1; j++)
        lib->items[j] = lib->items[j + 1];
    lib->count--;

    save_file(lib);
    printf("Book deleted.\n");
}

