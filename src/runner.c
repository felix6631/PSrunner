#include "../header/runner.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ── Platform headers ─────────────────────────────────────────────────── */
#if defined(__APPLE__) || defined(__linux__)
#  define RUNNER_POSIX
#  include <unistd.h>      /* pipe, fork, dup2, read, write, close, execvp */
#  include <sys/wait.h>    /* waitpid, WIFEXITED, WEXITSTATUS, WNOHANG     */
#  include <sys/time.h>    /* gettimeofday                                 */
#  include <signal.h>      /* kill, SIGKILL                                */
#  include <errno.h>
#elif defined(_WIN32)
#  define RUNNER_WINDOWS
#  include <windows.h>
#endif


/* ═══════════════════════════════════════════════════════════════════════
 *  VerdictLabel
 * ═══════════════════════════════════════════════════════════════════════ */

const char* VerdictLabel(Verdict v) {
    switch (v) {
        case VERDICT_AC:      return "AC";
        case VERDICT_WA:      return "WA";
        case VERDICT_TLE:     return "TLE";
        case VERDICT_RE:      return "RE";
        case VERDICT_CE:      return "CE";
        case VERDICT_PENDING: return "...";
        default:              return "?";
    }
}


/* ═══════════════════════════════════════════════════════════════════════
 *  RunResultFree
 * ═══════════════════════════════════════════════════════════════════════ */

void RunResultFree(RunResult* r) {
    free(r->stdoutBuf); r->stdoutBuf = NULL;
    free(r->stderrBuf); r->stderrBuf = NULL;
}


/* ═══════════════════════════════════════════════════════════════════════
 *  Internal helper — read everything from a file descriptor into a
 *  heap-allocated, NUL-terminated string.
 *
 *  Returns the buffer on success, NULL on allocation failure.
 *  The caller owns the returned memory.
 *
 *  Note: uses the "doubling strategy" for dynamic buffer growth —
 *  the same idea behind std::string or ArrayList internally.
 * ═══════════════════════════════════════════════════════════════════════ */

#ifdef RUNNER_POSIX
static char* _readAll(int fd) {
    size_t cap = 4096, len = 0;
    char*  buf = malloc(cap);
    if (!buf) return NULL;

    ssize_t n;
    while ((n = read(fd, buf + len, cap - len - 1)) > 0) {
        len += (size_t)n;
        if (len + 1 >= cap) {
            cap *= 2;
            char* tmp = realloc(buf, cap);
            if (!tmp) { free(buf); return NULL; }
            buf = tmp;
        }
    }
    buf[len] = '\0';
    return buf;
}
#endif


