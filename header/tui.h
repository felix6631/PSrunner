#ifndef TUI_H
#define TUI_H
#include <stdio.h>
#include "vector2.h"

/* ═══════════════════════════════════════════════════════════════════════
 *  Platform detection
 * ═══════════════════════════════════════════════════════════════════════ */
#if defined(__APPLE__) || defined(__linux__)
#  define TUI_POSIX
#  include <unistd.h>
#  include <sys/ioctl.h>   /* ioctl, TIOCGWINSZ                */
#  include <termios.h>     /* tcgetattr, tcsetattr, struct termios */
#  include <signal.h>      /* signal, SIGWINCH                 */
#  include <sys/select.h>  /* select, fd_set                   */
#  include <fcntl.h>       /* fcntl, F_GETFL, F_SETFL, O_NONBLOCK */
#elif defined(_WIN32)
#  define TUI_WINDOWS
#  include <windows.h>     /* GetConsoleScreenBufferInfo, SetConsoleMode … */
#  include <conio.h>       /* _kbhit, _getch                   */
#else
#  error "Unsupported platform"
#endif


/* ═══════════════════════════════════════════════════════════════════════
 *  Screen-size macro  (sets size->x = cols, size->y = rows)
 *
 *  POSIX  : ioctl(STDOUT_FILENO, TIOCGWINSZ, &w)
 *  Windows: GetConsoleScreenBufferInfo(handle, &csbi)
 *
 *  On error the macro writes INFINITY so callers can detect failure.
 * ═══════════════════════════════════════════════════════════════════════ */
#ifdef TUI_POSIX
#  define TUIgetScreenSizeMacro(size) do {              \
       struct winsize _w;                               \
       if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &_w) == -1)\
           { (size)->x = (size)->y = (int)INFINITY; }   \
       else {                                           \
           (size)->x = _w.ws_col;                       \
           (size)->y = _w.ws_row;                       \
       }                                                \
   } while(0)

#elif defined(TUI_WINDOWS)
#  define TUIgetScreenSizeMacro(size) do {                              \
       CONSOLE_SCREEN_BUFFER_INFO _csbi;                                \
       HANDLE _h = GetStdHandle(STD_OUTPUT_HANDLE);                     \
       if (!GetConsoleScreenBufferInfo(_h, &_csbi))                     \
           { (size)->x = (size)->y = (int)INFINITY; }                   \
       else {                                                           \
           (size)->x = _csbi.srWindow.Right  - _csbi.srWindow.Left + 1;\
           (size)->y = _csbi.srWindow.Bottom - _csbi.srWindow.Top  + 1;\
       }                                                                \
   } while(0)
#endif


/* ═══════════════════════════════════════════════════════════════════════
 *  Key codes
 *
 *  Printable ASCII is represented as-is (32–126).
 *  Special keys use values above 256 to avoid collisions.
 * ═══════════════════════════════════════════════════════════════════════ */
typedef enum {
    TUI_KEY_UNKNOWN  = 0,

    /* Control keys (ASCII) */
    TUI_KEY_CTRL_C   = 3,
    TUI_KEY_CTRL_D   = 4,
    TUI_KEY_TAB      = 9,
    TUI_KEY_ENTER    = 13,
    TUI_KEY_ESC      = 27,
    TUI_KEY_BACKSPACE= 127,

    /* Arrow / navigation — encoded above ASCII range */
    TUI_KEY_UP       = 256,
    TUI_KEY_DOWN,
    TUI_KEY_LEFT,
    TUI_KEY_RIGHT,
    TUI_KEY_HOME,
    TUI_KEY_END,
    TUI_KEY_PAGE_UP,
    TUI_KEY_PAGE_DOWN,
    TUI_KEY_DELETE,
    TUI_KEY_INSERT,

    /* Function keys. 266~278 */
    TUI_KEY_F1, TUI_KEY_F2,  TUI_KEY_F3,  TUI_KEY_F4,
    TUI_KEY_F5, TUI_KEY_F6,  TUI_KEY_F7,  TUI_KEY_F8,
    TUI_KEY_F9, TUI_KEY_F10, TUI_KEY_F11, TUI_KEY_F12,
} TUIKey;


