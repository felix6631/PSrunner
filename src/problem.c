#include "../header/problem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>   /* isspace */


/* ═══════════════════════════════════════════════════════════════════════
 *  TestCase  —  construction / destruction
 * ═══════════════════════════════════════════════════════════════════════ */

TestCase* TestCaseMake(const char* input, const char* expected, int timeLimitMs) {
    TestCase* tc = malloc(sizeof(TestCase));
    if (!tc) return NULL;

    tc->inputStr       = strdup(input);
    tc->expectedOutput = strdup(expected);
    tc->timeLimitMs    = timeLimitMs;

    if (!tc->inputStr || !tc->expectedOutput) {
        free(tc->inputStr);
        free(tc->expectedOutput);
        free(tc);
        return NULL;
    }
    return tc;
}

void TestCaseFree(TestCase* tc) {
    if (!tc) return;
    free(tc->inputStr);
    free(tc->expectedOutput);
    free(tc);
}


/* ═══════════════════════════════════════════════════════════════════════
 *  Problem  —  construction / destruction
 * ═══════════════════════════════════════════════════════════════════════ */

Problem* ProblemMake(const char* name) {
    Problem* p = malloc(sizeof(Problem));
    if (!p) return NULL;

    p->name      = strdup(name);
    p->cases     = NULL;
    p->caseCount = 0;

    if (!p->name) { free(p); return NULL; }
    return p;
}

int ProblemAddCase(Problem* p, TestCase* tc) {
    /* Grow the cases array by one slot */
    TestCase** tmp = realloc(p->cases, (size_t)(p->caseCount + 1) * sizeof(TestCase*));
    if (!tmp) return -1;

    p->cases = tmp;
    p->cases[p->caseCount++] = tc;
    return 0;
}

void ProblemFree(Problem* p) {
    if (!p) return;
    for (int i = 0; i < p->caseCount; i++)
        TestCaseFree(p->cases[i]);
    free(p->cases);
    free(p->name);
    free(p);
}


/* ═══════════════════════════════════════════════════════════════════════
 *  OutputMatches
 *
 *  ┌─ YOUR TASK ─────────────────────────────────────────────────────────┐
 *  │                                                                     │
 *  │  Compare two strings after normalising whitespace, the same way     │
 *  │  most online judges do it. Return 1 if they match, 0 if not.        │
 *  │                                                                     │
 *  │  Normalisation rules:                                               │
 *  │    1. Treat \r\n the same as \n  (Windows line endings)             │
 *  │    2. Strip trailing spaces/tabs from each line                     │
 *  │    3. Ignore trailing blank lines at the end of the string          │
 *  │                                                                     │
 *  │  Suggested approach (PS-style, no extra allocation):                │
 *  │    Walk both strings in parallel with two pointers (pa, pe).        │
 *  │    At each step, skip trailing whitespace on the current line,      │
 *  │    then compare characters until \n or \0.                          │
 *  │                                                                     │
 *  │  Useful C functions:                                                │
 *  │    isspace(c)     — true for ' ', '\t', '\r', '\n', etc.            │
 *  │    strchr(s, c)   — find next occurrence of character c in s        │
 *  │    strncmp(a,b,n) — compare at most n characters                    │
 *  │                                                                     │
 *  └─────────────────────────────────────────────────────────────────────┘ */

int OutputMatches(const char* actual, const char* expected) {
    const char* a = actual;
    const char* e = expected;

    while (1) {
        /* find the end of each current line (stop at \n or \0) */
        const char* aEnd = a; while (*aEnd && *aEnd != '\n') aEnd++;
        const char* eEnd = e; while (*eEnd && *eEnd != '\n') eEnd++;

        /* trim trailing spaces, tabs, and \r from each line */
        const char* aTrim = aEnd;
        while (aTrim > a && (*(aTrim-1) == ' ' || *(aTrim-1) == '\t' || *(aTrim-1) == '\r'))
            aTrim--;
        const char* eTrim = eEnd;
        while (eTrim > e && (*(eTrim-1) == ' ' || *(eTrim-1) == '\t' || *(eTrim-1) == '\r'))
            eTrim--;

        int aLen = (int)(aTrim - a);
        int eLen = (int)(eTrim - e);

        if (aLen != eLen || (aLen > 0 && strncmp(a, e, (size_t)aLen) != 0))
            return 0;

        /* advance past the newline */
        a = aEnd; if (*a == '\n') a++;
        e = eEnd; if (*e == '\n') e++;

        /* check whether all remaining content on each side is blank lines only */
        int aDone = 1, eDone = 1;
        const char* p;
        for (p = a; *p; p++)
            if (*p != '\n' && *p != '\r' && *p != ' ' && *p != '\t') { aDone = 0; break; }
        for (p = e; *p; p++)
            if (*p != '\n' && *p != '\r' && *p != ' ' && *p != '\t') { eDone = 0; break; }

        if (aDone && eDone) return 1;
        if (aDone != eDone) return 0;   /* one side has more content */
    }
}


