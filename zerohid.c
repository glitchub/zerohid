// Read key events from stdin and write corresponding keyboard reports to the
// specified OTG HID device. Events are either straight ASCII or hex-encoded X
// keys.

#define usage() die("\
Usage:\n\
\n\
    zerohid [options] /dev/hidX [/dev/hidX]\n\
\n\
Read key events from stdin and write reports to specified OTG HID device.\n\
Supports XKB mode and ASCII mode.\n\
\n\
In XKB mode, keyboard and mouse events are read from stdin, one per line,\n\
and converted to HID key or mouse codes. Key codes are sent to the first\n\
specified HID device, mouse codes to the second device (if given).\n\
\n\
In ASCII mode, individual characters are read from stdin and converted to HID\n\
key codes.\n\
\n\
By default, starts in XKB mode and if an empty line is received switch to ASCII\n\
mode.\n\
\n\
Options are:\n\
\n\
    -a      - start in ASCII mode\n\
    -d      - write debug messages to stdout\n\
    -x      - start in XKB mode, disable switch to ASCII mode\n\
")

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <termios.h>
#include <time.h>
#include <sched.h>

#include "hidkeys.h"

// This defines the X11 key code symbols, header file is lifted directly from
// the X11 distro. The symbol naming isn't super consistent.
#define XK_LATIN1
#define XK_MISCELLANY
#include "keysymdef.h"

// write message to stderr and exit
#define die(...) ({ fprintf(stderr, __VA_ARGS__); exit(1); })

#define expect(q) ({ if (!(q)) die("Failed expect line %d: %s (%s)\n", __LINE__, #q, strerror(errno)); })

// write debug messages to stdout if enabled
bool dodebug = false;
#define debug(...) ({ if (dodebug) fprintf(stdout, __VA_ARGS__); })

// Restore saved tty state, invoked by atexit()
struct termios saveattr;
static void restore(void) { tcsetattr(0, TCSANOW, &saveattr); }

// Return monotonic milliseconds since boot, wraps after 49 days!
uint32_t mS(void)
{
    struct timespec t;
    expect (!clock_gettime(CLOCK_MONOTONIC, &t));
    return ((uint32_t)t.tv_sec*1000) + (t.tv_nsec/1000000);
}

// write report of specified size to hid file descriptor. Return 0 on success, -1 if blocked for one second, die if error
int write_hid(int hid, uint8_t *report, int size)
{
    uint32_t start = mS();

    while (size > 0)
    {
        int sent = write(hid, report, size);
        if (sent > 0)
        {
            report += sent;
            size -= sent;
        } else
        {
            expect(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR);
            if (mS() - start > 1000)
            {
                debug("hid timeout\n");
                return -1;
            }
            sched_yield();
        }
    }
    return 0;
}

