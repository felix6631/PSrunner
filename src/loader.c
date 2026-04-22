#include "../header/loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Platform detection ─────────────────────────────────────────────────── */

#if defined(__APPLE__) || defined(__linux__)
#  define LOADER_POSIX
#  include <unistd.h>
#  include <limits.h>
#  include <dirent.h>
#  include <sys/wait.h>
#  ifdef __APPLE__
#    include <mach-o/dyld.h>
#  endif
#elif defined(_WIN32)
#  define LOADER_WINDOWS
#  include <windows.h>
#  define PATH_MAX  MAX_PATH
#  define popen    _popen
#  define pclose   _pclose
#endif

#ifndef PATH_MAX
#  define PATH_MAX 4096
#endif

#ifdef LOADER_WINDOWS
#  define SEP_CHAR '\\'
#else
#  define SEP_CHAR '/'
#endif


/* ═══════════════════════════════════════════════════════════════════════
 *  Internal helpers
 * ═══════════════════════════════════════════════════════════════════════ */

static char* _readFile(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    if (size < 0) { fclose(f); return NULL; }
    char* buf = malloc((size_t)size + 2);
    if (!buf) { fclose(f); return NULL; }
    size_t n = fread(buf, 1, (size_t)size, f);
    buf[n] = '\0';
    fclose(f);
    return buf;
}

/* Scan dir for files named N.in (all-digit N). Fill nums[] with up to
 * maxNums values. Returns count found, or -1 if the directory can't be opened. */
static int _collectNums(const char* dir, int* nums, int maxNums) {
    int count = 0;

#ifdef LOADER_POSIX
    DIR* d = opendir(dir);
    if (!d) return -1;

    struct dirent* ent;
    while ((ent = readdir(d)) != NULL && count < maxNums) {
        const char* name = ent->d_name;
        size_t len = strlen(name);
        if (len < 4 || strcmp(name + len - 3, ".in") != 0) continue;

        size_t prefixLen = len - 3;
        char prefix[256];
        if (prefixLen == 0 || prefixLen >= sizeof(prefix)) continue;
        memcpy(prefix, name, prefixLen);
        prefix[prefixLen] = '\0';

        int valid = 1;
        for (size_t i = 0; i < prefixLen; i++)
            if (prefix[i] < '0' || prefix[i] > '9') { valid = 0; break; }
        if (!valid) continue;

        nums[count++] = atoi(prefix);
    }
    closedir(d);

#elif defined(LOADER_WINDOWS)
    char pattern[MAX_PATH];
    snprintf(pattern, sizeof(pattern), "%s\\*.in", dir);

    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return -1;

    do {
        const char* name = fd.cFileName;
        size_t len = strlen(name);
        if (len < 4 || strcmp(name + len - 3, ".in") != 0) continue;

        size_t prefixLen = len - 3;
        char prefix[256];
        if (prefixLen == 0 || prefixLen >= sizeof(prefix)) continue;
        memcpy(prefix, name, prefixLen);
        prefix[prefixLen] = '\0';

        int valid = 1;
        for (size_t i = 0; i < prefixLen; i++)
            if (prefix[i] < '0' || prefix[i] > '9') { valid = 0; break; }
        if (!valid) continue;

        if (count < maxNums) nums[count++] = atoi(prefix);
    } while (FindNextFileA(h, &fd));
    FindClose(h);
#endif

    return count;
}


/* ═══════════════════════════════════════════════════════════════════════
 *  GetExeDir
 * ═══════════════════════════════════════════════════════════════════════ */