/* ═══════════════════════════════════════════════════════════════════════
 *  GradeTestCase
 *
 *  ┌─ YOUR TASK ─────────────────────────────────────────────────────────┐
 *  │                                                                     │
 *  │  Tie RunProcess and OutputMatches together for one test case.       │
 *  │                                                                     │
 *  │  1. Build a RunConfig:                                              │
 *  │       RunConfig cfg;                                                │
 *  │       cfg.cmd         = solutionCmd;                                │
 *  │       cfg.stdinData   = tc->inputStr;                               │
 *  │       cfg.timeLimitMs = tc->timeLimitMs;                            │
 *  │                                                                     │
 *  │  2. Run it:                                                         │
 *  │       RunResult run;                                                │
 *  │       if (RunProcess(&cfg, &run) == -1) return -1;                  │
 *  │                                                                     │
 *  │  3. Determine verdict — check in this priority order:               │
 *  │       run.timedOut  → VERDICT_TLE                                   │
 *  │       run.exitCode != 0 → VERDICT_RE                                │
 *  │       OutputMatches(run.stdoutBuf, tc->expectedOutput)              │
 *  │                    → VERDICT_AC  or  VERDICT_WA                    │
 *  │                                                                     │
 *  │  4. Fill result->verdict and result->elapsedMs.                     │
 *  │                                                                     │
 *  │  5. Always call RunResultFree(&run) before returning.               │
 *  │                                                                     │
 *  └─────────────────────────────────────────────────────────────────────┘ */

int GradeTestCase(const char*    solutionCmd,
                  const TestCase* tc,
                  ProblemResult*  result) {
    result->verdict   = VERDICT_PENDING;
    result->elapsedMs = 0;

    RunConfig cfg;
    cfg.cmd         = solutionCmd;
    cfg.stdinData   = tc->inputStr;
    cfg.timeLimitMs = tc->timeLimitMs;

    RunResult run;
    if (RunProcess(&cfg, &run) != 0) return -1;

    result->elapsedMs = run.elapsedMs;

    if (run.timedOut) {
        result->verdict = VERDICT_TLE;
    } else if (run.exitCode != 0) {
        result->verdict = VERDICT_RE;
    } else if (OutputMatches(run.stdoutBuf ? run.stdoutBuf : "", tc->expectedOutput)) {
        result->verdict = VERDICT_AC;
    } else {
        result->verdict = VERDICT_WA;
    }

    RunResultFree(&run);
    return 0;
}


/* ═══════════════════════════════════════════════════════════════════════
 *  GradeProblem  —  implemented (calls GradeTestCase in a loop)
 * ═══════════════════════════════════════════════════════════════════════ */

int GradeProblem(const char*    solutionCmd,
                 const Problem* p,
                 ProblemResult* results) {
    int accepted = 0;
    for (int i = 0; i < p->caseCount; i++) {
        results[i].caseIndex = i;
        if (GradeTestCase(solutionCmd, p->cases[i], &results[i]) == 0)
            if (results[i].verdict == VERDICT_AC) accepted++;
    }
    return accepted;
}

#ifdef DEBUG_PROBLEM
int main(void) {
    puts("Debug INFO");
    printf("Source : %s\n",__FILE__);
    printf("Compiled : %s at %s\n",__DATE__,__TIME__);
    puts("OutputMatches Test");
    printf("Test 1: %s\n",1 == OutputMatches("010 10 1 2 3", "010 10 1 2 3") ? "Pass" : "Fail");
    printf("Test 2: %s\n",0 == OutputMatches("010 10 1 2 3", "010 11 3 2 1") ? "Pass" : "Fail");
    printf("Test 3: %s\n",0 == OutputMatches("010 10 1 2 3", "1234 567 891") ? "Pass" : "Fail");
    return 0;
}
#endif // DEBUG_PROBLEM