// Given ASCII character return 16-bit scan code, upper byte is modifier bit or 0, lower byte is the scan code or 0
#define shift(c) ( ((uint16_t)HID_LSHIFT<<8) | (c))
#define control(c) ( ((uint16_t)HID_LCTRL<<8) | (c))
uint16_t a2scan(uint8_t key)
{
    switch(key)
    {
        case   0: return control(HID_2);        // control chars
        case   1: return control(HID_A);
        case   2: return control(HID_B);
        case   3: return control(HID_C);
        case   4: return control(HID_D);
        case   5: return control(HID_E);
        case   6: return control(HID_F);
        case   7: return control(HID_G);
        case   8: return HID_BACKSPACE;         // ^H -> backspace
        case   9: return HID_TAB;               // ^I -> tab
        case  10: return HID_ENTER;             // ^J aka \n -> enter
        case  11: return control(HID_K);
        case  12: return control(HID_L);
        case  13: return control(HID_M);
        case  14: return control(HID_N);
        case  15: return control(HID_O);
        case  16: return control(HID_P);
        case  17: return control(HID_Q);
        case  18: return control(HID_R);
        case  19: return control(HID_S);
        case  20: return control(HID_T);
        case  21: return control(HID_U);
        case  22: return control(HID_V);
        case  23: return control(HID_W);
        case  24: return control(HID_X);
        case  25: return control(HID_Y);
        case  26: return control(HID_Z);
        case  27: return HID_ESC;
        case  28: return control(HID_LEFTBRACE);
        case  29: return control(HID_BACKSLASH);
        case  30: return control(HID_RIGHTBRACE);
        case  31: return control(HID_MINUS);
        case  32: return HID_SPACE;
        case  33: return shift(HID_1);          // !
        case  34: return shift(HID_APOSTROPHE); // "
        case  35: return shift(HID_3);          // #
        case  36: return shift(HID_4);          // $
        case  37: return shift(HID_5);          // %
        case  38: return shift(HID_7);          // &
        case  39: return HID_APOSTROPHE;
        case  40: return shift(HID_9);          // (
        case  41: return shift(HID_0);          // )
        case  42: return shift(HID_8);          // *
        case  43: return shift(HID_EQUAL);      // +
        case  44: return HID_COMMA;
        case  45: return HID_MINUS;
        case  46: return HID_DOT;
        case  47: return HID_SLASH;
        case  48: return HID_0;
        case  49: return HID_1;
        case  50: return HID_2;
        case  51: return HID_3;
        case  52: return HID_4;
        case  53: return HID_5;
        case  54: return HID_6;
        case  55: return HID_7;
        case  56: return HID_8;
        case  57: return HID_9;
        case  58: return shift(HID_SEMICOLON);  // :
        case  59: return HID_SEMICOLON;
        case  60: return shift(HID_COMMA);      // <
        case  61: return HID_EQUAL;
        case  62: return shift(HID_DOT);        // >
        case  63: return shift(HID_SLASH);      // |
        case  64: return shift(HID_2);          // @
        case  65: return shift(HID_A);          // upper case letters
        case  66: return shift(HID_B);
        case  67: return shift(HID_C);
        case  68: return shift(HID_D);
        case  69: return shift(HID_E);
        case  70: return shift(HID_F);
        case  71: return shift(HID_G);
        case  72: return shift(HID_H);
        case  73: return shift(HID_I);
        case  74: return shift(HID_J);
        case  75: return shift(HID_K);
        case  76: return shift(HID_L);
        case  77: return shift(HID_M);
        case  78: return shift(HID_N);
        case  79: return shift(HID_O);
        case  80: return shift(HID_P);
        case  81: return shift(HID_Q);
        case  82: return shift(HID_R);
        case  83: return shift(HID_S);
        case  84: return shift(HID_T);
        case  85: return shift(HID_U);
        case  86: return shift(HID_V);
        case  87: return shift(HID_W);
        case  88: return shift(HID_X);
        case  89: return shift(HID_Y);
        case  90: return shift(HID_Z);
        case  91: return HID_LEFTBRACE;
        case  92: return HID_BACKSLASH;
        case  93: return HID_RIGHTBRACE;
        case  94: return shift(HID_6);          // ^
        case  95: return shift(HID_MINUS);      // _
        case  96: return HID_GRAVE;             // `
        case  97: return HID_A;                 // lower case letters
        case  98: return HID_B;
        case  99: return HID_C;
        case 100: return HID_D;
        case 101: return HID_E;
        case 102: return HID_F;
        case 103: return HID_G;
        case 104: return HID_H;
        case 105: return HID_I;
        case 106: return HID_J;
        case 107: return HID_K;
        case 108: return HID_L;
        case 109: return HID_M;
        case 110: return HID_N;
        case 111: return HID_O;
        case 112: return HID_P;
        case 113: return HID_Q;
        case 114: return HID_R;
        case 115: return HID_S;
        case 116: return HID_T;
        case 117: return HID_U;
        case 118: return HID_V;
        case 119: return HID_W;
        case 120: return HID_X;
        case 121: return HID_Y;
        case 122: return HID_Z;
        case 123: return shift(HID_LEFTBRACE);  // }
        case 124: return shift(HID_BACKSLASH);  // |
        case 125: return shift(HID_RIGHTBRACE); // {
        case 126: return shift(HID_GRAVE);      // ~
        case 127: return HID_BACKSPACE;         // DEL -> backspace

        default:  return 0;                     // invalid
    }
};