/* ═══════════════════════════════════════════════════════════════════════
 *  Colors  (maps to standard ANSI 4-bit palette)
 * ═══════════════════════════════════════════════════════════════════════ */
typedef enum {
    TUI_COLOR_DEFAULT = -1,

    /* Normal (dim) */
    TUI_BLACK   = 0,
    TUI_RED     = 1,
    TUI_GREEN   = 2,
    TUI_YELLOW  = 3,
    TUI_BLUE    = 4,
    TUI_MAGENTA = 5,
    TUI_CYAN    = 6,
    TUI_WHITE   = 7,

    /* Bright variants */
    TUI_BRIGHT_BLACK   = 8,
    TUI_BRIGHT_RED     = 9,
    TUI_BRIGHT_GREEN   = 10,
    TUI_BRIGHT_YELLOW  = 11,
    TUI_BRIGHT_BLUE    = 12,
    TUI_BRIGHT_MAGENTA = 13,
    TUI_BRIGHT_CYAN    = 14,
    TUI_BRIGHT_WHITE   = 15,
} TUIColor;


/* ═══════════════════════════════════════════════════════════════════════
 *  Style  (text attributes for a single print call)
 * ═══════════════════════════════════════════════════════════════════════ */
typedef struct {
    TUIColor fg;        /* foreground color  (TUI_COLOR_DEFAULT = no change) */
    TUIColor bg;        /* background color  (TUI_COLOR_DEFAULT = no change) */
    int bold;           /* 1 = bold                                          */
    int underline;      /* 1 = underline                                     */
    int italic;         /* 1 = italic  (not all terminals support this)      */
    int blink;          /* 1 = blink   (not all terminals support this)      */
    int reverse;        /* 1 = swap fg/bg                                    */
} TUIStyle;


/* ═══════════════════════════════════════════════════════════════════════
 *  ANSI escape sequence macros
 *
 *  These work on any ANSI/VT100-compatible terminal.
 *  On Windows, VT support must be enabled first — TUIinit() does this.
 *  Reference: https://en.wikipedia.org/wiki/ANSI_escape_code
 * ═══════════════════════════════════════════════════════════════════════ */

/* Cursor movement */
#define TUI_CURSOR_HOME          "\033[H"
#define TUI_CURSOR_UP(n)         "\033[" #n "A"
#define TUI_CURSOR_DOWN(n)       "\033[" #n "B"
#define TUI_CURSOR_RIGHT(n)      "\033[" #n "C"
#define TUI_CURSOR_LEFT(n)       "\033[" #n "D"
#define TUI_CURSOR_SAVE          "\033[s"
#define TUI_CURSOR_RESTORE       "\033[u"
#define TUI_CURSOR_HIDE          "\033[?25l"
#define TUI_CURSOR_SHOW          "\033[?25h"

/* Erase */
#define TUI_CLEAR_SCREEN         "\033[2J"
#define TUI_CLEAR_LINE           "\033[2K"
#define TUI_CLEAR_TO_EOL         "\033[K"
#define TUI_CLEAR_TO_BOL         "\033[1K"

/* Alternate screen buffer (saves and restores the user's scrollback) */
#define TUI_ALTSCREEN_ENTER      "\033[?1049h"
#define TUI_ALTSCREEN_EXIT       "\033[?1049l"

/* Style reset */
#define TUI_STYLE_RESET          "\033[0m"


/* ═══════════════════════════════════════════════════════════════════════
 *  Function declarations
 * ═══════════════════════════════════════════════════════════════════════ */

/* ── Init / teardown ──────────────────────────────────────────────────
 *  TUIinit    : enable VT on Windows, enter alt-screen, hide cursor
 *  TUIcleanup : reverse everything TUIinit did (call on exit/SIGINT)
 * ─────────────────────────────────────────────────────────────────── */
