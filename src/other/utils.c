#include <utils.h>

void hexdump(uint8_t* addr, size_t count) {
    // if(count == 0) return;

    // for(int i=0;i<count;i++) {
    //     if((i % 16) == 0) {
    //         if(i != 0) {
    //             printf_(" |................|\n");
    //         }

    //         printf_("%08X", addr);
    //     }

    //     if((i % 8) == 0 && i != 0) printf_(" ");

    //     printf_(" %02X", *(addr+i));
    // }

    // if(!(count % 16)) {
    //     printf_(" |................|");
    // }

    // printf_("\n");

    if(count == 0) return;
    for(int i=0;i<count;i++) {
        if((i % 16) == 0) {
            // output the ASCII equivalent of the past 16 bytes
            if(i != 0) {
                printf_(" |");
                for(int x=0;x<16;x++) {
                    uint8_t c = *(addr+i-16+x);
                    if(is_printable(c)) printf_("%c", c);
                    else printf_(".");
                }
                printf_("|\n");
            }
            printf_("%08X  ", addr+i);
        } else if((i%8) == 0 && i != 0) printf_(" ");
        printf_("%02X ", *(addr+i));
    }
    uint8_t ex = count%16;
    if(ex != 0) {
        for(int i=0;i<16-ex;i++) printf_("   ");
    }
    printf_(" |");
    ex = count%17;
    for(int x=0;x<ex;x++) {
        uint8_t c = *(addr+count-(16-ex)+x);
        if(is_printable(c)) printf_("%c", c);
        else printf_(".");
    }
    printf_("|\n");
}