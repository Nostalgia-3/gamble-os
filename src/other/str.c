#include <str.h>
#include <memory.h>
#include <types.h>

int strcmp(char *s1, char *s2) {
    size_t ind = 0;
    uint8_t c;
    uint8_t c2;

    do {
        c = *(s1+ind);
        c2 = *(s2+ind);

        if(c2 != c) return c - c2;
        if(c == 0 && c2 == 0) return 0;
        ind++;
    } while(c != 0);

    return 0;
}

int strlen(char *s1) {
    if(s1 == NULL) return 0;
    
    int i = 0;

    while(*s1) {
        i++;
        s1++;
    }

    return i;
}

uint32_t atoi(char* st) {
    uint32_t val = 0;
    char c;
 
    while ((c = *st++)) {
        if(is_digit(c)) {
            val *= 10UL;
            val += (uint32_t)(c - '0');
        }
    }
    return val;
}