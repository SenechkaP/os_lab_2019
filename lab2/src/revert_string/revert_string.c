#include "revert_string.h"
#include <string.h>
#include <stddef.h>

void RevertString(char *str)
{
	if (str == NULL) {
		return;
	}

	char *start = str;
	char *end = start + strlen(str) - 1;

	while (start < end) {
		char temp = *start;
		*start = *end;
		*end = temp;

		start++;
		end--;
	}
}
