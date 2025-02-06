#ifndef STRING_H
#define STRING_H

#include "types.h"

// The max number of characters returned by strtok
#define MAX_STRTOK_LEN 128

void *memset(void *str, int c, size_t n);

// Copy a string into another buffer
void strcpy(u8* dest, u8* src);

// Concatenate two strings, putting src at the end of dest
void strcat(u8* dest, u8* src);

// Convert an int to ascii
char* itoa(int val, int base);
// Convert an ascii base-10 number to an int
u32 atoi(char*);
// Convert an ascii base-16 number to an int
u32 hatoi(char*);

#define is_letter(c) ((((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z')) ? TRUE : FALSE)
#define is_printable(c) (((c) >= 32 && (c) <= 126) ? TRUE : FALSE)
#define is_digit(c) (((c) >= '0' && (c) <= '9') ? TRUE : FALSE)

const char *get_last_del(const char *st, char del);
size_t strlen(char *s1);
int strcmp(char *s1, char *s2);
char* strtok(char *str, char del);

#endif