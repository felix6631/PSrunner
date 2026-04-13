#include "../header/tui.h"
#include <string.h>
#include <unistd.h>

/* ═══════════════════════════════════════════════════════════════════════
 *  Internal state
 * ═══════════════════════════════════════════════════════════════════════ */

#ifdef TUI_POSIX
static struct termios _savedTermios;   /* saved before entering raw mode  */
#elif defined(TUI_WINDOWS)
static DWORD _savedConsoleMode;        /* saved before entering raw mode  */
static HANDLE _hIn;                    /* stdin handle                    */
static HANDLE _hOut;                   /* stdout handle                   */
#endif

static void (*_resizeCb)(Vector2*) = NULL;   /* user resize callback     */


/* ═══════════════════════════════════════════════════════════════════════
 *  Init / teardown
 * ═══════════════════════════════════════════════════════════════════════ */

void TUIinit(void) {
#ifdef TUI_WINDOWS
    /* Enable VT100 processing so ANSI escape codes work on Windows 10+ */
    _hIn  = GetStdHandle(STD_INPUT_HANDLE);
    _hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleMode(_hOut, &_savedConsoleMode);
    SetConsoleMode(_hOut, _savedConsoleMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif

    TUIenterAltScreen();
    TUIhideCursor();
    TUIclearScreen();
}

void TUIcleanup(void) {
    TUIresetStyle();
    TUIshowCursor();
    TUIexitAltScreen();
    TUIrestoreMode();

#ifdef TUI_WINDOWS
    SetConsoleMode(_hOut, _savedConsoleMode);
#endif
}


/* ═══════════════════════════════════════════════════════════════════════
 *  Raw mode
 *
 *  ┌─ YOUR TASK ────────────────────────────────────────────────────────┐
 *  │  POSIX                                                             │
 *  │    1. tcgetattr(STDIN_FILENO, &saved)  — snapshot current attrs    │
 *  │    2. Copy to `raw`, then clear flags:                             │
 *  │         raw.c_lflag &= ~(ICANON | ECHO | ISIG | IEXTEN)            │
 *  │         raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP)   │
 *  │         raw.c_cflag |=  CS8                                        │
 *  │         raw.c_cc[VMIN] = 1;  raw.c_cc[VTIME] = 0;                  │
 *  │    3. tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw)  — apply            │
 *  │                                                                    │
 *  │  Windows                                                           │
 *  │    1. GetConsoleMode(_hIn, &mode)                                  │
 *  │    2. Clear: ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT                 │
 *  │              ENABLE_PROCESSED_INPUT                                │
 *  │    3. SetConsoleMode(_hIn, mode)                                   │
 *  └────────────────────────────────────────────────────────────────────┘ */

void TUIenableRawMode(void) {
#ifdef TUI_POSIX
    /** Explain:
     *  
     *  1. Get Current terminal setting in _savedTermios (safe because it's static)
     *  2. Copy current setting to raw
     *  3. Set these settings
     *      - In local mode, 
     *          - Disable canonical mode (so that makes program to get input immidiatly, char by char)
     *          - Disable Echo; no one to one corresponding output for input
     *          - Disable Signal; Ctrl+Key won't make any signal on program.
     *          - Make expand control characters to pass through input stream
     *      - In input mode,
     *          - Disable terminal pause (If it's abled, Ctrl+S pauses terminal)
     *          - Use Carage Return as it is (Not as Newline)
     *          - No sending SIGINT and flush stdin/stdout when BREAK
     *          - Disable input parity check
     *          - Use char as whole 8 bit
     *      - In Control mode,
     *          - Set character size 8 bit
     *      - In Control Characters Array,
     *          - Get input for every character
     *          - Set input timeout 0.1 * 0 second.
     */
    struct termios raw;
    tcgetattr(STDIN_FILENO, &_savedTermios);
    raw = _savedTermios;

    raw.c_lflag &= ~(ICANON | ECHO | ISIG | IEXTEN);
    raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
    raw.c_cflag |= CS8;
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
#endif


    /* TODO: implement it for Windows */
#ifdef TUI_WINDOWS

#endif // TUI_WINDOWS
}

void TUIrestoreMode(void) {
    /* TODO: restore the saved terminal state                        */
    /* POSIX  : tcsetattr(STDIN_FILENO, TCSAFLUSH, &_savedTermios)  */
    /* --> on stdin, discard remain input and set back to saved options */
#ifdef TUI_POSIX
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &_savedTermios);
#endif
    /* Windows: SetConsoleMode(_hIn, savedInputMode)                */
}


/* ═══════════════════════════════════════════════════════════════════════ 
 *  Screen
 * ═══════════════════════════════════════════════════════════════════════ */

void TUIgetScreenSize(Vector2* out) {
    TUIgetScreenSizeMacro(out);
}

void TUIclearScreen(void) {
    fputs(TUI_CLEAR_SCREEN TUI_CURSOR_HOME, stdout);
}

void TUIclearLine(void) {
    fputs(TUI_CLEAR_LINE, stdout);
}

void TUIenterAltScreen(void) {
    fputs(TUI_ALTSCREEN_ENTER, stdout);
}

void TUIexitAltScreen(void) {
    fputs(TUI_ALTSCREEN_EXIT, stdout);
}


/* ═══════════════════════════════════════════════════════════════════════
 *  Cursor
 * ═══════════════════════════════════════════════════════════════════════ */

void TUImoveCursor(Vector2* pos) {
    /* Terminal rows/cols are 1-based */
    printf("\033[%d;%dH", pos->y, pos->x);
}

void TUIhideCursor(void) {
    fputs(TUI_CURSOR_HIDE, stdout);
}

