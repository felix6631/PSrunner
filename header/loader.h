#ifndef LOADER_H
#define LOADER_H

#include "problem.h"
#include <stddef.h>   /* size_t */

/* Fill buf with the directory that contains the running executable.
 * No trailing separator. Returns 0 on success, -1 on failure. */
int GetExeDir(char* buf, size_t size);

/* Compile srcPath (.c → gcc, .cpp/.cc/.cxx → g++) to a binary at outPath.
 * Compiler output is written into errMsg (up to errMsgSize-1 bytes, NUL-terminated).
 * Returns 0 on success, -1 on compile error or system failure. */
int CompileSolution(const char* srcPath, const char* outPath,
                    char* errMsg, size_t errMsgSize);

/* Load test cases from problemDir, which must contain files named
 * N.in / N.out for consecutive integers N (e.g. 1.in, 1.out, 2.in, 2.out …).
 * Returns a heap-allocated Problem, or NULL if no valid cases are found.
 * Caller must call ProblemFree() on the result. */
Problem* LoadProblem(const char* problemDir, int defaultTimeLimitMs);

#endif /* LOADER_H */
