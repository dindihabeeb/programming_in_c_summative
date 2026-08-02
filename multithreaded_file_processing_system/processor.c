// Multi-threaded File Processing System
// Spawns one POSIX thread per input file. Each thread counts the lines,
// words, and characters of its file and writes the result to "<file>.stats".
// Threads share no data, so no synchronization is needed.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>

typedef struct {
    char input[256];
    long lines, words, chars;
    int started;   // 1 if the thread was created
} FileTask;

// Thread body: analyze one file and save its stats.
