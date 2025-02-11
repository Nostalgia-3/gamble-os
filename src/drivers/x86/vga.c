#include <drivers/x86/vga.h>
#include <gosh/gosh.h>
#include <str.h>
#include <memory.h>
#include <port.h>
#include <str.h>
#include <font.h>

// black, red, green, yellow, blue, magenta, cyan, white, reset

static u8   ansi_to_text[]  = { 0x0, 0x4, 0x2, 0x6, 0x1, 0x5, 0x3, 0x7, 0x8, 0xC, 0xA, 0xE, 0x9, 0xD, 0xB, 0xF };
static u16  screen_cursor   = 0;
static u8   attributes      = 7;
static bool in_ansi         = FALSE;
static u8   ansi_buf[64]    = {};
static u32  ansi_index      = 0;

static u16 line_width       = 0;
static u16 line_height      = 0;

static u8 type              = 0;
static u32 width            = 0;
static u32 height           = 0;
static u32 bpp              = 0;
static u32* fb_addr         = 0;

static device_t out;

// black, red, green, yellow, blue, magenta, cyan, white, reset
uint32_t colors[16] = {
    0x000000, 0x800000, 0x008000, 0x808000, 0x000080, 0x800080, 0x008080, 0x808080,
    0x606060, 0xFF0000, 0x00FF00, 0xFFFF00, 0x0000FF, 0xFF00FF, 0x00FFFF, 0xFFFFFF
};

u8 ansi_to_ega_text(u8 attr) {
    return attr & (1 << 7) | ansi_to_text[(attr >> 4) & 8] | ansi_to_text[attr & 0xF];
}

int vga_entry(module_t *dev) {
    // Initialize VGA at linear 320x200 @ 256 color mode (this is currently done
    // in BIOS because I'm bad at this)
    // set_mode((u8*)MODE_320_200);

    out = (device_t) {
        .write = vga_write
    };

    // Add a framebuffer device
    int suc = register_device("console", &out);
    if(suc < 0) {
        kprintf("Failed to create console device!\n");
        return DRIVER_FAILED;
    }

    return DRIVER_SUCCESS;
}

module_t get_vga_module(multiboot_info_t *mbd) {
    return (module_t) {
        .name = "vga",
        .module_start = vga_entry,
        .data = mbd
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
    if(type == MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT) {
        u32 line_width = (80*2);
        for(u32 i=0;i<25;i++) {
            memcpy((u8*)0xB8000+line_width*i, (u8*)0xB8000+line_width*(i+1), line_width);
        }
        memset((u8*)0xB8000+line_width*(25), 0, line_width);
    } else {
        // TODO: make scrolling work
    }

    screen_cursor-=line_width-1;
}

u16 vga_get_cursor_x() {
    return (screen_cursor % line_width);
}

u16 vga_get_cursor_y() {
    return (screen_cursor - (screen_cursor % line_width))/line_width;
}

void set_vga_mbd(multiboot_info_t *mbd) {
    type    = mbd->framebuffer_type;
    if(type == 2) {
        width = 80;
        height = 25;
        bpp = 0;
        line_width = 80;
        line_height = 25;
    } else {
        width   = mbd->framebuffer_width;
        height  = mbd->framebuffer_height;
        bpp     = mbd->framebuffer_bpp;

        if(mbd->framebuffer_addr > 0xFFFFFFFF) {
            // puts_dbg("framebuffer address is too high :(\n");
            return;
        }

        line_width  = width / CHAR_WIDTH;
        line_height = height / CHAR_HEIGHT;

        fb_addr = (u32*)(u32)(mbd->framebuffer_addr & 0xFFFFFFFF);
    }
}

void setc(uint32_t pos, char c, uint8_t attributes) {
    if(c == '\n') return;

    if(type == 2) {
        *(u8*)(0xB8000+pos*2)   = c;
        *(u8*)(0xB8000+pos*2+1) = ansi_to_ega_text(attributes);
    } else {
        // draw a character at the pos
        if(bpp != 32) return; // support for non 24-bit/32-bit color isn't gonna happen
        if(c > 128) return;
        for(int y=0;y<CHAR_HEIGHT;y++) {
            for(int x=0;x<CHAR_WIDTH;x++) {
                u32 posx = pos % line_width;
                u32 posy = (pos - posx) / line_width;
                if(font[c][y] & 1<<x) { // (1<<CHAR_WIDTH)>>x
                    fb_addr[posx*CHAR_WIDTH+x+(posy*CHAR_HEIGHT+y)*width] = colors[attributes & 0xF];
                    fb_addr[1+posx*CHAR_WIDTH+x+(posy*CHAR_HEIGHT+y)*width] = colors[attributes & 0xF];
                } else {
                    fb_addr[posx*CHAR_WIDTH+x+(posy*CHAR_HEIGHT+y)*width] = colors[(attributes >> 4) & 8];
                    fb_addr[1+posx*CHAR_WIDTH+x+(posy*CHAR_HEIGHT+y)*width] = colors[(attributes >> 4) & 8];
                }
            }
        }
    }
}

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
                        else if((code)/10 == 3) attributes = (attributes & 0xF0) | code%10;
                        else if((code)/10 == 4) attributes = (attributes & 0x7F) | (code%10 & 8)<<4;
                        // bright fg
                        else if((code)/10 == 9) attributes = (attributes & 0xF0) | (code%10 | 0b1000);
                    break; }

                    case 'J': {
                        char* d = strtok(ansi_buf+1, 'J');
                        u32 code = atoi(d);

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
                        char* d = strtok(ansi_buf+1, 'K');
                        u32 code = atoi(d);

                        u32 line = (screen_cursor-screen_cursor%line_width);

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
            if(st[i] == '\x1b') {
                in_ansi = TRUE;
            } else if(st[i] == '\n') {
                screen_cursor += line_width - (screen_cursor % line_width);
            } else if(st[i] == '\t') {
                screen_cursor += CHAR_WIDTH - (screen_cursor % CHAR_WIDTH);
            } else if(st[i] == '\b') {
                screen_cursor--;
                setc(screen_cursor, '\0', attributes);
            } else if(st[i] == '\r') {
                // Go to the beginning of the line
                screen_cursor = screen_cursor - (screen_cursor % line_width);
            } else {
                setc(screen_cursor, st[i], attributes);
                if(screen_cursor >= line_width*line_height) vga_scroll_down();
                else screen_cursor++;
            }
        }
    }

    vga_set_cursor(vga_get_cursor_x(), vga_get_cursor_y());
}