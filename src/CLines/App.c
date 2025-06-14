#include <CLines/App.h>

#include <Log.h>
#include <Utils.h>

#include <Config.h>
#include <Definitions.h>
#include <INodeSet.h>
#include <LineCounterList.h>
#include <StringList.h>

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>
#include <regex.h>
#include <sys/stat.h>
#include <unistd.h>

/**
 * Initializes CLinesApp (should be called before CL_Run).
 */
CL_Error CL_Init(CLinesApp* self) {
    memset(self, 0, sizeof(CLinesApp));

    CFG_Error cerr = CFG_Init(&self->cfg);
    if (cerr != CFGE_Ok) return CL_MapAndExceptCFG(self, cerr);

    LCL_Error lcerr = LCL_InitReserved(&self->files, 128);
    if (lcerr != LCLE_Ok) return CL_MapAndExceptLCL(self, lcerr);

    INS_Error inerr = INS_DefaultInit(&self->seen);
    if (inerr != INSE_Ok) return CL_MapAndExceptINS(self, inerr);

    return CLE_Ok;
}

/**
 * Destroys CLinesApp and frees up memory.
 */
CL_Error CL_Destroy(CLinesApp* self) {
    for (usize i = 0; i < self->includedRegexesCount; ++i) {
        regfree(&self->includedRegexes[i]);
    }
    free(self->includedRegexes);
    self->includedRegexes = NULL;
    self->includedRegexesCount = 0;

    for (usize i = 0; i < self->excludedRegexesCount; ++i) {
        regfree(&self->excludedRegexes[i]);
    }
    free(self->excludedRegexes);
    self->excludedRegexes = NULL;
    self->excludedRegexesCount = 0;

    for (usize i = 0; i < self->excludedPathsCount; ++i) {
        free(self->excludedPaths[i]);
    }

    free(self->excludedPaths);
    self->excludedPaths = NULL;
    self->excludedPathsCount = 0;

    CFG_Error cerr = CFG_Destroy(&self->cfg);
    if (cerr != CFGE_Ok) return CLE_ConfigError;

    LCL_Error lcerr = LCL_Destroy(&self->files);
    if (lcerr != LCLE_Ok) return CLE_ListError;

    INS_Error inerr = INS_Destroy(&self->seen);
    if (inerr != INSE_Ok) return CLE_SetError;

    self->linesCount = 0;
    self->fileCount = 0;
    self->dirCount = 0;

    self->errorDetails = NULL;

    return CLE_Ok;
}

static bool HasExcludedExtension(CLinesApp* self, const char* path) {
    const char* ext = GetExtension(path);
    if (!ext) return false;

    for (usize i = 0; i < self->cfg.excludedExtensions.len; ++i) {
        char* excludedExt;
        SL_Get(&self->cfg.excludedExtensions, i, &excludedExt);
        if (StrEql(ext, excludedExt)) return true;
    }

    return false;
}

static bool MatchesExcludedRegex(CLinesApp* self, const char* path) {
    for (usize i = 0; i < self->excludedRegexesCount; ++i) {
        int res = regexec(&self->excludedRegexes[i], path, 0, NULL, 0);
        if (res == 0) return true;
    }

    return false;
}

static bool HasIncludedExtension(CLinesApp* self, const char* path) {
    const char* ext = GetExtension(path);
    if (!ext) return false;

    for (usize i = 0; i < self->cfg.includedExtensions.len; ++i) {
        char* includedExt;
        SL_Get(&self->cfg.includedExtensions, i, &includedExt);
        if (StrEql(ext, includedExt)) return true;
    }

    return false;
}

static bool MatchesIncludedRegex(CLinesApp* self, const char* path) {
    for (usize i = 0; i < self->includedRegexesCount; ++i) {
        int res = regexec(&self->includedRegexes[i], path, 0, NULL, 0);
        if (res == 0) return true;
    }

    return false;
}

/**
 * Checks if file/directory should be included
 */
bool CL_ShouldIncludePath(CLinesApp* self, const char* path, const char* name, bool isDir) {
    if (CL_IsExcluded(self, path)) return false;
    if (!isDir) {
        if (HasExcludedExtension(self, name)) return false;
    }
    if (MatchesExcludedRegex(self, path)) return false;

    if (!isDir) {
        if (self->cfg.includedExtensions.len > 0 && !HasIncludedExtension(self, name)) {
            return false;
        }
    }
    if (self->includedRegexesCount > 0 && !MatchesIncludedRegex(self, name)) {
        return false;
    }

    return true;
}

/**
 * Checks if the file is excluded
 */
bool CL_IsExcluded(CLinesApp* self, const char* fullPath) {
    for (usize i = 0; i < self->excludedPathsCount; ++i) {
        if (HasPrefix(fullPath, self->excludedPaths[i])) {
            return true;
        }
    }
    return false;
}

CL_Error CL_ResetCounter(CLinesApp* self) {
    self->linesCount = 0;
    self->fileCount = 0;
    self->dirCount = 0;

    return CLE_Ok;
}

static CL_Error CL_CompileRegexList(StringList* src, regex_t** outList, usize* outCount) {
    *outCount = 0;
    *outList = malloc(src->len * sizeof(regex_t));
    if (!*outList) return CLE_AllocFailed;

    for (usize i = 0; i < src->len; ++i) {
        char* regStr;
        SL_Get(src, i, &regStr);

        int res = regcomp(&(*outList)[i], regStr, REG_ICASE);
        if (res != 0) return CLE_RegexError;

        (*outCount)++;
    }

    return CLE_Ok;
}

CL_Error CL_LoadIncludedRegexes(CLinesApp* self) {
    return CL_CompileRegexList(&self->cfg.includedRegexes, &self->includedRegexes, &self->includedRegexesCount);
}

