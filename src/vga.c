#include "vga.h"

u16 get_bios_area_hardware() {
    return *(const u16*)0x410;
}

// Get the BIOS video type
enum VideoType get_bios_video_type() {
    return (enum VideoType) (get_bios_area_hardware() & 0x30);
}

void clear(u8 c, TextAttribute attr) {
    for(u32 i=0;i<80*25*2;i+=2) {
        *(u8*)(i+0xB8000) = c;
        *(TextAttribute*)(i+0xB8000+1) = attr;
    }
}

void drawc(u8 c, TextAttribute attr, u32 addr) {

}

void write(u8* st, u8 x, u8 y) {
    u8 c = *st;
    u32 i = 0;

    *(u8*)(0xB8000) = *st;

    // while(c != '\0') {
        

    //     c = *(st+i); i++;
    // }
}