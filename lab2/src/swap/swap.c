#include "swap.h"

void Swap(char *left, char *right)
{
	if (left != right) {
		*left = *left ^ *right;
		*right = *right ^ *left;
		*left = *left ^ *right;
	}
}
