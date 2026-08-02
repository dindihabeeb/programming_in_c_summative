# Unit Conversion Toolkit

A C program that converts between common units and remembers what you converted. The interesting part, and the point of the exercise, is that it leans on function pointers: the conversion you pick from the menu is looked up in a table and called through a pointer, and the extra operations (rounding, filtering, sorting) are done with callback functions.

## Build and run

```bash
make
./toolkit
```

It links against the math library (`-lm`) for rounding, which the Makefile already handles. `make clean` removes the binary and the saved history.

## The conversions

Eight of them, four pairs both ways:

- Celsius to Fahrenheit and back
- Kilometres to Miles and back
- Kilograms to Pounds and back
- Centimetres to Inches and back

Each one is a tiny function. They all sit in a table next to their names, and `perform_conversion` grabs the right function pointer based on your menu choice.

## The callbacks

There are three kinds, one for each thing the assignment asked for:

- Map: round every stored result to a precision you choose.
- Filter: show only the records whose output is above (or below) a value you give. Which of the two is used is itself decided by a function pointer.
- Compare: used while sorting to decide the order, by type or by value.

## History

Every conversion you do is appended to a history list. That list lives in a dynamically allocated array (`malloc` and `realloc`, freed at exit) and can be saved to and loaded from a binary file, `history.dat`.

## Menu

1. Perform a conversion
2. View history
3. Search records (by conversion type or by converted value)
4. Sort records (by type or by value)
5. Apply callback operations (round all, or filter)
6. Save history
7. Load history
8. Exit

Search and sort are both written by hand, a linear scan for search and a selection sort for ordering.

## Error handling

Menu and numeric inputs are validated and re-asked when wrong. Opening a file that isn't there, a corrupt or empty history file, and a failed allocation are all reported rather than left to crash.
