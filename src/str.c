#include "types.h"
#include "memory.h"
#include "str.h"

char* itoa(int val, int base) {
    int tval = val;
    static char buf[32] = {0};
    
    if(tval == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return buf;
    }

    int i = 30;
    
    for(; tval && i ; --i, tval /= base)
        buf[i] = "0123456789abcdef"[tval % base];
    
    return &buf[i+1];
}

u32 atoi(char* st) {
    u32 val = 0;
    char c;
 
    while (c = *st++) {
        if(is_digit(c)) {
            val *= 10UL;
            val += (u32)(c - '0');
        }
    }
    return val;
}

u32 hatoi(char*st) {
    u32 val = 0;
    char c;
 
    while (c = *st++) {
        if(c >= '0' && c <= '9') {
            val *= 16;
            val += (u32)(c - '0');
        } else if((c >= 'a' && c <= 'f')) {
            val *= 16;
            val += c-'a'+10;
        } else if((c >= 'A' && c <= 'F')) {
            val *= 16;
            val += c-'A'+10;
        } else {
            // non-hex character here
            return val;
        }
    }
    return val;
}

size_t strlen(char *s1) {
    u8 c;
    size_t ind = 0;

    while((c = *(s1+ind)) != NULL) {
        ind++;
    }

    return ind;
}

int strcmp(char *s1, char *s2) {
    u8 c;
    size_t ind = 0;

    do {
        c = *(s1+ind);
        u8 c2 = *(s2+ind);

        if(c2 != c) {
            return c - c2;
        }

        if(c == NULL && c2 == NULL) return 0;

        ind++;
    } while(c != NULL);
}

void *memset(void *str, int c, size_t n) {
    for(size_t i=0;i<n;i++) {
        *(u8*)(str+i) = c;
    }

    return str;
}

char str_storage[MAX_STRTOK_LEN];
volatile int tok_ind;
volatile char *curstr;
volatile int str_ind;

// I have no idea if this is what strtok is supposed to do,
// but if it works, it works
char* strtok(char *str, char del) {
    if(str != NULL) {
        curstr = str;
        str_ind = 0;
    }

    tok_ind = 0;

    while(curstr[str_ind] != NULL && curstr[str_ind] != del) {
        str_storage[tok_ind++] = curstr[str_ind++];
    }

    str_storage[tok_ind++] = '\0';
    str_ind++;

    return str_storage;
}

const char *get_last_del(const char *st, char del) {
    size_t index = 0;
    for(int i=0;i<strlen(st);i++) {
        if(st[i] == del) index = i;
    }
    return st+index+1;
}

void strcpy(u8* dest, u8* src) {
    for(size_t i=0;i<strlen(src);i++) {
        dest[i] = src[i];
    }
}

void strcat(u8* dest, u8* src) {
    size_t destlen = strlen(dest);
    for(size_t i=0;i<strlen(src);i++) {
        dest[i+destlen] = src[i];
    }
}