# PSrunner — Agent Guide

## What This Project Is

PSrunner is a **program written in pure C** for grading algorithm/problem-solving submissions. It targets macOS, Linux, and Windows.

The project has **two goals that run in parallel**:
- Build a working PS-grader CLI program from scratch


## Project Structure

```
PSrunner/
├── CLAUDE.md           # This file
├── Makefile            # Empty (build system not yet defined)
├── README.md           # One-liner description
├── a.out               # Compiled binary artifact (do not edit)
├── header/
│   ├── angle.h         # Angle type alias (typedef double Angle)
│   ├── rect.h          # Rect struct + layout helper declarations
│   ├── loader.h        # GetExeDir, CompileSolution, LoadProblem — CLI support module
│   ├── tui.h           # Full TUI interface — types, macros, declarations (learning module, not used in main CLI)
│   ├── vector2.h       # Vector2 struct + all function declarations
│   ├── runner.h        # RunProcess, RunResult, Verdict — process execution
│   └── problem.h       # TestCase, Problem, grading pipeline declarations
└── src/
    ├── main.c          # CLI entry point — arg parsing, compile, grade, print results
    ├── loader.c        # GetExeDir, CompileSolution, LoadProblem
    ├── rect.c          # Rect geometry implementation (complete — read as reference)
    ├── tui.c           # TUI implementation (learning module — not part of CLI flow)
    ├── vector2.c       # Vector2 implementation (complete)
    ├── runner.c        # Process execution (RunProcess complete for POSIX)
    └── problem.c       # Grading pipeline (OutputMatches + GradeTestCase complete)
```

---

## Key Design Decisions

- **Headers live in `header/`**; source files include via `../header/`.
- **OOP-inspired style**: each module exposes free functions (`V2shift`, …) and, where applicable, wires those same functions into struct method pointers so callers can use either style.
- **`Vector2` is heap-allocated** via `V2create()` / `V2delete()`. `volatile int heapCounterVector2` tracks live allocations (manual memory tracking, not a leak detector).
- **`DEBUG_VECTOR2` macro** compiles a standalone `main()` in `vector2.c` for module-level testing.
- **Platform detection** via `RUNNER_POSIX` / `RUNNER_WINDOWS` macros defined locally in `runner.c`. All platform-specific includes are gated behind these.
- **No external libraries** — stdlib only (`stdio.h`, `stdlib.h`, `math.h`, POSIX/Windows syscall headers).

---

## Module Status

### `vector2` — Complete

`Vector2` is a 2D integer tuple (`int x, y`) with full OOP-style method pointers and matching free functions.

| Function | Description |
|---|---|
| `V2create()` / `V2createXY(x,y)` | Heap-allocate; wires all method pointers |
| `V2delete(v)` | Free + decrement heap counter |
| `V2shift(v, t)` | Translate by another vector |
| `V2rotate(v, angle)` | Rotate in-place (standard rotation matrix, rounded) |
| `V2scale(v, scalar)` | Scalar multiply (rounded) |
| `V2negate(v)` | `(-x, -y)` |
| `V2zero(v)` | Reset to `(0, 0)` |
| `V2copy(dst, src)` | Copy components |
| `V2reflectOrigin(v)` | Point reflection: `(-x, -y)` |
| `V2reflectXAxis(v)` | Flip y |
| `V2reflectYAxis(v)` | Flip x |
| `V2reflectDiagonal(v)` | Swap x and y (reflection over y=x) |
| `V2magnitude(v)` | Euclidean length (returns `double`) |
| `V2dot(v, u)` | Dot product (returns `double`) |
| `V2distance(v, u)` | Distance between two vectors (returns `double`) |
| `V2equals(v, u)` | Component equality (returns `int`) |
| `V2print(v)` | `printf("Vector2(%d, %d)\n", …)` |

---

### `rect` — Complete (read as reference, not used in CLI flow)

`Rect` is an axis-aligned rectangle (`int x, y, w, h`). There are no TODOs here — `rect.c` is meant to be read as a reference for C struct usage and simple coordinate geometry.