int GetExeDir(char* buf, size_t size) {
#ifdef LOADER_POSIX
    char tmp[PATH_MAX];

#  ifdef __linux__
    ssize_t n = readlink("/proc/self/exe", tmp, sizeof(tmp) - 1);
    if (n < 0) return -1;
    tmp[n] = '\0';

#  elif defined(__APPLE__)
    uint32_t sz = (uint32_t)sizeof(tmp);
    if (_NSGetExecutablePath(tmp, &sz) != 0) return -1;
    char resolved[PATH_MAX];
    if (!realpath(tmp, resolved)) return -1;
    strncpy(tmp, resolved, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

#  else
    (void)buf; (void)size;
    return -1;  /* unsupported POSIX platform */
#  endif

    char* slash = strrchr(tmp, '/');
    if (slash) *slash = '\0';
    strncpy(buf, tmp, size - 1);
    buf[size - 1] = '\0';
    return 0;

#elif defined(LOADER_WINDOWS)
    char tmp[MAX_PATH];
    if (GetModuleFileNameA(NULL, tmp, MAX_PATH) == 0) return -1;
    char* bs = strrchr(tmp, '\\');
    if (bs) *bs = '\0';
    strncpy(buf, tmp, size - 1);
    buf[size - 1] = '\0';
    return 0;

#else
    (void)buf; (void)size;
    return -1;
#endif
}


/* ═══════════════════════════════════════════════════════════════════════
 *  CompileSolution
 * ═══════════════════════════════════════════════════════════════════════ */

int CompileSolution(const char* srcPath, const char* outPath,
                    char* errMsg, size_t errMsgSize) {
    const char* ext = strrchr(srcPath, '.');
    const char* cc  = "gcc";
    if (ext && (strcmp(ext, ".cpp") == 0 || strcmp(ext, ".cxx") == 0 ||
                strcmp(ext, ".cc")  == 0))
        cc = "g++";

    /* redirect stderr into stdout so popen captures both */
    char cmd[4096];
    snprintf(cmd, sizeof(cmd),
             "%s \"%s\" -o \"%s\" -lm 2>&1", cc, srcPath, outPath);

    FILE* pipe = popen(cmd, "r");
    if (!pipe) {
        if (errMsg && errMsgSize)
            snprintf(errMsg, errMsgSize, "failed to invoke compiler '%s'", cc);
        return -1;
    }

    size_t len = 0;
    if (errMsg && errMsgSize > 1) {
        int c;
        while (len + 1 < errMsgSize && (c = fgetc(pipe)) != EOF)
            errMsg[len++] = (char)c;
        errMsg[len] = '\0';
    }

    int status = pclose(pipe);

#ifdef LOADER_POSIX
    return (WIFEXITED(status) && WEXITSTATUS(status) == 0) ? 0 : -1;
#else
    return (status == 0) ? 0 : -1;
#endif
}


/* ═══════════════════════════════════════════════════════════════════════
 *  LoadProblem
 * ═══════════════════════════════════════════════════════════════════════ */

Problem* LoadProblem(const char* problemDir, int defaultTimeLimitMs) {
    int nums[1024];
    int count = _collectNums(problemDir, nums, 1024);
    if (count <= 0) return NULL;

    /* sort numerically (insertion sort — count is at most 1024) */
    for (int i = 1; i < count; i++) {
        int key = nums[i], j = i - 1;
        while (j >= 0 && nums[j] > key) { nums[j + 1] = nums[j]; j--; }
        nums[j + 1] = key;
    }

    Problem* p = ProblemMake(problemDir);
    if (!p) return NULL;

    for (int i = 0; i < count; i++) {
        char inPath[PATH_MAX], outPath[PATH_MAX];
        snprintf(inPath,  sizeof(inPath),  "%s%c%d.in",  problemDir, SEP_CHAR, nums[i]);
        snprintf(outPath, sizeof(outPath), "%s%c%d.out", problemDir, SEP_CHAR, nums[i]);

        char* inData  = _readFile(inPath);
        char* outData = _readFile(outPath);

        if (!inData || !outData) {
            free(inData);
            free(outData);
            fprintf(stderr, "warning: skipping case %d (missing .in or .out)\n", nums[i]);
            continue;
        }

        TestCase* tc = TestCaseMake(inData, outData, defaultTimeLimitMs);
        free(inData);
        free(outData);

        if (!tc) { ProblemFree(p); return NULL; }
        if (ProblemAddCase(p, tc) != 0) { TestCaseFree(tc); ProblemFree(p); return NULL; }
    }

    if (p->caseCount == 0) { ProblemFree(p); return NULL; }
    return p;
}
