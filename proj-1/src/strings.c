#include "strings.h"

#include <string.h>

string string_from(char* str) {
    string s = {str, strlen(str)};
    return s;
}
