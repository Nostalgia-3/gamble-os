#ifndef STRING_H
#define STRING_H

#include "types.h"

// The max number of characters returned by strtok
#define MAX_STRTOK_LEN 128

void *memset(void *str, int c, size_t n);

char* itoa(int val, int base);

size_t strlen(char *s1);

int strcmp(char *s1, char *s2);

char* strtok(char *str, char del);

#endif