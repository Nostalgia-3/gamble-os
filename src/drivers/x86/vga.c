#include <drivers/x86/vga.h>
#include <gosh/gosh.h>
#include <gosh/pipe.h>
#include <str.h>
#include <memory.h>
#include <port.h>
#include <str.h>

// black, red, green, yellow, blue, magenta, cyan, white, reset

static u8   ansi_to_text[]  = { 0x0, 0x4, 0x2, 0x6, 0x1, 0x5, 0x3, 0x7 };
static u8   bansi_to_text[] = { 0x8, 0xC, 0xA, 0xE, 0x9, 0xD, 0xB, 0xF };
static u16  screen_cursor   = 0;
static u8   attributes      = 7;
static bool in_ansi         = FALSE;
static u8   ansi_buf[64]    = {};
static u32  ansi_index      = 0;

static device_t out;

int vga_entry(module_t *dev) {
    // Initialize VGA at linear 320x200 @ 256 color mode (this is currently done
    // in BIOS because I'm bad at this)
    // set_mode((u8*)MODE_320_200);

    out = (device_t) {
        .write = vga_write
    };

    // Add a framebuffer device
    int suc = register_device("/dev/console", &out);
    if(suc < 0) {
        kprintf("Failed to create console device!\n");
        return DRIVER_FAILED;
    }

    return DRIVER_SUCCESS;
}

module_t get_vga_module() {
    return (module_t) {
        .name = "vga",
        .module_start = vga_entry
    };
}

void vga_set_cursor(u16 x, u16 y) {
    u16 pos = y * 80 + x;
    if(pos > 0);
    outb(0x3D4, 0x0F);
    outb(0x3D5, (u8) (pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (u8) ((pos >> 8) & 0xFF));
}

void vga_scroll_down() {    
    u32 line_width = (80*2);
    for(u32 i=0;i<25;i++) {
        memcpy((u8*)0xB8000+line_width*i, (u8*)0xB8000+line_width*(i+1), line_width);
    }
    memset((u8*)0xB8000+line_width*(25), 0, line_width);

    screen_cursor-=79;
}

u16 vga_get_cursor_x() {
    return (screen_cursor % 80);
}

u16 vga_get_cursor_y() {
    return (screen_cursor - (screen_cursor % 80))/80;
}

// void vga_write(char *st, size_t len)
ssize_t vga_write(const void *buf, size_t len, off_t *offset) {
    char *st = (char*)buf;

    for(int i=0;i<len;i++) {
        if(st[i] == '\0') break;
        if(in_ansi) {
            ansi_buf[ansi_index++] = st[i];

            if(is_letter(st[i])) {
                in_ansi = FALSE;

                if(ansi_buf[0] != '[') goto finish_ansi;

                char comm = st[i];

                switch(comm) {
                    case 'm': { // num[\;num]:*m
                        char* d = strtok(ansi_buf+1, 'm');

                        u32 code = atoi(d);

                        if(code == 0) attributes = 7;
                        else if(code == 5)      attributes |= 0x80;
                        else if((code)/10 == 3) attributes = (attributes & 0xF0) | ansi_to_text[code%10];
                        else if((code)/10 == 4) attributes = (attributes & 0x7F) | (ansi_to_text[code%10]<<4);
                        else if((code)/10 == 9) attributes = (attributes & 0xF0) | bansi_to_text[code%10];
                    break; }

                    case 'J': {
                        char* d = strtok(ansi_buf+1, 'J');
                        u32 code = atoi(d);

                        if(code == 0) {
                            for(int x=0;x<(80*25 - screen_cursor);x++) {
                                *(u8*)(0xB8000+x*2) = '\0';
                                *(u8*)(0xB8000+x*2+1) = attributes;
                            }
                        } else if(code == 1) {
                            for(int x=0;x<screen_cursor;x++) {
                                *(u8*)(0xB8000+x*2) = '\0';
                                *(u8*)(0xB8000+x*2+1) = attributes;
                            }
                        } else if(code == 2) {
                            for(int x=0;x<80*25;x++) {
                                *(u8*)(0xB8000+x*2) = '\0';
                                *(u8*)(0xB8000+x*2+1) = attributes;
                            }
                            screen_cursor = 0;
                        }
                    break; }

                    case 'K': {
                        char* d = strtok(ansi_buf+1, 'K');
                        u32 code = atoi(d);

                        u32 line = (screen_cursor-screen_cursor%80)*2;

                        if(code == 0) {
                            for(int x=screen_cursor%80;x<80;x++) {
                                *(u8*)(0xB8000+line+x*2) = '\0';
                                *(u8*)(0xB8000+line+x*2+1) = attributes;
                            }
                        } else if(code == 1) {
                            for(int x=0;x<screen_cursor%80;x++) {
                                *(u8*)(0xB8000+line+x*2) = '\0';
                                *(u8*)(0xB8000+line+x*2+1) = attributes;
                            }
                        } else if(code == 2) {
                            for(int x=0;x<80;x++) {
                                *(u8*)(0xB8000+line+x*2) = '\0';
                                *(u8*)(0xB8000+line+x*2+1) = attributes;
                            }
                        }
                    break; }

                    case 'H':
                        screen_cursor = 0;
                    break;
                }

            finish_ansi:
                memset(ansi_buf, 0, ansi_index);
                ansi_index = 0;
            }
        } else {
            if(st[i] == '\x1b') {
                in_ansi = TRUE;
            } else if(st[i] == '\n') {
                screen_cursor += 80 - (screen_cursor % 80);
            } else if(st[i] == '\t') {
                screen_cursor += 8 - (screen_cursor % 8);
            } else if(st[i] == '\b') {
                screen_cursor--;
                *(u8*)(0xB8000+screen_cursor*2) = '\0';
                *(u8*)(0xB8000+screen_cursor*2+1) = attributes;
            } else if(st[i] == '\r') {
                // Go to the beginning of the line
                screen_cursor = screen_cursor - (screen_cursor % 80);
            } else {
                *(u8*)(0xB8000+screen_cursor*2) = st[i];
                *(u8*)(0xB8000+screen_cursor*2+1) = attributes;
                if(screen_cursor >= 80*25) vga_scroll_down();
                else screen_cursor++;
            }
        }
    }

    vga_set_cursor(vga_get_cursor_x(), vga_get_cursor_y());
}

