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

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <file1> [file2 ...]\n", argv[0]);
        return 1;
    }

    int n = argc - 1;
    pthread_t *threads = malloc(n * sizeof(pthread_t));
    FileTask *tasks = malloc(n * sizeof(FileTask));
    if (!threads || !tasks) {
        printf("Error: memory allocation failed.\n");
        free(threads);
        free(tasks);
        return 1;
    }

    // one thread per input file
    for (int i = 0; i < n; i++) {
        snprintf(tasks[i].input, sizeof(tasks[i].input), "%s", argv[i + 1]);
        tasks[i].started = 0;
        if (pthread_create(&threads[i], NULL, process_file, &tasks[i]) == 0)
            tasks[i].started = 1;
        else
            printf("[%s] Error: could not create thread.\n", tasks[i].input);
    }

    // wait for every thread that started
    for (int i = 0; i < n; i++)
        if (tasks[i].started)
            pthread_join(threads[i], NULL);

    printf("All files processed.\n");
    free(threads);
    free(tasks);
    return 0;
}
