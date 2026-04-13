#ifndef RUNNER_H
#define RUNNER_H


/* ═══════════════════════════════════════════════════════════════════════
 *  runner.h  —  run a subprocess and capture its output
 *
 *  This is the core of PSrunner: take a compiled solution binary,
 *  feed it test-case input, record stdout/stderr, measure elapsed
 *  time, and enforce a time limit.
 *
 *  Key OS concepts you will learn when implementing runner.c:
 *    POSIX  — pipe(), fork(), dup2(), execvp(), waitpid(), SIGALRM
 *    Windows— CreatePipe(), CreateProcess(), WaitForSingleObject()
 * ═══════════════════════════════════════════════════════════════════════ */


/* ── Verdict ──────────────────────────────────────────────────────────── */

typedef enum {
    VERDICT_PENDING = 0,   /* not yet judged                              */
    VERDICT_AC,            /* Accepted         — output matches exactly   */
    VERDICT_WA,            /* Wrong Answer      — output differs          */
    VERDICT_TLE,           /* Time Limit Exceeded — killed for timeout    */
    VERDICT_RE,            /* Runtime Error     — non-zero exit code      */
    VERDICT_CE,            /* Compile Error     — reserved for future use */
} Verdict;

/* Short human-readable label: "AC", "WA", "TLE", "RE", "CE", "…" */
const char* VerdictLabel(Verdict v);


/* ── RunConfig ────────────────────────────────────────────────────────── */

typedef struct {
    const char* cmd;          /* command to execute, e.g. "./solution"     */
    const char* stdinData;    /* string piped to the child's stdin          */
    int         timeLimitMs;  /* wall-clock time limit in milliseconds      */
} RunConfig;


/* ── RunResult ────────────────────────────────────────────────────────── */

/*  Memory note:
 *    stdoutBuf and stderrBuf are heap-allocated by RunProcess().
 *    Always call RunResultFree() when you are done with a RunResult.
 */
typedef struct {
    char* stdoutBuf;   /* everything the child wrote to stdout (NUL-term) */
    char* stderrBuf;   /* everything the child wrote to stderr (NUL-term) */
    int   exitCode;    /* child's exit code  (0 = clean)                  */
    int   elapsedMs;   /* wall-clock time the child ran, in milliseconds   */
    int   timedOut;    /* 1 if we killed the child for exceeding the limit */
} RunResult;

/* Free heap buffers inside *r. Does NOT free the struct itself. */
void RunResultFree(RunResult* r);


/* ── RunProcess ───────────────────────────────────────────────────────── */

/*  Launch cfg->cmd as a child process:
 *    • pipe cfg->stdinData into its stdin
 *    • capture its stdout → out->stdoutBuf
 *    • capture its stderr → out->stderrBuf
 *    • measure elapsed wall-clock time → out->elapsedMs
 *    • kill the child and set out->timedOut=1 if cfg->timeLimitMs exceeded
 *
 *  Returns  0  on success (child ran; check out->timedOut / out->exitCode).
 *  Returns -1  if the process could not be launched (system error).
 */
int RunProcess(const RunConfig* cfg, RunResult* out);


#endif /* RUNNER_H */
