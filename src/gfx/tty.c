#include <module.h>
#include <device.h>
#include <fs/vfs.h>
#include <printf.h>
#include <x86/x86.h>
#include <str.h>
#include <memory.h>

device tty;

#define FONT_WIDTH 9
#define FONT_HEIGHT 16
#define FONT_PADDINGRIGHT 7

void write_serial(char a) {
    while((inb(0x3f8 + 5) & 0x20) == 0);
    outb(0x3f8, a);
}

static uint16_t screen_cursor   = 0;
static uint8_t  attributes      = 7;
static bool     in_ansi         = false;
static uint8_t  ansi_buf[64]    = {};
static uint32_t ansi_index      = 0;

static uint32_t framebuffer_pitch = 0;
static uint32_t framebuffer_width = 0;
static uint32_t framebuffer_height = 0;
static uint32_t* framebuffer_addr   = NULL;

static uint16_t line_width      = 0;
static uint16_t line_height     = 0;

void set_fb(uint32_t pitch, uint32_t width, uint32_t height, void* addr) {
    framebuffer_pitch = pitch;
    framebuffer_width = width;
    framebuffer_height = height;
    framebuffer_addr = addr;

    line_width = framebuffer_width/FONT_WIDTH;
    line_height = framebuffer_height/FONT_HEIGHT;
}

uint32_t get_fbpitch() {
    return framebuffer_pitch;
}

extern unsigned char console_font_9x16[];

static uint32_t palette[] = {
    // Normal
    0x494d64,
    0xed8796,
    0xa6da95,
    0xeed49f,
    0x8aadf4,
    0xf5bde6,
    0x8bd5ca,
    0xa5adcb,

    // Bright
    0x494d64,
    0xed8796,
    0xa6da95,
    0xeed49f,
    0x8aadf4,
    0xf5bde6,
    0x8bd5ca,
    0xa5adcb,
};

void vga_scroll_down();

void setc(uint32_t pos, char c, uint8_t attributes) {
    if(c == '\n') return;

    size_t xpos = (pos % line_width) * FONT_WIDTH;
    size_t ypos = (pos / line_width) * FONT_HEIGHT;

    if(ypos/FONT_HEIGHT > line_height) vga_scroll_down();

    uint32_t fg = palette[attributes & 0xF];
    uint32_t bg = palette[(attributes >> 4)];

    for(int x=0;x<FONT_WIDTH;x++) {
        for(int y=0;y<FONT_HEIGHT;y++) {
            uint16_t line = (
                (console_font_9x16[y*2 + (c) * (FONT_HEIGHT * 2)] << 8) |
                (console_font_9x16[y*2 + (c) * (FONT_HEIGHT * 2) + 1])
            ) >> FONT_PADDINGRIGHT;
            *((uint32_t*)0xE0001000 + x + y*(framebuffer_pitch/sizeof(uint32_t)) + xpos + ypos*(framebuffer_pitch/sizeof(uint32_t))) = line & (1 << (FONT_WIDTH-x)) ? fg : bg;
        }
    }

    // *(char*)(0xC03FF000+pos*2)   = c;
    // *(char*)(0xC03FF000+pos*2+1) = ansi_to_ega_text(attributes);
}