// Given xkb code return 16-bit scan code, upper byte is modifer bit or 0, lower byte is scan code or 0
uint16_t x2scan(uint16_t key)
{
    switch(key)
    {
        case XK_A: case XK_a:                     return HID_A;
        case XK_B: case XK_b:                     return HID_B;
        case XK_C: case XK_c:                     return HID_C;
        case XK_D: case XK_d:                     return HID_D;
        case XK_E: case XK_e:                     return HID_E;
        case XK_F: case XK_f:                     return HID_F;
        case XK_G: case XK_g:                     return HID_G;
        case XK_H: case XK_h:                     return HID_H;
        case XK_I: case XK_i:                     return HID_I;
        case XK_J: case XK_j:                     return HID_J;
        case XK_K: case XK_k:                     return HID_K;
        case XK_L: case XK_l:                     return HID_L;
        case XK_M: case XK_m:                     return HID_M;
        case XK_N: case XK_n:                     return HID_N;
        case XK_O: case XK_o:                     return HID_O;
        case XK_P: case XK_p:                     return HID_P;
        case XK_Q: case XK_q:                     return HID_Q;
        case XK_R: case XK_r:                     return HID_R;
        case XK_S: case XK_s:                     return HID_S;
        case XK_T: case XK_t:                     return HID_T;
        case XK_U: case XK_u:                     return HID_U;
        case XK_V: case XK_v:                     return HID_V;
        case XK_W: case XK_w:                     return HID_W;
        case XK_X: case XK_x:                     return HID_X;
        case XK_Y: case XK_y:                     return HID_Y;
        case XK_Z: case XK_z:                     return HID_Z;
        case XK_1: case XK_exclam:                return HID_1;;
        case XK_2: case XK_at:                    return HID_2;;
        case XK_3: case XK_numbersign:            return HID_3;;
        case XK_4: case XK_dollar:                return HID_4;;
        case XK_5: case XK_percent:               return HID_5;;
        case XK_6: case XK_asciicircum:           return HID_6;;
        case XK_7: case XK_ampersand:             return HID_7;;
        case XK_8: case XK_asterisk:              return HID_8;;
        case XK_9: case XK_parenleft:             return HID_9;;
        case XK_0: case XK_parenright:            return HID_0;;
        case XK_Return:                           return HID_ENTER;;
        case XK_Escape:                           return HID_ESC;;
        case XK_BackSpace:                        return HID_BACKSPACE;;
        case XK_Tab:                              return HID_TAB;;
        case XK_space:                            return HID_SPACE;;
        case XK_minus: case XK_underscore:        return HID_MINUS;;
        case XK_equal: case XK_plus:              return HID_EQUAL;;
        case XK_braceleft: case XK_bracketleft:   return HID_LEFTBRACE;;
        case XK_braceright: case XK_bracketright: return HID_RIGHTBRACE;;
        case XK_backslash: case XK_bar:           return HID_BACKSLASH;;
        case XK_semicolon: case XK_colon:         return HID_SEMICOLON;;
        case XK_apostrophe: case XK_quotedbl:     return HID_APOSTROPHE;;
        case XK_grave: case XK_asciitilde:        return HID_GRAVE;;
        case XK_comma: case XK_less:              return HID_COMMA;;
        case XK_period: case XK_greater:          return HID_DOT;;
        case XK_slash: case XK_question:          return HID_SLASH;;
        case XK_Caps_Lock:                        return HID_CAPSLOCK;;
        case XK_F1:                               return HID_F1;;
        case XK_F2:                               return HID_F2;;
        case XK_F3:                               return HID_F3;;
        case XK_F4:                               return HID_F4;;
        case XK_F5:                               return HID_F5;;
        case XK_F6:                               return HID_F6;;
        case XK_F7:                               return HID_F7;;
        case XK_F8:                               return HID_F8;;
        case XK_F9:                               return HID_F9;;
        case XK_F10:                              return HID_F10;;
        case XK_F11:                              return HID_F11;;
        case XK_F12:                              return HID_F12;;
        case XK_Sys_Req :                         return HID_SYSRQ;;
        case XK_Scroll_Lock:                      return HID_SCROLLLOCK;;
        case XK_Pause: case XK_Break:             return HID_PAUSE;;
        case XK_Insert:                           return HID_INSERT;;
        case XK_Home:                             return HID_HOME;;
        case XK_Page_Up:                          return HID_PAGEUP;;
        case XK_Delete:                           return HID_DELETE;;
        case XK_End:                              return HID_END;;
        case XK_Page_Down:                        return HID_PAGEDOWN;;
        case XK_Right:                            return HID_RIGHT;;
        case XK_Left:                             return HID_LEFT;;
        case XK_Down:                             return HID_DOWN;;
        case XK_Up:                               return HID_UP;;
        case XK_Num_Lock:                         return HID_NUMLOCK;;
        case XK_KP_Divide:                        return HID_KPSLASH;;
        case XK_KP_Multiply:                      return HID_KPASTERISK;;
        case XK_KP_Subtract:                      return HID_KPMINUS;;
        case XK_KP_Add:                           return HID_KPPLUS;;
        case XK_KP_Enter:                         return HID_KPENTER;;
        case XK_KP_1: case XK_KP_End:             return HID_KP1;;
        case XK_KP_2: case XK_KP_Down:            return HID_KP2;;
        case XK_KP_3: case XK_KP_Page_Down:       return HID_KP3;;
        case XK_KP_4: case XK_KP_Left:            return HID_KP4;;
        case XK_KP_5:                             return HID_KP5;;
        case XK_KP_6: case XK_KP_Right:           return HID_KP6;;
        case XK_KP_7: case XK_KP_Home:            return HID_KP7;;
        case XK_KP_8: case XK_KP_Up:              return HID_KP8;;
        case XK_KP_9: case XK_KP_Page_Up:         return HID_KP9;;
        case XK_KP_0: case XK_KP_Insert:          return HID_KP0;;
        case XK_KP_Decimal: case XK_KP_Delete:    return HID_KPDOT;;

        // modifer bits go in the high byte
        case XK_Control_L:                        return HID_LCTRL << 8;;
        case XK_Shift_L:                          return HID_LSHIFT << 8;;
        case XK_Alt_L:                            return HID_LALT << 8;;
        case XK_Super_L:                          return HID_LSUPER << 8;;
        case XK_Control_R:                        return HID_RCTRL << 8;;
        case XK_Shift_R:                          return HID_RSHIFT << 8;;
        case XK_Alt_R:                            return HID_RALT << 8;;
        case XK_Super_R:                          return HID_RSUPER << 8;;

        default: return 0;
    }
}

