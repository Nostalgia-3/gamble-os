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