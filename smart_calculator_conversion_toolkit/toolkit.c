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

// ---------- storage ----------

static int ensure_capacity(History *h) {
    if (h->count < h->capacity)
        return 1;
    int cap = h->capacity == 0 ? 8 : h->capacity * 2;
    Record *tmp = realloc(h->items, cap * sizeof(Record));
    if (!tmp) {
        printf("Error: memory allocation failed.\n");
        return 0;
    }
    h->items = tmp;
    h->capacity = cap;
    return 1;
}

static void print_record(const Record *r) {
    printf("  %-22s %12.4f -> %.4f\n", r->type, r->input, r->output);
}

static void view_history(History *h) {
    if (h->count == 0) {
        printf("History is empty.\n");
        return;
    }
    printf("\n%-24s %12s    %s\n", "Type", "Input", "Output");
    printf("--------------------------------------------------------\n");
    for (int i = 0; i < h->count; i++)
        print_record(&h->items[i]);
}

static void save_history(History *h) {
    FILE *f = fopen(DATA_FILE, "wb");
    if (!f) {
        printf("Error: could not open %s for writing.\n", DATA_FILE);
        return;
    }
    fwrite(&h->count, sizeof(int), 1, f);
    fwrite(h->items, sizeof(Record), h->count, f);
    fclose(f);
    printf("Saved %d record(s).\n", h->count);
}

static void load_history(History *h) {
    FILE *f = fopen(DATA_FILE, "rb");
    if (!f) {
        printf("No saved history found.\n");
        return;
    }
    int count = 0;
    if (fread(&count, sizeof(int), 1, f) != 1 || count < 0) {
        printf("Error: corrupt history file.\n");
        fclose(f);
        return;
    }
    if (count == 0) { // empty history file
        free(h->items);
        h->items = NULL;
        h->count = 0;
        h->capacity = 0;
        fclose(f);
        printf("Loaded 0 record(s).\n");
        return;
    }
    Record *tmp = malloc(count * sizeof(Record));
    if (!tmp) {
        printf("Error: memory allocation failed while loading.\n");
        fclose(f);
        return;
    }
    free(h->items);
    h->items = tmp;
    h->count = (int)fread(h->items, sizeof(Record), count, f);
    h->capacity = count;
    fclose(f);
    printf("Loaded %d record(s).\n", h->count);
}

// ---------- operations ----------

static void perform_conversion(History *h) {
    printf("\nAvailable conversions:\n");
    for (int i = 0; i < NCONV; i++)
        printf("  %d) %s\n", i + 1, CONVERSIONS[i].name);

    int choice = read_int("Select a conversion: ", 1, NCONV);
    double input = read_double("Enter value: ");

    // function pointer selected from the table at runtime
    ConvFunc f = CONVERSIONS[choice - 1].func;
    double output = f(input);

    if (!ensure_capacity(h))
        return;
    Record *r = &h->items[h->count++];
    strcpy(r->type, CONVERSIONS[choice - 1].name);
    r->input = input;
    r->output = output;

    printf("Result: %.4f\n", output);
}

static void search_menu(History *h) {
    if (h->count == 0) {
        printf("History is empty.\n");
        return;
    }
    printf("Search by: 1) Conversion type  2) Converted value\n");
    int choice = read_int("Select: ", 1, 2);
    int found = 0;

    if (choice == 1) {
        char type[MAX_TYPE];
        read_line("Enter conversion type (e.g. Miles->Kilometres): ", type, MAX_TYPE);
        for (int i = 0; i < h->count; i++)
            if (strcmp(h->items[i].type, type) == 0) {
                print_record(&h->items[i]);
                found = 1;
            }
    } else {
        double val = read_double("Enter converted value: ");
        for (int i = 0; i < h->count; i++)
            if (fabs(h->items[i].output - val) < 1e-6) {
                print_record(&h->items[i]);
                found = 1;
            }
    }
    if (!found)
        printf("No matching records.\n");
}

static void sort_submenu(History *h) {
    if (h->count == 0) {
        printf("History is empty.\n");
        return;
    }
    printf("Sort by: 1) Conversion type  2) Converted value\n");
    int choice = read_int("Select: ", 1, 2);
    sort_records(h, choice == 1 ? cmp_type : cmp_value);
    printf("Sorted.\n");
    view_history(h);
}

static void callbacks_menu(History *h) {
    if (h->count == 0) {
        printf("History is empty.\n");
        return;
    }
    printf("Callback: 1) Round all outputs to precision  2) Filter records\n");
    int choice = read_int("Select: ", 1, 2);

    if (choice == 1) {
        int p = read_int("Decimal precision (0-10): ", 0, 10);
        map_outputs(h, p);
        printf("Applied.\n");
        view_history(h);
    } else {
        printf("Keep records with output: 1) above  2) below a value\n");
        int dir = read_int("Select: ", 1, 2);
        double arg = read_double("Value: ");
        filter_records(h, dir == 1 ? cb_above : cb_below, arg);
    }
}

// ---------- menu ----------

int main(void) {
    History h = { NULL, 0, 0 };

    while (1) {
        printf("\n===== Unit Conversion Toolkit =====\n");
        printf("1) Perform a conversion\n");
        printf("2) View history\n");
        printf("3) Search records\n");
        printf("4) Sort records\n");
        printf("5) Apply callback operations\n");
        printf("6) Save history\n");
        printf("7) Load history\n");
        printf("8) Exit\n");

        int choice = read_int("Select an option: ", 1, 8);
        switch (choice) {
            case 1: perform_conversion(&h); break;
            case 2: view_history(&h);       break;
            case 3: search_menu(&h);        break;
            case 4: sort_submenu(&h);       break;
            case 5: callbacks_menu(&h);     break;
            case 6: save_history(&h);       break;
            case 7: load_history(&h);       break;
            case 8:
                free(h.items);
                printf("Goodbye!\n");
                return 0;
        }
    }
}
