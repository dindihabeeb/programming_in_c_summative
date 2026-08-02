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
static void *process_file(void *arg) {
    FileTask *t = arg;

    FILE *f = fopen(t->input, "r");
    if (!f) {
        printf("[%s] Error: cannot open file.\n", t->input);
        return NULL;
    }
    printf("[%s] Processing...\n", t->input);

    long lines = 0, words = 0, chars = 0;
    int c, in_word = 0;
    while ((c = fgetc(f)) != EOF) {
        chars++;
        if (c == '\n')
            lines++;
        if (isspace(c)) {
            in_word = 0;
        } else if (!in_word) {
            in_word = 1;
            words++;
        }
    }
    fclose(f);
    t->lines = lines;
    t->words = words;
    t->chars = chars;

    // write results to a separate output file
    char out[300];
    snprintf(out, sizeof(out), "%s.stats", t->input);
    FILE *of = fopen(out, "w");
    if (!of) {
        printf("[%s] Error: cannot write output file.\n", t->input);
        return NULL;
    }
    fprintf(of, "File: %s\nLines: %ld\nWords: %ld\nCharacters: %ld\n",
            t->input, lines, words, chars);
    fclose(of);

    printf("[%s] Done -> %s (lines=%ld, words=%ld, chars=%ld)\n",
           t->input, out, lines, words, chars);
    return NULL;
}

