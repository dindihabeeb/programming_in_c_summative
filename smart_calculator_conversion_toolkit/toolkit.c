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

// ---------- callbacks ----------

// map: round a value to the given precision
static double cb_round(double v, int precision) {
    double f = pow(10.0, precision);
    return round(v * f) / f;
}

// filter predicates: keep records whose output is above/below arg
static int cb_above(const Record *r, double arg) { return r->output > arg; }
static int cb_below(const Record *r, double arg) { return r->output < arg; }

// compare predicates for sorting (return <0, 0, >0)
static int cmp_type(const Record *a, const Record *b) {
    return strcmp(a->type, b->type);
}
static int cmp_value(const Record *a, const Record *b) {
    if (a->output < b->output) return -1;
    return a->output > b->output;
}

typedef int (*FilterCB)(const Record *, double);
typedef int (*CompareCB)(const Record *, const Record *);

// apply a precision callback to every stored output
static void map_outputs(History *h, int precision) {
    for (int i = 0; i < h->count; i++)
        h->items[i].output = cb_round(h->items[i].output, precision);
}

// print records accepted by the filter callback
static void filter_records(History *h, FilterCB keep, double arg) {
    int found = 0;
    for (int i = 0; i < h->count; i++) {
        if (keep(&h->items[i], arg)) {
            printf("  %-22s %12.4f -> %.4f\n",
                   h->items[i].type, h->items[i].input, h->items[i].output);
            found = 1;
        }
    }
    if (!found)
        printf("No matching records.\n");
}

// selection sort driven by a compare callback
static void sort_records(History *h, CompareCB cmp) {
    for (int i = 0; i < h->count - 1; i++) {
        int sel = i;
        for (int j = i + 1; j < h->count; j++)
            if (cmp(&h->items[j], &h->items[sel]) < 0)
                sel = j;
        if (sel != i) {
            Record tmp = h->items[i];
            h->items[i] = h->items[sel];
            h->items[sel] = tmp;
        }
    }
}