| Function | Description |
|---|---|
| `RectMake(x,y,w,h)` | Construct a Rect |
| `RectFromCorners(x0,y0,x1,y1)` | Smallest rect covering two corners (normalises order) |
| `RectContains(r, cx, cy)` | Point-in-rect test |
| `RectIntersects(a, b)` | Overlap test (standard interval check on both axes) |
| `RectIntersection(a, b)` | Clipped rectangle of the overlap region |
| `RectSplitH(r, at, top, bot)` | Split horizontally: top gets `at` rows, bot gets the rest |
| `RectSplitV(r, at, left, right)` | Split vertically: left gets `at` columns, right gets the rest |
| `RectInset(r, margin)` | Shrink on all sides — use to get content area inside a border |

`Rect` is a coordinate geometry primitive. It was originally a layout building block for the TUI; now it serves as a reference module only.

---

### `tui` — Learning module (not part of CLI flow)

The TUI module is kept as a standalone learning exercise in terminal/syscall concepts. The main program no longer uses it.

#### Fully implemented (ANSI-based, cross-platform)

| Group | Functions |
|---|---|
| Init/teardown | `TUIinit`, `TUIcleanup` |
| Screen | `TUIgetScreenSize`, `TUIclearScreen`, `TUIclearLine`, `TUIenterAltScreen`, `TUIexitAltScreen` |
| Cursor | `TUImoveCursor`, `TUIhideCursor`, `TUIshowCursor`, `TUIsaveCursor`, `TUIrestoreCursor` |
| Output | `TUIsetStyle`, `TUIresetStyle`, `TUIprint`, `TUIprintAt`, `TUIprintStyled`, `TUIflush` |

`TUIsetStyle` builds a full SGR (`\033[…m`) parameter string from a `TUIStyle` struct (`fg`, `bg`, `bold`, `underline`, `italic`, `blink`, `reverse`). Colors map to the standard ANSI 4-bit palette via `TUIColor` enum.

#### Left as stubs — author's learning tasks (optional; not required for CLI)

**Do not implement these as part of the main program.** Each has a detailed guide comment in `tui.c`.

| Function | POSIX concept to learn | Windows concept to learn |
|---|---|---|
| `TUIenableRawMode` | `tcgetattr` + `tcsetattr`, clear `ICANON`/`ECHO`/`ISIG` | `GetConsoleMode` + `SetConsoleMode` |
| `TUIrestoreMode` | `tcsetattr(TCSAFLUSH, &_savedTermios)` | restore saved `DWORD` mode |
| `TUIreadKey` | `read()` + parse `\033[` escape sequences | `ReadConsoleInput` + `KEY_EVENT_RECORD` |
| `TUIpollKey` | `select()` / `fcntl(O_NONBLOCK)` | `PeekConsoleInput` / `_kbhit()` |
| `TUIonResize` | `signal(SIGWINCH, handler)` | poll `GetConsoleScreenBufferInfo` |

#### Key types in `tui.h`

- `TUIKey` — enum of key codes (printable ASCII as-is, special keys encoded above 256)
- `TUIColor` — 16-color ANSI palette (`TUI_BLACK` … `TUI_BRIGHT_WHITE`, `TUI_COLOR_DEFAULT`)
- `TUIStyle` — struct: `fg`, `bg`, `bold`, `underline`, `italic`, `blink`, `reverse`
- `TUIgetScreenSizeMacro(size)` — low-level macro wrapping `ioctl`/`TIOCGWINSZ` (POSIX) or `GetConsoleScreenBufferInfo` (Windows)

---

### `runner` — Stub (core learning task)

This module runs a compiled solution binary as a child process, captures its output, and enforces a time limit. It is the mechanical heart of the grader.

`runner.h` declares:
- `Verdict` enum — `AC`, `WA`, `TLE`, `RE`, `CE`, `PENDING`
- `VerdictLabel(v)` — short string label (implemented)
- `RunConfig` — `cmd`, `stdinData`, `timeLimitMs`
- `RunResult` — `stdoutBuf`, `stderrBuf`, `exitCode`, `elapsedMs`, `timedOut`
- `RunResultFree(r)` — frees heap buffers (implemented)
- `RunProcess(cfg, out)` — **the main TODO**

#### `RunProcess` — author's learning task

