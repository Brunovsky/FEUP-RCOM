#include "strings.h"

#include <stdlib.h>
#include <string.h>

void free_string(string str) {
	free(str.s);
}

string string_from(char* str) {
	string s = {str, strlen(str)};
	return s;
}
