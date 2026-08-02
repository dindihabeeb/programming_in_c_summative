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

