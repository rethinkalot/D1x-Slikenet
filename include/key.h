#ifndef D1X_VIRTUAL_KEY_H
#define D1X_VIRTUAL_KEY_H
#include <stdint.h>
#include <SDL_keyboard.h>
#include <SDL_keysym.h>

extern uint8_t keyd_pressed[];
#define KEY_MAC(x)

extern uint32_t keyd_time_when_last_pressed;
extern uint8_t keyd_last_pressed;
extern uint8_t keyd_last_released;

/* Standard hardware scan mapping translations for gameplay control blocks */
#define KEY_ESC             0x01
#define KEY_1               0x02
#define KEY_2               0x03
#define KEY_3               0x04
#define KEY_4               0x05
#define KEY_5               0x06
#define KEY_6               0x07
#define KEY_7               0x08
#define KEY_8               0x09
#define KEY_9               0x0A
#define KEY_0               0x0B
#define KEY_MINUS           0x0C
#define KEY_EQUAL           0x0D
#define KEY_BACKSP          0x0E
#define KEY_TAB             0x0F

#define KEY_A               0x1E
#define KEY_R               0x13
#define KEY_I               0x17
#define KEY_P               0x19
#define KEY_B               0x30
#define KEY_D               0x20
#define KEY_F               0x21
#define KEY_K               0x25
#define KEY_C               0x2E
#define KEY_SPACEBAR        0x39

#define KEY_F1              0x3B
#define KEY_F2              0x3C
#define KEY_F3              0x3D
#define KEY_F4              0x3E
#define KEY_F5              0x3F
#define KEY_F6              0x40
#define KEY_F7              0x41
#define KEY_F8              0x42
#define KEY_F9              0x43
#define KEY_F10             0x44
#define KEY_F11             0x57
#define KEY_F12             0x58

#define KEY_PAUSE           0x61
#define KEY_PRINT_SCREEN    0xB7
#define KEY_DELETE          0xD3
#define KEY_ENTER           0x1C
#define KEY_PERIOD          0x34

#define KEY_CAPSLOCK        0x3A
#define KEY_SCROLLOCK       0x46
#define KEY_NUMLOCK         0x45

#define KEY_HOME            0xC7
#define KEY_UP              0xC8
#define KEY_PAGEUP          0x49
#define KEY_LEFT            0xCB
#define KEY_RIGHT           0xCD
#define KEY_END             0xCF
#define KEY_DOWN            0xD0
#define KEY_PAGEDOWN        0x51

#define KEY_PAD7            0x101
#define KEY_PAD8            0x102
#define KEY_PAD9            0x103
#define KEY_PAD4            0x104
#define KEY_PAD5            0x105
#define KEY_PAD6            0x106
#define KEY_PAD1            0x107
#define KEY_PAD2            0x108
#define KEY_PAD3            0x109
#define KEY_PAD0            0x10A
#define KEY_PADPERIOD       0x10B
#define KEY_PADENTER        0x10C

/* Arithmetic Keypad Scan Code Slots */
#define KEY_PADDIVIDE       0x10D
#define KEY_PADMULTIPLY     0x10E
#define KEY_PADMINUS        0x10F
#define KEY_PADPLUS         0x110

#define KEY_LSHIFT          0x2A
#define KEY_RSHIFT          0x36
#define KEY_LCTRL           0x1D
#define KEY_RCTRL           0x9D

#define KEY_ALTED           0x200
#define KEY_DEBUGGED        0x400
#define KEY_COMMAND         0x800
#define KEY_SHIFTED         0x1000
#define KEY_CTRLED          0x2000

typedef struct kc_key_prop {
    const char *key_text;
} kc_key_prop;

extern kc_key_prop key_properties[];

#endif
