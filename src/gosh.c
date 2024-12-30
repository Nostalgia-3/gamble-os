#include "gosh.h"

volatile const Device gdevt[256];



void kpanic() {
     __asm__ volatile ("cli; hlt":);
    while(1);
}