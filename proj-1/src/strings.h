#ifndef STRINGS_H___
#define STRINGS_H___

#include <stddef.h>

typedef struct {
    char* s;
    size_t len;
} string;

string string_from(char* str);

void print_string(string str);

void print_stringn(string str);

#endif // STRINGS_H___
