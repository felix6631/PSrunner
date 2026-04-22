#include "../header/loader.h"
#include "../header/problem.h"
#include "../header/runner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#  include <windows.h>
#  define SEP "\\"
#  define getpid() ((int)GetCurrentProcessId())
#else
#  include <unistd.h>
#  include <limits.h>
#  define SEP "/"
#endif

#ifndef PATH_MAX
#  define PATH_MAX 4096
#endif

/* ── Display helpers ──────────────────────────────────────────────────── */

static void printDivider(void) {
    puts("────────────────────────────────");
}

static const char* verdictColor(Verdict v) {
    switch (v) {
        case VERDICT_AC:  return "\033[32m";   /* green  */
        case VERDICT_WA:  return "\033[31m";   /* red    */
        case VERDICT_TLE: return "\033[33m";   /* yellow */
        case VERDICT_RE:  return "\033[35m";   /* magenta*/
        default:          return "\033[0m";
    }
}

/* ── Entry point ──────────────────────────────────────────────────────── */

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: psrunner <answer_file> <problem_number>\n");
        fprintf(stderr, "  answer_file    path to the source file (.c or .cpp)\n");
        fprintf(stderr, "  problem_number directory name under the psrunner binary's location\n");
        return 1;
    }

    const char* answerFile = argv[1];
    const char* problemNum = argv[2];

    /* ── locate test-case directory: <exe_dir>/<problem_number>/ ── */
    char exeDir[PATH_MAX];
    if (GetExeDir(exeDir, sizeof(exeDir)) != 0) {
        fprintf(stderr, "error: could not determine executable directory\n");
        return 1;
    }

    char problemDir[PATH_MAX];
    snprintf(problemDir, sizeof(problemDir), "%s" SEP "%s", exeDir, problemNum);

    /* ── build a temp path for the compiled binary ── */
    char binPath[PATH_MAX];
#ifdef _WIN32
    snprintf(binPath, sizeof(binPath),
             "%s" SEP "psrunner_tmp_%d.exe", exeDir, getpid());
#else
    snprintf(binPath, sizeof(binPath), "/tmp/psrunner_%d", getpid());
#endif

    /* ── compile ── */
    printf("Compiling %s ... ", answerFile);
    fflush(stdout);

    char ceMsg[8192] = {0};
    if (CompileSolution(answerFile, binPath, ceMsg, sizeof(ceMsg)) != 0) {
        printf("\033[31mCompile Error\033[0m\n");
        if (ceMsg[0]) printf("%s\n", ceMsg);
        return 1;
    }
    printf("\033[32mOK\033[0m\n\n");

    /* ── load test cases ── */
    Problem* p = LoadProblem(problemDir, 2000);
    if (!p) {
        fprintf(stderr,
                "error: no test cases found in '%s'\n"
                "       (expected files named 1.in/1.out, 2.in/2.out, …)\n",
                problemDir);
        remove(binPath);
        return 1;
    }

    /* ── grade ── */
    ProblemResult* results = malloc((size_t)p->caseCount * sizeof(ProblemResult));
    if (!results) {
        ProblemFree(p);
        remove(binPath);
        return 1;
    }

    printf("Problem %-6s  (%d test case%s)\n",
           problemNum, p->caseCount, p->caseCount == 1 ? "" : "s");
    printDivider();

    int ac = GradeProblem(binPath, p, results);

    for (int i = 0; i < p->caseCount; i++) {
        Verdict v = results[i].verdict;
        printf("  Case %2d : %s%-4s\033[0m  %dms",
               i + 1, verdictColor(v), VerdictLabel(v), results[i].elapsedMs);

        /* for WA, hint that input can be inspected */
        if (v == VERDICT_WA)
            printf("  (output mismatch)");
        if (v == VERDICT_RE)
            printf("  (non-zero exit)");

        putchar('\n');
    }

    printDivider();
    printf("Result: %d / %d\n", ac, p->caseCount);

    int allPassed = (ac == p->caseCount);

    /* ── cleanup ── */
    free(results);
    ProblemFree(p);
    remove(binPath);

    return allPassed ? 0 : 1;
}
