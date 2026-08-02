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

// ---------- conversion functions + table ----------

typedef double (*ConvFunc)(double);

static double c_to_f(double v)  { return v * 9.0 / 5.0 + 32.0; }
static double f_to_c(double v)  { return (v - 32.0) * 5.0 / 9.0; }
static double km_to_mi(double v){ return v * 0.621371; }
static double mi_to_km(double v){ return v / 0.621371; }
static double kg_to_lb(double v){ return v * 2.20462; }
static double lb_to_kg(double v){ return v / 2.20462; }
static double cm_to_in(double v){ return v / 2.54; }
static double in_to_cm(double v){ return v * 2.54; }

typedef struct {
    const char *name;
    ConvFunc func;
} Conversion;

static const Conversion CONVERSIONS[] = {
    { "Celsius->Fahrenheit", c_to_f },
    { "Fahrenheit->Celsius", f_to_c },
    { "Kilometres->Miles",   km_to_mi },
    { "Miles->Kilometres",   mi_to_km },
    { "Kilograms->Pounds",   kg_to_lb },
    { "Pounds->Kilograms",   lb_to_kg },
    { "Centimetres->Inches", cm_to_in },
    { "Inches->Centimetres", in_to_cm },
};
static const int NCONV = (int)(sizeof(CONVERSIONS) / sizeof(CONVERSIONS[0]));

// ---------- input helpers ----------

static void read_line(const char *prompt, char *buf, int size) {
    printf("%s", prompt);
    if (!fgets(buf, size, stdin)) {
        buf[0] = '\0';
        return;
    }
    buf[strcspn(buf, "\n")] = '\0';
}

static int read_int(const char *prompt, int min, int max) {
    char buf[64], *end;
    long v;
    while (1) {
        read_line(prompt, buf, sizeof(buf));
        v = strtol(buf, &end, 10);
        if (end != buf && *end == '\0' && v >= min && v <= max)
            return (int)v;
        printf("Invalid choice, try again.\n");
    }
}

static double read_double(const char *prompt) {
    char buf[64], *end;
    double v;
    while (1) {
        read_line(prompt, buf, sizeof(buf));
        v = strtod(buf, &end);
        if (end != buf && *end == '\0')
            return v;
        printf("Invalid number, try again.\n");
    }
}