CL_Error CL_LoadExcludedRegexes(CLinesApp* self) {
    return CL_CompileRegexList(&self->cfg.excludedRegexes, &self->excludedRegexes, &self->excludedRegexesCount);
}

CL_Error CL_LoadExcludedPaths(CLinesApp* self) {
    self->excludedPathsCount = 0;
    self->excludedPaths = malloc(self->cfg.excludedPaths.len * sizeof(char*));
    if (!self->excludedPaths) return CLE_AllocFailed;

    for (usize i = 0; i < self->cfg.excludedPaths.len; ++i) {
        char* excluded;
        SL_Get(&self->cfg.excludedPaths, i, &excluded);

        errno = 0;
        self->excludedPaths[i] = realpath(excluded, NULL);
        if (self->excludedPaths[i] == NULL) {
            if (errno == ENOENT || errno == ENOTDIR) {
                CL_SetErrorDetails(self, excluded);
                return CLE_NoSuchFileOrDir;
            } else if (errno == ENOMEM) {
                return CLE_AllocFailed;
            } else {
                // unexpected error
                CL_SetErrorDetails(self, excluded);
                return CLE_InternalError;
            }
        }

        self->excludedPathsCount++;
    }

    return CLE_Ok;
}

CL_Error CL_LoadConfig(CLinesApp* self, int argc, char** argv) {
    CFG_Error cerr = CFG_Parse(&self->cfg, argc, argv);
    return CL_MapAndExceptCFG(self, cerr);
}

CL_Error CL_PrintFiles(CLinesApp* self) {
    if (!self->cfg.verbose.val) return CLE_Ok;

    for (usize i = 0; i < self->files.len; ++i) {
        LineCounter* f;
        LCL_Get(&self->files, i, &f);

        printf("[+] %s - %zu lines\n", f->toPrint, f->lines);
    }
    return CLE_Ok;
}

CL_Error CL_ApplySort(CLinesApp* self) {
    LCL_Error lcerr = LCL_SortBy(&self->files, self->cfg.sortMode, self->cfg.reverse.val);
    return (int)CL_MapAndExceptLCL(self, lcerr);
}

int CL_Run(CLinesApp* self, int argc, char** argv) {
    CL_Error err;

    err = CL_LoadConfig(self, argc, argv);
    if (err != CLE_Ok) {
        return (int)CL_MapAndExceptCL(self, err);
    }
    debug = self->cfg.debugMode.val;

    if (debug) CFG_DebugBump(&self->cfg, stdout, NULL);

    err = CL_LoadExcludedRegexes(self);
    if (err != CLE_Ok) {
        return (int)CL_MapAndExceptCL(self, err);
    }

    err = CL_LoadIncludedRegexes(self);
    if (err != CLE_Ok) {
        return (int)CL_MapAndExceptCL(self, err);
    }

    err = CL_LoadExcludedPaths(self);
    if (err != CLE_Ok) {
        return (int)CL_MapAndExceptCL(self, err);
    }

    if (self->cfg.includedPaths.len > 1) {
        usize totalLinesCount = 0;
        usize totalFileCount = 0;
        usize totalDirCount = 0;
        for (usize i = 0; i < self->cfg.includedPaths.len; ++i) {
            INS_Clear(&self->seen); // seen is invidual for all paths
            SL_Get(&self->cfg.includedPaths, i, &self->currentPath);

            printf("\033[1m-------- %s/ --------\033[0m\n", self->currentPath);
            err = CL_CountRecursive(self, self->currentPath, 0);
            if (err != CLE_Ok) {
                return (int)CL_MapAndExceptCL(self, err);
            }

            err = CL_ApplySort(self);
            if (err != CLE_Ok) {
                return (int)CL_MapAndExceptCL(self, err);
            }

            err = CL_PrintFiles(self);
            if (err != CLE_Ok) {
                return (int)CL_MapAndExceptCL(self, err);
            }

            printf(BOLD "Lines Count:" RESET " %zu\n", self->linesCount);
            printf(BOLD "Files Count:" RESET " %zu\n", self->fileCount);
            if (self->cfg.recursive.val) {
                printf(BOLD "Directories Count:" RESET " %zu\n", self->dirCount);
            }
            putchar('\n');

            totalLinesCount += self->linesCount;
            totalFileCount += self->fileCount;
            totalDirCount += self->dirCount;

            CL_ResetCounter(self);
        }

        printf(BOLD "-------- TOTAL --------\n" RESET);
        printf(BOLD "Total LINES:" RESET " %zu\n", totalLinesCount);
        printf(BOLD "Total files:" RESET " %zu\n", totalFileCount);
        if (self->cfg.recursive.val) {
            printf(BOLD "Total directories:" RESET " %zu\n", totalDirCount);
        }
    } else {
        SL_Get(&self->cfg.includedPaths, 0, &self->currentPath);
        err = CL_CountRecursive(self, self->currentPath, 0);
        if (err != CLE_Ok) {
            return (int)CL_MapAndExceptCL(self, err);
        }

        err = CL_ApplySort(self);
        if (err != CLE_Ok) {
            return (int)CL_MapAndExceptCL(self, err);
        }

        err = CL_PrintFiles(self);
        if (err != CLE_Ok) {
            return (int)err;
        }

        printf(BOLD "Total Lines:" RESET " %zu\n", self->linesCount);
        printf(BOLD "Total Files:" RESET " %zu\n", self->fileCount);
        if (self->cfg.recursive.val) {
            printf(BOLD "Total Directories:" RESET " %zu\n", self->dirCount);
        }
    }

    return 0;
}
