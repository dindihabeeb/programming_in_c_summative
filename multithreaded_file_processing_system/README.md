# Multi-threaded File Processing System

This program reads several text files at the same time using POSIX threads. Each file gets its own thread, that thread counts the lines, words and characters in the file, and writes the totals to a separate output file. Because no two threads ever touch the same data, there are no locks anywhere in the code.

## Build and run

```bash
make
./processor file1.txt file2.txt file3.txt
```

Pass as many files as you like on the command line. One thread is created per file. `make clean` removes the binary and any `.stats` files left behind.

## What each thread does

1. Opens its file. If that fails (missing file, no permission) it prints an error and stops, without taking the whole program down.
2. Reads through the file once, keeping running counts:
   - lines: number of newline characters
   - words: runs of non-whitespace, tracked with a simple in-word flag
   - characters: every byte read
3. Writes the results to `<filename>.stats`.

For example, running it on `notes.txt` produces `notes.txt.stats` containing something like:

```
File: notes.txt
Lines: 12
Words: 84
Characters: 501
```

The counts match what `wc` reports on the same file, which is how I checked them.

## Status output

While it runs, each thread prints to the terminal when it starts a file and when it finishes, so you can see them working in parallel:

```
[a.txt] Processing...
[b.txt] Processing...
[a.txt] Done -> a.txt.stats (lines=2, words=5, chars=24)
```

The order can vary from run to run since the threads run concurrently. That is expected.

## Error handling

- A file that can't be opened is reported and skipped.
- If an output file can't be written, that thread says so.
- If a thread fails to start, or memory can't be allocated for the thread list, the program reports it instead of continuing blindly.

## Note on building

It needs the pthread library. The Makefile compiles with `-pthread`, so there is nothing extra to do. Written against standard POSIX threads, so it builds and runs the same on Linux.