void TUIshowCursor(void) {
    fputs(TUI_CURSOR_SHOW, stdout);
}

void TUIsaveCursor(void) {
    fputs(TUI_CURSOR_SAVE, stdout);
}

void TUIrestoreCursor(void) {
    fputs(TUI_CURSOR_RESTORE, stdout);
}


/* ═══════════════════════════════════════════════════════════════════════
 *  Output
 * ═══════════════════════════════════════════════════════════════════════ */

void TUIsetStyle(TUIStyle* s) {
    /* SGR (Select Graphic Rendition): \033[ <params> m
     * Params are semicolon-separated integers.
     * Reference: https://en.wikipedia.org/wiki/ANSI_escape_code#SGR */
    fputs("\033[", stdout);

    int first = 1;
#define _SEP() do { if (!first) fputc(';', stdout); first = 0; } while(0)

    if (s->bold)      { _SEP(); fputs("1", stdout); }
    if (s->italic)    { _SEP(); fputs("3", stdout); }
    if (s->underline) { _SEP(); fputs("4", stdout); }
    if (s->blink)     { _SEP(); fputs("5", stdout); }
    if (s->reverse)   { _SEP(); fputs("7", stdout); }

    if (s->fg != TUI_COLOR_DEFAULT) {
        _SEP();
        if (s->fg < 8)  printf("3%d",   s->fg);        /* \033[30–37m  */
        else            printf("9%d",   s->fg - 8);     /* \033[90–97m  */
    }

    if (s->bg != TUI_COLOR_DEFAULT) {
        _SEP();
        if (s->bg < 8)  printf("4%d",   s->bg);        /* \033[40–47m  */
        else            printf("10%d",  s->bg - 8);     /* \033[100–107m*/
    }

#undef _SEP
    fputc('m', stdout);
}

void TUIresetStyle(void) {
    fputs(TUI_STYLE_RESET, stdout);
}

void TUIprint(const char* text) {
    fputs(text, stdout);
}

void TUIprintAt(Vector2* pos, const char* text) {
    TUImoveCursor(pos);
    fputs(text, stdout);
}

void TUIprintStyled(Vector2* pos, const char* text, TUIStyle* style) {
    TUImoveCursor(pos);
    TUIsetStyle(style);
    fputs(text, stdout);
    TUIresetStyle();
}

void TUIflush(void) {
    fflush(stdout);
}


/* ═══════════════════════════════════════════════════════════════════════
 *  Input
 *
 *  ┌─ YOUR TASK ───────────────────────────────────────────────────────────────┐
 *  │  Call TUIenableRawMode() before using these functions.                    │
 *  │                                                                           │
 *  │  POSIX — read raw bytes from stdin:                                       │
 *  │    char c;                                                                │
 *  │    read(STDIN_FILENO, &c, 1)          — single byte                       │
 *  │    If c == '\033', read two more bytes to detect escape sequences:        │
 *  │      "\033[A" = up, "\033[B" = down, "\033[C" = right, "\033[D" = left    │
 *  │      "\033[H" = home, "\033[F" = end, "\033[2~" = ins, "\033[3~" = del    │
 *  │    For non-blocking: use select()/poll() before read()                    │
 *  │      or set O_NONBLOCK on fd with fcntl()                                 │
 *  │                                                                           │
 *  │  Windows — use console input API:                                         │
 *  │    ReadConsoleInput(_hIn, &rec, 1, &count)                                │
 *  │    Check rec.EventType == KEY_EVENT                                       │
 *  │    Use rec.Event.KeyEvent.wVirtualKeyCode for special keys                │
 *  │    Use rec.Event.KeyEvent.uChar.AsciiChar for printable chars             │
 *  │    For non-blocking: PeekConsoleInput() first, or _kbhit()                │      
 *  └───────────────────────────────────────────────────────────────────────────┘ */

TUIKey TUIreadKey(void) {
    /* TODO: implement blocking key read — see guide above */
    return TUI_KEY_UNKNOWN;
}

TUIKey TUIpollKey(void) {
    /* TODO: implement non-blocking key poll — see guide above */
    return TUI_KEY_UNKNOWN;
}


/* ═══════════════════════════════════════════════════════════════════════
 *  Resize callback
 *
 *  ┌─ YOUR TASK ────────────────────────────────────────────────────────┐
 *  │  POSIX                                                             │
 *  │    When the terminal is resized, the kernel sends SIGWINCH.        │
 *  │    Register a handler: signal(SIGWINCH, _sigwinchHandler)          │
 *  │    Inside the handler, call TUIgetScreenSize() and invoke _resizeCb│
 *  │    Note: only async-signal-safe functions are safe in a handler;   │
 *  │    a common pattern is to write(pipe) from the handler and         │
 *  │    process the resize in the main loop via select().               │
 *  │                                                                    │
 *  │  Windows                                                           │
 *  │    No SIGWINCH equivalent. Options:                                │
 *  │    a) Poll GetConsoleScreenBufferInfo() in your render loop        │
 *  │    b) Use ReadConsoleInput() — it emits WINDOW_BUFFER_SIZE_EVENT   │
 *  └────────────────────────────────────────────────────────────────────┘ */

#ifdef TUI_POSIX
static void _sigwinchHandler(int sig) {
    (void)sig;
    if (_resizeCb) {
        Vector2 size;
        TUIgetScreenSizeMacro(&size);
        _resizeCb(&size);
    }
}
#endif

void TUIonResize(void (*cb)(Vector2* newSize)) {
    _resizeCb = cb;
#ifdef TUI_POSIX
    /* TODO: register _sigwinchHandler — see guide above */
    /* signal(SIGWINCH, _sigwinchHandler); */
#endif
    /* Windows: nothing to register here; poll in your loop */
}
