#include "strings.h"

#include <string.h>
#include <stdio.h>

string string_from(char* str) {
    string s = {str, strlen(str)};
    return s;
}

void print_string(string str) {
    printf("  [len=%lu]: ", str.len);
    fwrite(str.s, str.len, 1, stdout);
}

void print_stringn(string str) {
    printf("  [len=%lu]: ", str.len);
    fwrite(str.s, str.len, 1, stdout);
    printf("\n");
}