/* ═══════════════════════════════════════════════════════════════════════
 *  RunProcess  (POSIX)
 *
 *  ┌─ YOUR TASK ─────────────────────────────────────────────────────────┐
 *  │                                                                     │
 *  │  Big picture: create a child process that runs the solution binary, │
 *  │  with stdin/stdout/stderr wired to pipes so the parent can write    │
 *  │  input and read back output.                                        │
 *  │                                                                     │
 *  │  Step 1 — Create three pipes                                        │
 *  │    int stdinPipe[2], stdoutPipe[2], stderrPipe[2];                  │
 *  │    pipe(stdinPipe);    // [0] = read end, [1] = write end           │
 *  │    pipe(stdoutPipe);                                                │
 *  │    pipe(stderrPipe);                                                │
 *  │    A pipe is a one-way byte channel: write to [1], read from [0].   │
 *  │                                                                     │
 *  │  Step 2 — Record start time                                         │
 *  │    struct timeval t0;                                               │
 *  │    gettimeofday(&t0, NULL);                                         │
 *  │                                                                     │
 *  │  Step 3 — fork()                                                    │
 *  │    pid_t pid = fork();                                              │
 *  │    fork() clones this process. Return value:                        │
 *  │      0   — you are now inside the child                             │
 *  │      >0  — you are the parent; value is the child's PID             │
 *  │      -1  — error                                                    │
 *  │                                                                     │
 *  │  Step 4 — Child side  (pid == 0)                                    │
 *  │    a) Redirect stdin from the pipe                                  │
 *  │         dup2(stdinPipe[0],  STDIN_FILENO);                          │
 *  │    b) Redirect stdout into the pipe                                 │
 *  │         dup2(stdoutPipe[1], STDOUT_FILENO);                         │
 *  │    c) Redirect stderr into the pipe                                 │
 *  │         dup2(stderrPipe[1], STDERR_FILENO);                         │
 *  │    d) Close all raw pipe file descriptors (dup2 already wired them) │
 *  │         close(stdinPipe[0]);  close(stdinPipe[1]);  ... etc.        │
 *  │    e) Replace this process image with the solution binary           │
 *  │         char* argv[] = { (char*)cfg->cmd, NULL };                   │
 *  │         execvp(argv[0], argv);                                      │
 *  │         _exit(1);  // only reached if execvp failed                 │
 *  │                                                                     │
 *  │  Step 5 — Parent side  (pid > 0)                                    │
 *  │    a) Close the child-side ends so EOF propagates correctly         │
 *  │         close(stdinPipe[0]);                                        │
 *  │         close(stdoutPipe[1]);                                       │
 *  │         close(stderrPipe[1]);                                       │
 *  │    b) Feed input to the child, then close the write end             │
 *  │         write(stdinPipe[1], cfg->stdinData, strlen(cfg->stdinData));│
 *  │         close(stdinPipe[1]);  // signals EOF to the child           │
 *  │    c) Capture output with the _readAll helper above                 │
 *  │         out->stdoutBuf = _readAll(stdoutPipe[0]);                   │
 *  │         out->stderrBuf = _readAll(stderrPipe[0]);                   │
 *  │    d) Wait for the child; enforce the time limit with a poll loop   │
 *  │         int status;                                                 │
 *  │         while (waitpid(pid, &status, WNOHANG) == 0) {               │
 *  │             usleep(10000);  // sleep 10 ms, then check again        │
 *  │             // re-compute elapsed; if over limit: kill(pid, SIGKILL)│
 *  │         }                                                           │
 *  │         WNOHANG: waitpid returns immediately if child still runs.   │
 *  │    e) Compute elapsed time                                          │
 *  │         struct timeval t1;                                          │
 *  │         gettimeofday(&t1, NULL);                                    │
 *  │         out->elapsedMs = (t1.tv_sec  - t0.tv_sec)  * 1000           │
 *  │                        + (t1.tv_usec - t0.tv_usec) / 1000;          │
 *  │    f) Extract exit code                                             │
 *  │         if (WIFEXITED(status))                                      │
 *  │             out->exitCode = WEXITSTATUS(status);                    │
 *  │                                                                     │
 *  └─────────────────────────────────────────────────────────────────────┘ */

#ifdef RUNNER_POSIX
int RunProcess(const RunConfig* cfg, RunResult* out) {
    memset(out, 0, sizeof(*out));

    /* TODO: implement steps 1-5 described above */
    (void)cfg;   /* suppress unused-parameter warning until implemented */
    return -1;
}
#endif


/* ═══════════════════════════════════════════════════════════════════════
 *  RunProcess  (Windows stub — future work)
 *
 *  Key API calls to look up:
 *    CreatePipe()              — equivalent of pipe()
 *    CreateProcess()           — equivalent of fork() + exec()
 *    WaitForSingleObject(hProcess, timeLimitMs)
 *    GetExitCodeProcess()
 *    ReadFile() on pipe handles
 * ═══════════════════════════════════════════════════════════════════════ */

#ifdef RUNNER_WINDOWS
int RunProcess(const RunConfig* cfg, RunResult* out) {
    memset(out, 0, sizeof(*out));
    (void)cfg;
    /* TODO: Windows implementation */
    return -1;
}
#endif