The full step-by-step guide is in `runner.c`. The POSIX implementation requires:

```
pipe() × 3  →  fork()  →  child: dup2() + execvp()
                         parent: write stdin, _readAll stdout/stderr,
                                 poll waitpid() with time limit, kill() if TLE
```

Key syscalls to learn: `pipe`, `fork`, `dup2`, `execvp`, `waitpid`, `WNOHANG`, `kill`, `gettimeofday`.

`_readAll(fd)` (a private helper in `runner.c`) is already implemented and shows how to read from a file descriptor into a dynamically growing buffer.

---

### `problem` — Partially stubbed

This module glues test-case data to the runner and produces verdicts.

#### Fully implemented

| Function | What it does |
|---|---|
| `TestCaseMake(input, expected, limitMs)` | `strdup` both strings, heap-allocate `TestCase` |
| `TestCaseFree(tc)` | free strings + struct |
| `ProblemMake(name)` | heap-allocate `Problem` with empty `cases[]` |
| `ProblemAddCase(p, tc)` | `realloc` the `cases[]` array and append |
| `ProblemFree(p)` | free all `TestCase`s, the array, the name, and the struct |
| `GradeProblem(cmd, p, results)` | loop over all test cases, call `GradeTestCase`, count AC |

#### Left as stubs — author's learning tasks

| Function | What to implement |
|---|---|
| `OutputMatches(actual, expected)` | Walk both strings, ignore trailing whitespace per line and trailing blank lines. Uses `isspace`, `strchr`, `strncmp`. |
| `GradeTestCase(cmd, tc, result)` | Build `RunConfig`, call `RunProcess`, determine verdict from `timedOut`/`exitCode`/`OutputMatches`. |

---

### `loader` — Complete

Bridges the OS and the grading pipeline.

| Function | What it does |
|---|---|
| `GetExeDir(buf, size)` | Fills `buf` with the directory of the running binary (Linux: `/proc/self/exe`, macOS: `_NSGetExecutablePath`, Windows: `GetModuleFileName`) |
| `CompileSolution(src, out, errMsg, errMsgSize)` | Shells out to `gcc`/`g++`, captures compiler output, returns 0 on success |
| `LoadProblem(dir, limitMs)` | Scans `dir` for `N.in`/`N.out` pairs, builds and returns a `Problem*` |

---

### `main.c` — Complete

```
Usage: psrunner <answer_file> <problem_number>
```

- Calls `GetExeDir` to find where the binary lives
- Looks for test cases in `<exe_dir>/<problem_number>/` (files `1.in`/`1.out`, `2.in`/`2.out`, …)
- Compiles `<answer_file>` with `gcc`/`g++` to a temp binary
- Runs `GradeProblem` and prints a colour-coded result table
- Exits 0 if all cases pass, 1 otherwise

---

## Building

Makefile is empty. Compile manually:

```sh
# Full CLI build (macOS/Linux)
gcc src/main.c src/loader.c src/vector2.c src/runner.c src/problem.c -lm -o psrunner

# vector2 standalone test
gcc -DDEBUG_VECTOR2 src/vector2.c -lm -o vector2_test

# problem module standalone test
gcc -DDEBUG_PROBLEM src/problem.c src/runner.c src/vector2.c -lm -o problem_test

# TUI learning module (separate; not linked into main binary)
gcc src/tui.c src/vector2.c src/rect.c -lm -o tui_test
```

Expected warnings on a full build before the stubs are filled in:
- `signal.h` included but not yet used (runner.c — needed for `kill`/`SIGKILL`)

---

## Recommended Implementation Order

The stubs are designed to be tackled roughly in this order:

1. **`OutputMatches`** — pure string processing, no syscalls. Good warm-up.
2. **`GradeTestCase`** — connects `RunProcess` to the verdict logic. Mechanical once `RunProcess` works.
3. **`RunProcess`** — the hardest one: pipes + fork + exec + time-limiting.
4. **`main.c`** — wire everything together into a working CLI grader (parse args, load test cases, call `GradeProblem`, print results).

TUI stubs (`TUIenableRawMode`, `TUIreadKey`, `TUIpollKey`, `TUIonResize`) are optional learning exercises and are not on the critical path.
