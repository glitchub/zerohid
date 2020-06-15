// USB HID key scan codes for 101-key keyboards only. The names are
// QWERTY-centric even though scan codes don't care about the labels on the key
// caps.

// modifer bits, in the 'mod' byte
#define HID_LCTRL  0x01
#define HID_LSHIFT 0x02
#define HID_LALT   0x04
#define HID_LSUPER 0x08
#define HID_RCTRL  0x10
#define HID_RSHIFT 0x20
#define HID_RALT   0x40
#define HID_RSUPER 0x80

// report in all slots if too many keys pressed
#define HID_OVF 0x01

#define HID_A 0x04
#define HID_B 0x05
#define HID_C 0x06
#define HID_D 0x07
#define HID_E 0x08
#define HID_F 0x09
#define HID_G 0x0a
#define HID_H 0x0b
#define HID_I 0x0c
#define HID_J 0x0d
#define HID_K 0x0e
#define HID_L 0x0f
#define HID_M 0x10
#define HID_N 0x11
#define HID_O 0x12
#define HID_P 0x13
#define HID_Q 0x14
#define HID_R 0x15
#define HID_S 0x16
#define HID_T 0x17
#define HID_U 0x18
#define HID_V 0x19
#define HID_W 0x1a
#define HID_X 0x1b
#define HID_Y 0x1c
#define HID_Z 0x1d

#define HID_1 0x1e                          // or !
#define HID_2 0x1f                          // or @
#define HID_3 0x20                          // or #
#define HID_4 0x21                          // or $
#define HID_5 0x22                          // or %
#define HID_6 0x23                          // or ^
#define HID_7 0x24                          // or &
#define HID_8 0x25                          // or *
#define HID_9 0x26                          // or (
#define HID_0 0x27                          // or )

#define HID_ENTER 0x28
#define HID_ESC 0x29
#define HID_BACKSPACE 0x2a
#define HID_TAB 0x2b
#define HID_SPACE 0x2c
#define HID_MINUS 0x2d                      // or _
#define HID_EQUAL 0x2e                      // or +
#define HID_LEFTBRACE 0x2f                  // or {
#define HID_RIGHTBRACE 0x30                 // or }
#define HID_BACKSLASH 0x31                  // or |
#define HID_HASHTILDE 0x32                  // #/~
#define HID_SEMICOLON 0x33                  // or :
#define HID_APOSTROPHE 0x34                 // or "
#define HID_GRAVE 0x35                      // or ~
#define HID_COMMA 0x36                      // or <
#define HID_DOT 0x37                        // or >
#define HID_SLASH 0x38                      // or ?
#define HID_CAPSLOCK 0x39

#define HID_F1 0x3a                         // F1
#define HID_F2 0x3b                         // F2
#define HID_F3 0x3c                         // F3
#define HID_F4 0x3d                         // F4
#define HID_F5 0x3e                         // F5
#define HID_F6 0x3f                         // F6
#define HID_F7 0x40                         // F7
#define HID_F8 0x41                         // F8
#define HID_F9 0x42                         // F9
#define HID_F10 0x43                        // F10
#define HID_F11 0x44                        // F11
#define HID_F12 0x45                        // F12

#define HID_SYSRQ 0x46
#define HID_SCROLLLOCK 0x47
#define HID_PAUSE 0x48
#define HID_INSERT 0x49
#define HID_HOME 0x4a
#define HID_PAGEUP 0x4b
#define HID_DELETE 0x4c
#define HID_END 0x4d
#define HID_PAGEDOWN 0x4e
#define HID_RIGHT 0x4f
#define HID_LEFT 0x50
#define HID_DOWN 0x51
#define HID_UP 0x52

#define HID_NUMLOCK 0x53

#define HID_KPSLASH 0x54
#define HID_KPASTERISK 0x55
#define HID_KPMINUS 0x56
#define HID_KPPLUS 0x57
#define HID_KPENTER 0x58
#define HID_KP1 0x59                        // or end
#define HID_KP2 0x5a                        // or down
#define HID_KP3 0x5b                        // or page down
#define HID_KP4 0x5c                        // or left
#define HID_KP5 0x5d
#define HID_KP6 0x5e                        // or right
#define HID_KP7 0x5f                        // or home
#define HID_KP8 0x60                        // or up
#define HID_KP9 0x61                        // or page up
#define HID_KP0 0x62                        // or insert
#define HID_KPDOT 0x63                      // or delete
