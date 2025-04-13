#pragma once

#include <types.h>

#define is_letter(c) ((((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z')) ? true : false)
#define is_printable(c) (((c) >= 32 && (c) <= 126) ? true : false)
#define is_digit(c) (((c) >= '0' && (c) <= '9') ? true : false)

int strcmp(char *s1, char *s2);

int strlen(char *s1);

uint32_t atoi(char* st);