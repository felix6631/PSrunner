#ifndef PROBLEM_H
#define PROBLEM_H

#include "runner.h"   /* Verdict, RunResult, RunConfig */


/* ═══════════════════════════════════════════════════════════════════════
 *  TestCase  —  one input/expected-output pair
 *
 *  Memory: inputStr and expectedOutput are owned by the TestCase.
 *          Always call TestCaseFree() when done.
 * ═══════════════════════════════════════════════════════════════════════ */
typedef struct {
    char* inputStr;        /* string fed to the solution's stdin          */
    char* expectedOutput;  /* string the solution must produce on stdout  */
    int   timeLimitMs;     /* wall-clock time limit for this test case    */
} TestCase;

/* Allocate and return a TestCase. Strings are strdup'd (heap-copied).
 * Returns NULL on allocation failure. */
TestCase* TestCaseMake(const char* input, const char* expected, int timeLimitMs);

/* Free all memory owned by *tc, then free tc itself. */
void TestCaseFree(TestCase* tc);


/* ═══════════════════════════════════════════════════════════════════════
 *  Problem  —  a named collection of test cases
 *
 *  Memory: name is owned by Problem; cases[] are owned by Problem.
 *          Always call ProblemFree() when done.
 * ═══════════════════════════════════════════════════════════════════════ */
typedef struct {
    char*      name;        /* human-readable problem name e.g. "A. Hello" */
    TestCase** cases;       /* heap array of TestCase pointers              */
    int        caseCount;   /* number of elements in cases[]                */
} Problem;

/* Allocate and return an empty Problem with the given name.
 * Returns NULL on allocation failure. */
Problem* ProblemMake(const char* name);

/* Append a TestCase to p->cases[]. Takes ownership of tc.
 * Returns 0 on success, -1 on allocation failure. */
int ProblemAddCase(Problem* p, TestCase* tc);

/* Free all memory owned by *p, then free p itself. */
void ProblemFree(Problem* p);


/* ═══════════════════════════════════════════════════════════════════════
 *  ProblemResult  —  result of grading one test case
 * ═══════════════════════════════════════════════════════════════════════ */
typedef struct {
    int     caseIndex;   /* which test case (0-based)                    */
    Verdict verdict;     /* AC / WA / TLE / RE                           */
    int     elapsedMs;   /* wall-clock time the solution ran             */
} ProblemResult;


/* ── Output comparison ────────────────────────────────────────────────── */

/*  Return 1 if `actual` matches `expected` after normalisation, else 0.
 *
 *  Normalisation rules (same as most online judges):
 *    • Trailing whitespace on each line is ignored
 *    • Trailing blank lines at end of output are ignored
 *    • Line endings: \r\n and \n are treated identically
 *
 *  Learn: this is a string-processing function — use the C string API:
 *    strlen, strchr, strncmp, isspace (ctype.h)
 */
int OutputMatches(const char* actual, const char* expected);


/* ── Grading ──────────────────────────────────────────────────────────── */

/*  Grade a single test case:
 *    1. Build a RunConfig from tc and solutionCmd
 *    2. Call RunProcess()
 *    3. Determine the Verdict (TLE → timedOut, RE → exitCode!=0, else compare)
 *    4. Fill *result and return 0. Return -1 if RunProcess itself failed.
 */
int GradeTestCase(const char*   solutionCmd,
                  const TestCase* tc,
                  ProblemResult*  result);

/*  Grade all test cases in p. Writes results into the caller-supplied
 *  array `results` (must have room for at least p->caseCount entries).
 *  Returns the number of Accepted test cases.
 */
int GradeProblem(const char*    solutionCmd,
                 const Problem* p,
                 ProblemResult* results);


#endif /* PROBLEM_H */