void TUIinit(void);
void TUIcleanup(void);

/* ── Raw mode  ────────────────────────────────────────────────────────
 *  Switches the terminal between "cooked" (line-buffered) and "raw"
 *  (character-at-a-time, no echo) mode.
 *
 *  POSIX  → learn: tcgetattr / tcsetattr  (termios.h)
 *                  flags to clear: ICANON, ECHO, ISIG …
 *  Windows→ learn: GetConsoleMode / SetConsoleMode  (windows.h)
 *                  flags to clear: ENABLE_LINE_INPUT, ENABLE_ECHO_INPUT …
 * ─────────────────────────────────────────────────────────────────── */
void TUIenableRawMode(void);
void TUIrestoreMode(void);

/* ── Screen ───────────────────────────────────────────────────────────
 *  TUIgetScreenSize : function wrapper around TUIgetScreenSizeMacro
 *  TUIclearScreen   : erase entire display and move cursor to home
 *  TUIclearLine     : erase the current line
 *  TUIenterAltScreen: switch to alternate screen buffer
 *  TUIexitAltScreen : return to normal screen buffer
 * ─────────────────────────────────────────────────────────────────── */
void TUIgetScreenSize(Vector2* out);
void TUIclearScreen(void);
void TUIclearLine(void);
void TUIenterAltScreen(void);
void TUIexitAltScreen(void);

/* ── Cursor ───────────────────────────────────────────────────────────
 *  pos: x = column (1-based), y = row (1-based)  — terminal convention
 * ─────────────────────────────────────────────────────────────────── */
void TUImoveCursor(Vector2* pos);
void TUIhideCursor(void);
void TUIshowCursor(void);
void TUIsaveCursor(void);
void TUIrestoreCursor(void);

/* ── Output ───────────────────────────────────────────────────────────
 *  TUIsetStyle   : emit ANSI codes for a TUIStyle (persistent until reset)
 *  TUIresetStyle : emit SGR reset (\033[0m)
 *  TUIprint      : print text at current cursor position, no style change
 *  TUIprintAt    : move cursor then print (convenience)
 *  TUIprintStyled: apply style, print, then reset style
 *  TUIflush      : flush stdout (call after building a frame)
 * ─────────────────────────────────────────────────────────────────── */
void TUIsetStyle(TUIStyle* style);
void TUIresetStyle(void);
void TUIprint(const char* text);
void TUIprintAt(Vector2* pos, const char* text);
void TUIprintStyled(Vector2* pos, const char* text, TUIStyle* style);
void TUIflush(void);

/* ── Input  ───────────────────────────────────────────────────────────
 *  TUIreadKey      : blocking — wait for one keypress and return its TUIKey
 *  TUIpollKey      : non-blocking — return TUI_KEY_UNKNOWN if no key ready
 *
 *  POSIX  → learn: read(STDIN_FILENO, …)  to get raw bytes
 *                  parse escape sequences (\033[A = up, etc.)
 *                  select() / poll() for non-blocking check
 *  Windows→ learn: ReadConsoleInput / PeekConsoleInput  (windows.h)
 *                  INPUT_RECORD, KEY_EVENT_RECORD
 * ─────────────────────────────────────────────────────────────────── */
TUIKey TUIreadKey(void);
TUIKey TUIpollKey(void);

/* ── Resize callback  ─────────────────────────────────────────────────
 *  Register a function to be called whenever the terminal is resized.
 *  cb receives the new size.
 *
 *  POSIX  → learn: signal(SIGWINCH, handler)  — kernel sends SIGWINCH
 *                  re-query size inside handler with ioctl/TIOCGWINSZ
 *  Windows→ learn: no equivalent signal; poll GetConsoleScreenBufferInfo
 *                  periodically or hook SetConsoleWindowInfo
 * ─────────────────────────────────────────────────────────────────── */
void TUIonResize(void (*cb)(Vector2* newSize));


#endif // TUI_H
