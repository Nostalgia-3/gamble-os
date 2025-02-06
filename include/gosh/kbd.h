#ifndef GOSH_KBD_H
#define GOSH_KBD_H

#include <gosh/common.h>
#include <types.h>

typedef enum _key_t {
    K_None, K_Back, K_Tab, K_Enter, K_Return, K_CapsLock, K_Escape, K_Space,
    K_PageUp, K_PageDown, K_End, K_Home, K_Left, K_Up, K_Right, K_Down, K_Insert,
    K_Delete, K_D0, K_D1, K_D2, K_D3, K_D4, K_D5, K_D6, K_D7, K_D8, K_D9, K_A,
    K_B, K_C, K_D, K_E, K_F, K_G, K_H, K_I, K_J, K_K, K_L, K_M, K_N, K_O, K_P,
    K_Q, K_R, K_S, K_T, K_U, K_V, K_W, K_X, K_Y, K_Z, K_F1, K_F2, K_F3, K_F4,
    K_F5, K_F6, K_F7, K_F8, K_F9, K_F10, K_F11, K_F12, K_LeftShift, K_RightShift,
    K_LeftCtrl, K_RightCtrl, K_LeftAlt, K_RightAlt, KeyLen
} key_t;

typedef struct _keyboard_dev_t {
    u8 key_statuses[KeyLen/8];
    u8 fifo[32];
    u8 fifo_ind;
} keyboard_dev_t;

// Push a key to the global FIFO
// void pushc(u8 c);
// Halt the process until a character from the keyboard is detected
// u8 scanc();
// Get the latest ascii key pressed, or 0 if there was no key
// u8 getc();

#endif//GOSH_KBD_H