// Return one character from stdin, die if EOF or error
uint8_t readchar(void)
{
    while (true)
    {
        uint8_t c;
        int r = read(0, &c, 1); // read from stdin
        if (r == 1) return c;
        expect(errno == EINTR || errno == EAGAIN);
    }
}

// Read a '\n'-terminated line from stdin to given buffer, possibly truncated at specified len-1 (but always 0-terminated).
// Non-printable chars are ignored, returns size of string 0 to len-1
int readline(char *s, int len)
{
    int n = 0;
    while(true)
    {
        uint8_t c = readchar();
        if (c == '\n')
        {
            s[n] = 0;
            return n;
        }
        if (c >= ' ' && c <= '~' && n < len-1) s[n++] = c;
    }
}

int main(int argc, char *argv[])
{
    int mode = 0;            // 0 = xkb with shift to ascii, 1 = xkb only, 2 = ascii

    while(true) switch(getopt(argc, argv, ":adD:s:xz:"))
    {
        case 'a': mode = 2; break;
        case 'd': dodebug = true; break;
        case 'x': mode = 1; break;
        case ':':            // missing
        case '?': usage();   // or invalid options
        case -1: goto optx;  // no more options
    } optx:
    argc -= (optind-1);
    argv += (optind-1);
    if (argc < 2 || argc > 3) usage();

    debug("Starting zerohid in %s mode\n", (mode==0)?"auto":(mode==1)?"xkb":"ascii");

    int keyboard = open(argv[1], O_RDWR|O_NONBLOCK);
    if (keyboard <= 0) die("Can't open %s: %s\n", argv[1], strerror(errno));

    int mouse = 0;
    if (argc == 3)
    {
        int mouse = open(argv[2], O_RDWR|O_NONBLOCK);
        if (mouse <= 0) die("Can't open %s: %s\n", argv[2], strerror(errno));
    }

    if (isatty(0))
    {
        // put stdin tty in raw mode
        struct termios t;
        expect(!tcgetattr(0, &t));          // get current
        saveattr = t;                       // save a copy
        t.c_lflag &= ~(ICANON|ECHO|ISIG);   // make raw
        t.c_cc[VMIN] = 1;
        t.c_cc[VTIME] = 0;
        tcsetattr(0, TCSANOW, &t);
        atexit(restore);                    // restore tty on exit
    }

    if (mode < 2) while(true)
    {
        // The input is text, one event per line
        char s[32];
        int got = readline(s, sizeof s);
        if (got == 0) // empty line?
        {
            if (mode)
            {
                debug("xkb ignore null input\n");
                continue;
            }
            debug("xkb switch to ascii\n");
            break;  // break to the ascii loop below
        }

        if (s[0] == '+' || s[0] == '-' || s[0] == '!')
        {
            // Key event
            static uint8_t report[8] = {0}; // last sent report
            if (s[0] == '!')
            {
                debug("xkb reset\n");
                memset(report, 0, sizeof report);
            } else
            {
                uint16_t key;
                int n;
                if (sscanf(s+1, "%hu %n", &key, &n) != 1 || s[n+1]) goto invalid;

                uint16_t scan = x2scan(key);
                debug("xkb %d => %d\n", key, scan);
                if (scan)
                {
                    if (s[0] == '+')
                    {
                        // key pressed
                        if (scan > 255)
                            // Set modifier bit
                            report[0] |= scan >> 8;
                        else
                        {
                            // Add key to first empty report slot, if not already there
                            int slot = 2;
                            for (; slot < sizeof(report); slot++)
                            {
                                if (!report[slot])
                                {
                                    report[slot] = scan & 0xff;
                                    break;
                                }
                                if (report[slot] == (scan & 0xff)) break;
                            }
                            if (slot == sizeof report)
                            {
                                // oops, send overflow in all slots
                                debug("xkb overflow!\n");
                                write_hid(keyboard, (uint8_t[]){report[0], 0, HID_OVF, HID_OVF, HID_OVF, HID_OVF, HID_OVF, HID_OVF}, 8);
                                continue;
                            }
                        }
                    } else
                    {
                        // key released
                        if (scan > 255)
                            // Reset modifier bit
                            report[0] &= ~(scan >> 8);
                        else
                        {
                            // Delete scancode from report, if it's there
                            bool del = false;
                            for (int i = 2; i < sizeof(report); i++)
                            {
                                if (del) report[i-1] = report[i]; // shift remaining scan codes over
                                else if (report[i] == (scan & 0xff)) del = true;
                            }
                            if (del) report[7] = 0;
                        }
                    }
                }
            }
            // send key report
            write_hid(keyboard, report, 8);
        }
        else if (s[0] >= '0' && s[0] <= '7')
        {
            // Mouse event, code is the 3-bit button state.
            // Payload is decimal-encoded absolute X 0-32765, Y 0-32765, and (optional) relative wheel -127 to +127.
            if (!mouse) debug("xkb ignore mouse event\n");
            uint16_t X, Y;
            int8_t W = 0;
            int n;
            int r = sscanf(s+1, "%hu %hu %n %hhd %n", &X, &Y, &n, &W, &n);
            if (r < 2 || r > 3 || X > 32767 || Y > 32767 || W < -127 || s[n+1]) goto invalid;
            debug("xkb mouse buttons=%c X=%u Y=%u W=%d\n", s[0], X, Y, W);
            write_hid(mouse, (uint8_t []){s[0]-'0', X & 255, X >> 8, Y & 255, Y >> 8, W}, 6); // little endian!
        }
        else
        {
            invalid:
            if (dodebug)
            {
                // dump line in hex
                fprintf(stderr, "xkb invalid:");
                for (int i = 0; i < got; i++) fprintf(stderr," %02X", s[i]);
                fprintf(stderr, "\n");
            }
        }
    }

    // Here, input is raw ASCII chars
    while (true)
    {
        uint8_t key = readchar();
        uint16_t scan = a2scan(key);
        debug("ascii %02X => %04X\n", key, scan);
        if (!write_hid(keyboard, (uint8_t[]){scan >> 8, 0, scan & 0xff, 0, 0, 0, 0, 0}, 8))   // press
            write_hid(keyboard, (uint8_t[]){0, 0, 0, 0, 0, 0, 0, 0}, 8);                      // release
    }
}