void vga_set_cursor(uint16_t x, uint16_t y) {
    uint16_t pos = y * 80 + x;
    if(pos > 0);
    outb(0x3D4, 0x0F);
    outb(0x3D5, (char) (pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (char) ((pos >> 8) & 0xFF));
}

void vga_scroll_down() {
    size_t amount_to_copy = framebuffer_pitch * framebuffer_height - FONT_HEIGHT * framebuffer_pitch;

    memcpy((void*)0xE0001000, (void*)0xE0001000 + FONT_HEIGHT*framebuffer_pitch, amount_to_copy);
    for(size_t i=0;i<FONT_HEIGHT*framebuffer_pitch/4;i++) {
        *(uint32_t*)(0xE0001000 + amount_to_copy + i*4) = palette[attributes >> 4];
    }
    screen_cursor -= line_width;
}

uint16_t vga_get_cursor_x() {
    return (screen_cursor % line_width);
}

uint16_t vga_get_cursor_y() {
    return (screen_cursor - (screen_cursor % line_width))/line_width;
}

char str_storage[128];
volatile int tok_ind;
volatile char *curstr;
volatile int str_ind;

// I have no idea if this is what strtok is supposed to do,
// but if it works, it works
char* tty_strtok(char *str, char del) {
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

#define SEND_SERIAL 1

void _putchar(char c) {
#ifdef SEND_SERIAL
    if(c == '\n') write_serial('\r');
    write_serial(c);
#endif
    if(in_ansi) {
        ansi_buf[ansi_index++] = c;

        if(is_letter(c)) {
            in_ansi = false;

            if(ansi_buf[0] != '[') goto finish_ansi;

            char comm = c;

            switch(comm) {
                case 'm': { // num[\;num]:*m
                    char* d = tty_strtok((char*)((size_t)ansi_buf + (size_t)1), 'm');

                    uint32_t code = atoi(d);

                    if(code == 0) attributes = 7;
                    else if(code == 5)      attributes |= 0x80;
                    else if((code)/10 == 3) attributes = (attributes & 0xF0) | code%10;
                    else if((code)/10 == 4) attributes = (attributes & 0x7F) | (code%10 & 8)<<4;
                    // bright fg
                    else if((code)/10 == 9) attributes = (attributes & 0xF0) | (code%10 | 0b1000);
                break; }

                case 'J': {
                    char* d = tty_strtok((char*)((size_t)ansi_buf + (size_t)1), 'J');
                    uint32_t code = atoi(d);

                    if(code == 0) {
                        for(int x=0;x<(line_width*line_height - screen_cursor);x++) {
                            setc(x, '\0', attributes);
                        }
                    } else if(code == 1) {
                        for(int x=0;x<screen_cursor;x++) {
                            setc(x, '\0', attributes);
                        }
                    } else if(code == 2) {
                        for(int x=0;x<line_width*line_height;x++) {
                            setc(x, '\0', attributes);
                        }
                        screen_cursor = 0;
                    }
                break; }

                case 'K': {
                    char* d = tty_strtok((char*)((size_t)ansi_buf + (size_t)1), 'K');
                    uint32_t code = atoi(d);
                    uint32_t line = (screen_cursor-screen_cursor%line_width);

                    if(code == 0) {
                        for(int x=screen_cursor%line_width;x<line_width;x++) {
                            setc(x, '\0', attributes);
                        }
                    } else if(code == 1) {
                        for(int x=line;x<screen_cursor%line_width;x++) {
                            setc(x, '\0', attributes);
                        }
                    } else if(code == 2) {
                        for(int x=line;x<line_width;x++) {
                            setc(x, '\0', attributes);
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
        if(c == '\x1b') {
            in_ansi = true;
        } else if(c == '\n') {
            screen_cursor += line_width - (screen_cursor % line_width);
        } else if(c == '\t') {
            screen_cursor += line_width - (screen_cursor % line_width);
        } else if(c == '\b') {
            screen_cursor--;
            setc(screen_cursor, '\0', attributes);
        } else if(c == '\r') {
            // Go to the beginning of the line
            screen_cursor = screen_cursor - (screen_cursor % line_width);
        } else {
            if(screen_cursor >= line_width*line_height) vga_scroll_down();
            setc(screen_cursor, c, attributes);
            screen_cursor++;
        }
    }

    vga_set_cursor(vga_get_cursor_x(), vga_get_cursor_y());
}

ssize_t tty_read(void* buf, size_t len, off_t *offset) {
    return 0;
}

ssize_t tty_write(const void* buf, size_t len, off_t *offset) {
    for(int i=0;i<len;i++) {
        _putchar(((uint8_t*)buf)[i]);
    }

    return 0;
}

#define TTY_CHANGE_PALETTE 0
#define TTY_GET_SIZE 1

int tty_ioctl(int op, void* data) {
    switch(op) {
        case TTY_CHANGE_PALETTE: {
            if(data == NULL) return -1;
            uint32_t* pal = (uint32_t*)data;

            for(int i=0;i<16;i++) {
                palette[i] = pal[i];
            }
        break; }
    
        case TTY_GET_SIZE: {
            if(data == NULL) return -1;

            uint32_t* size = (uint32_t*)data;
            size[0] = line_width;
            size[1] = line_height;
        break; }
    }

    return 0;
}

int tty_start(module* mod) {
    tty = (device) {
        .owner  = mod,
        .read   = tty_read,
        .write  = tty_write,
        .ioctl  = tty_ioctl,
    };

    if(mknod("/dev/", "tty", INODE_DEV, &tty) < 0) {
        printf("Failed to create /dev/tty!");
    }

    return 0;
}

module get_tty_module(uint32_t pitch) {
    framebuffer_pitch = pitch;

    return (module) {
        .name = "tty",
        .module_start = tty_start
    };
}