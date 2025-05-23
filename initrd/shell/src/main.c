#include <cpuid.h>

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include <printf.h>
#include <gstd.h>

static char str_storage[100];
static int tok_ind;
static char *curstr;
static int str_ind;

void _putchar(char character) {
    write(STDOUT, &character, 1);
}

size_t strlen(char* str) {
    char* start = str;
    if(str == NULL) return 0;
    while(*str != '\0') str++;
    return str - start;
}

// I have no idea if this is what strtok is supposed to do,
// but if it works, it works
char* strtok(char *str, char del) {
    if(str != NULL) {
        curstr = str;
        str_ind = 0;
    }

    tok_ind = 0;

    while(curstr[str_ind] != 0 && curstr[str_ind] != del) {
        str_storage[tok_ind++] = curstr[str_ind++];
    }

    str_storage[tok_ind++] = '\0';
    str_ind++;

    return str_storage;
}

int strcmp(char *s1, char *s2) {
    char c;
    size_t ind = 0;

    do {
        c = *(s1+ind);
        char c2 = *(s2+ind);

        if(c2 != c) {
            return c - c2;
        }

        if(c == 0 && c2 == 0) return 0;

        ind++;
    } while(c != 0);

    return 0;
}

uint32_t hatoi(char*st) {
    uint32_t val = 0;
    char c;
 
    while ((c = *st++)) {
        if(c >= '0' && c <= '9') {
            val *= 16;
            val += (uint32_t)(c - '0');
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

const char* get_entry_style(inode_type x) {
    switch(x) {
        case INODE_FILE: return "\x1b[32m"; break;
        case INODE_DEV: return "\x1b[36m"; break;
        case INODE_DIR: return "\x1b[34m"; break;
        case INODE_LINK: return "\x1b[35m"; break;
    }

    return "\x1b[31m";
}

void run_command(char* cmdbfr) {
    char* ex = strtok(cmdbfr, ' ');
    if(strcmp("", ex) == 0) {
    } else if(strcmp("help", ex) == 0) {
        printf(
            "Commands:\n"
            "  \x1b[93mcls/clear                            \x1b[0mclears the terminal\n"
            "  \x1b[93mls <dir>                             \x1b[0mlist contents of a directory\n"
            "  \x1b[93mcat <path>                           \x1b[0moutput the contents of path to stdout\n"
            "  \x1b[93mpal <path>                           \x1b[0mupdate palette to a palette file in initrd\n"
            "  \x1b[93mfetch                                \x1b[0mfetch information about the system\n"
            "  \x1b[93mwrite <addr> <val>                   \x1b[0mwrite a u32 to an arbitrary address\n"
            "  \x1b[93mread <addr>                          \x1b[0mread a u32 from an arbitrary address\n"
            // "  \x1b[93mmount\x1b[90m    <drive> <st:path>   \x1b[0mMount a device\n"
            // "  \x1b[93mmalloc\x1b[90m   <size>              \x1b[0mAllocate memory and output the address\n"
            // "  \x1b[93mhreset\x1b[90m                       \x1b[0mHard restart\n"
            // "  \x1b[93min[b/l]\x1b[90m  <port>              \x1b[0mRead data from a system port\n"
            // "  \x1b[93mout[b/l]\x1b[90m <port> <data>       \x1b[0mWrite data to a system port\n"
        );
    } else if(strcmp("cls", ex) == 0 || strcmp("clear", ex) == 0) {
        printf("\x1b[2J\x1b[H");
    } else if(strcmp("ls", ex) == 0) {
        char* path = strtok(NULL, ' ');
        if(strlen(path) == 0) path = "/";

        int32_t fd = open(path);
        struct stat st = {0};

        if(fd < 0) {
            printf("Failed to open path \"%s\"\n", path);
            return;
        }

        if(stat(fd, &st) < 0) {
            printf("Failed to stat fd?\n");
            return;
        }

        if(st.type != INODE_DIR) {
            printf("\"%s\" is not a directory! (type = %u)\n", path, st.type);
            return;
        }

        char buf[512] = {0};
        dirent* d = (dirent*)buf;
        size_t off = 0;
        ssize_t amount_read = getdents(fd, buf, sizeof(buf));

        if(amount_read < 0) {
            printf("Failed to read directory entry for path \"%s\"\n", path);
            return;
        }

        printf("Directory: %s\n", path);
        while(off < amount_read || d->len != 0) {
            printf_("%6u %s%s\x1b[0m\n", d->size, get_entry_style(d->type), d->name);
            off += d->len;
            d = (dirent*)((size_t)d + (size_t)off);
        }

        close(fd);
    } else if(strcmp("cat", ex) == 0) {
        char* path = strtok(NULL, ' ');
        int32_t fd = open(path);
        
        if(fd < 0) {
            printf("Failed to open path \"%s\"\n", path);
            return;
        }

        struct stat st;
        stat(fd, &st);

        char* buf = (char*)brk(st.size + 1);
        for(size_t i=0;i<st.size+1;i++) *(uint8_t*)(buf + i) = 0;
        read(fd, buf, st.size);

        write(STDOUT, buf, st.size);

        printf("\n");

        close(fd);
    } else if(strcmp("pal", ex) == 0) {
        char* path = strtok(NULL, ' ');
        int32_t fd = open(path);
        
        if(fd < 0) {
            printf("Failed to open path \"%s\"\n", path);
            return;
        }

        struct stat st;
        stat(fd, &st);

        char* buf = (char*)brk(st.size + 1);
        for(size_t i=0;i<st.size+1;i++) *(uint8_t*)(buf + i) = 0;
        read(fd, buf, st.size);
        close(fd);

        if(st.size < sizeof(uint32_t)*16) {
            printf("Palette isn't the correct size (it should be 64 bytes, but is instead %u bytes!)\n", st.size);
            return;
        }

        int32_t tty = open("/dev/tty");
        if(tty < 0) printf("Failed to open tty!!\n");

        if(ioctl((uint32_t)tty, TTY_CHANGE_PALETTE, buf) < 0) {
            printf("Failed to update palette!\n");
        }
    } else if(strcmp("fetch", ex) == 0) {
        printf_("todo\n");
    } else if(strcmp("write", ex) == 0) {
        ex = strtok(NULL, ' ');
        uint32_t addr = hatoi(ex);
        ex = strtok(NULL, ' ');
        uint32_t val = hatoi(ex);

        printf_("writing 0x%08x to 0x%08x\n", addr, val);

        *(uint32_t*)(addr) = val;
    } else if(strcmp("read", ex) == 0) {
        ex = strtok(NULL, ' ');
        uint32_t addr = hatoi(ex);

        printf_("read from 0x%08x\n", addr);

        printf_("%08x\n", *(uint32_t*)(addr));
    } else {
        printf("Unknown command: %s\n", ex);
    }
}

int main() {
    const char* prompt      = "\x1b[96m>\x1b[0m ";
    char        buffer[32]  = {0};
    bool        running     = true;
    char        cmdbfr[80]  = {0};
    int         index       = 0;

    printf("GambleOS Integrated Shell (0.0.1)\n");
    
    int ebx, ecx, edx, unused;
    __cpuid(0, unused, ebx, edx, ecx);

    printf("%c%c%c%c", ebx & 0xFF, (ebx >> 8) & 0xFF, (ebx >> 16) & 0xFF, (ebx >> 24) & 0xFF);
    printf("%c%c%c%c", ecx & 0xFF, (ecx >> 8) & 0xFF, (ecx >> 16) & 0xFF, (ecx >> 24) & 0xFF);
    printf("%c%c%c%c", edx & 0xFF, (edx >> 8) & 0xFF, (edx >> 16) & 0xFF, (edx >> 24) & 0xFF);
    _putchar('\n');

    printf(prompt);
    while(running) {
        int amount = read(STDIN, buffer, sizeof(buffer));
        if(amount == 0) continue;

        if(amount > 1) {
            if(buffer[0] != 0xE0) continue;

            switch(buffer[1]) {
                case 0x4B: break; // left arrow
                case 0x4D: break; // right arrow
                case 0x48: break; // up arrow
                case 0x5C: break; // down arrow
            }

            continue;
        }

        switch(buffer[0]) {
            case '\b':
                if(index == 0) break;
                cmdbfr[index] = '\0';
                index--;
                _putchar('\b');
                cmdbfr[index] = '\0';
            break;

            case '\n':
                _putchar('\n');
                // run command here
                run_command(cmdbfr);
                printf(prompt);

                for(int i=0;i<sizeof(cmdbfr);i++) cmdbfr[i] = '\0';
                index = 0;
            break;
            
            default:
                if(index >= sizeof(cmdbfr)) break;
                cmdbfr[index++] = buffer[0];
                _putchar(buffer[0]);
            break;
        }

        if(running) {
            // Fancy command highlighting
            printf("\x1b[2K\r%s\x1b[93m", prompt);

            int state = 0;

            for(int i=0;i<index;i++) {
                if(cmdbfr[i] == ' ') {
                    switch(state) {
                        case 0: state = 1; printf_("\x1b[0m"); break;
                        case 2: state = 1; printf_("\x1b[0m"); break;
                    }
                }

                if(i != 0 && state == 1 && cmdbfr[i-1] == ' ' && cmdbfr[i] == '-') {
                    state = 2;
                    printf_("\x1b[92m");
                }

                write(STDOUT, &cmdbfr[i], 1);
            }

            printf_("\x1b[0m");
        }
    }
    
    return 0;
}