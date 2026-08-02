# Library Book Inventory Management System

This is a menu driven program in C for keeping track of the books in a small library. You can add books, edit or remove them, search and sort the list, and print a quick report. The records are held in memory while the program runs and saved to a file so nothing is lost when you close it.

## Build and run

There is a Makefile, so:

```bash
make
./library
```

To remove the compiled program and the data file:

```bash
make clean
```

## How data is stored

Each book is a struct with these fields:

- Book ID (a whole number, must be unique)
- Title
- Author
- Category
- Number of copies available

The books are kept in one array that grows as needed using `malloc` and `realloc`, and is freed on exit. Everything is written to a binary file called `library.dat`. That file is read back automatically the next time you start the program, and it is rewritten after every change so the data on disk always matches what you see.

## Menu options

1. Add a book. It refuses to add an ID that already exists.
2. Display all books in a table.
3. Update a book. You give the ID, then type the new details.
4. Delete a book by ID.
5. Search, either by ID or by exact title.
6. Sort, by ID, title, or number of copies.
7. Inventory report.
8. Exit.

## About the search and sort

The task asked for these to be done manually, so I wrote them out rather than calling library functions. Search walks the array and compares (linear search). Sorting uses a selection sort with a small compare helper that switches on the field you picked.

## The report shows

- total number of books
- total copies across all books
- the title with the most copies
- how many books fall into each category

## Input checks and safety

- Numbers are read as whole lines and re-asked if you type something that isn't a valid number.
- Titles, authors and categories can't be left blank.
- Duplicate IDs are blocked when adding.
- File open failures and failed memory allocations print a message instead of crashing, and an empty saved file is handled without error.
