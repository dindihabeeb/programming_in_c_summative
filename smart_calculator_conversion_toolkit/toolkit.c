// Unit Conversion Toolkit
// Menu-driven converter. A function-pointer table selects the conversion at
// runtime; callbacks handle mapping (precision), filtering, and sort compares.
// History is stored in a dynamic array and persisted to a binary file.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_TYPE 40
#define DATA_FILE "history.dat"

typedef struct {
    char type[MAX_TYPE];
    double input;
    double output;
} Record;

typedef struct {
    Record *items;
    int count;
    int capacity;
} History;

