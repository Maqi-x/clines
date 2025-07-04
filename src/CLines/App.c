#include <CLines/App.h>

#include <Log.h>
#include <Utils.h>

#include <Config.h>
#include <Definitions.h>
#include <INodeSet.h>
#include <LineCounterList.h>
#include <StringList.h>

#include <HelpPrinter.h>
#include <HelpSettings.h>

#include <LocParser.h>
#include <LocSettings.h>
#include <LocUtils.h>

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

/// Initializes CLinesApp (should be called before CL_Run).
CL_Error CL_Init(CLinesApp* self) {
    memset(self, 0, sizeof(CLinesApp));

    CFG_Error cerr = CFG_Init(&self->cfg);
    if (cerr != CFGE_Ok) return CL_MapAndExceptCFG(self, cerr);

    LCL_Error lcerr = LCL_InitReserved(&self->files, 128);
    if (lcerr != LCLE_Ok) return CL_MapAndExceptLCL(self, lcerr);

    INS_Error inerr = INS_DefaultInit(&self->seen);
    if (inerr != INSE_Ok) return CL_MapAndExceptINS(self, inerr);

    HP_Init(
        &self->helpPrinter,
        "CLines Help",
        "A simple command line utility for counting lines in files",
        GetHelp(), GetHelpCategoryCount(),
        "For bug reports and more information visit: https://github.com/maqi-x/clines.\n"
        " Licensed under the GNU General Public License v3."
    );

    return CLE_Ok;
}

static void FreeRegexArray(regex_t** arr, usize* count) {
    for (usize i = 0; i < *count; ++i) regfree(&(*arr)[i]);
    free(*arr);
    *arr = NULL;
    *count = 0;
}

static void FreeStringArray(char*** arr, usize* count) {
    for (usize i = 0; i < *count; ++i) free((*arr)[i]);
    free(*arr);
    *arr = NULL;
    *count = 0;
}

/// Destroys CLinesApp and frees up memory.
CL_Error CL_Destroy(CLinesApp* self) {
    FreeRegexArray(&self->includedRegexes, &self->includedRegexesCount);
    FreeRegexArray(&self->excludedRegexes, &self->excludedRegexesCount);
    FreeStringArray(&self->excludedPaths, &self->excludedPathsCount);

    CFG_Error cerr = CFG_Destroy(&self->cfg);
    if (cerr != CFGE_Ok) return CLE_ConfigError;

    LCL_Error lcerr = LCL_Destroy(&self->files);
    if (lcerr != LCLE_Ok) return CLE_ListError;

    INS_Error inerr = INS_Destroy(&self->seen);
    if (inerr != INSE_Ok) return CLE_SetError;

    HP_Destroy(&self->helpPrinter);

    self->linesCount = 0;
    self->fileCount = 0;
    self->dirCount = 0;

    self->errorDetails = NULL;

    return CLE_Ok;
}

/// Prints the location statistics of a given file.
CL_Error CL_PrintLocStat(CLinesApp* self, LocStat* stat, usize indentLevel) {
    printf("%*s" BOLD "Code Lines: %zu\n" RESET,    (int)indentLevel * 2, "", stat->codeLines);
    printf("%*s" BOLD "Comment Lines: %zu\n" RESET, (int)indentLevel * 2, "", stat->commentLines);
    printf("%*s" BOLD "Blank Lines: %zu\n" RESET,   (int)indentLevel * 2, "", stat->emptyLines);
    printf("%*s" BOLD "Preprocessor Directive Lines: %zu\n" RESET, (int)indentLevel * 2, "", stat->preprocessorLines);
    return CLE_Ok;
}

/// Resets the counters of the application.
CL_Error CL_ResetCounter(CLinesApp* self) {
    self->linesCount = 0;
    self->fileCount = 0;
    self->dirCount = 0;

    return CLE_Ok;
}

/// Compiles a list of regular expressions from a string list.
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

/// Loads the included regular expressions from the configuration.
CL_Error CL_LoadIncludedRegexes(CLinesApp* self) {
    return CL_CompileRegexList(&self->cfg.includedRegexes, &self->includedRegexes, &self->includedRegexesCount);
}

/// Loads the excluded regular expressions from the configuration.
CL_Error CL_LoadExcludedRegexes(CLinesApp* self) {
    return CL_CompileRegexList(&self->cfg.excludedRegexes, &self->excludedRegexes, &self->excludedRegexesCount);
}

/// Loads the excluded paths from the configuration.
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

/// Loads the configuration from the command line arguments.
CL_Error CL_LoadConfig(CLinesApp* self, int argc, char** argv) {
    CFG_Error cerr = CFG_Parse(&self->cfg, argc, argv);
    return CL_MapAndExceptCFG(self, cerr);
}

CL_Error CL_PrintFiles(CLinesApp* self) {
    if (self->cfg.printMode.val) {
        for (usize i = 0; i < self->files.len; ++i) {
            LineCounter* f;
            LCL_Error lcerr = LCL_Get(&self->files, i, &f);
            if (lcerr != LCLE_Ok) return CL_MapAndExceptLCL(self, lcerr);

            printf("[+] %s - %zu lines\n", f->toPrint, f->lines);
            if (self->cfg.locEnabled.val && f->hasLocStat) {
                CL_PrintLocStat(self, &f->locStat, 1);
            }
        }
    } else if (self->cfg.locEnabled.val) {
        for (usize i = 0; i < self->files.len; ++i) {
            LineCounter* f;
            LCL_Error lcerr = LCL_Get(&self->files, i, &f);
            if (lcerr != LCLE_Ok) return CL_MapAndExceptLCL(self, lcerr);

            if (self->cfg.locEnabled.val && f->hasLocStat) {
                printf("[+] %s - %zu lines\n", f->toPrint, f->lines);
                CL_PrintLocStat(self, &f->locStat, 1);
            }
        }
    }
    return CLE_Ok;
}

CL_Error CL_ApplySort(CLinesApp* self) {
    LCL_Error lcerr = LCL_SortBy(&self->files, self->cfg.sortMode, self->cfg.reverse.val);
    return (int)CL_MapAndExceptLCL(self, lcerr);
}

static int HandleBasicFlags(CLinesApp* self) {
    if (self->cfg.showHelp.val) {
        HP_Print(&self->helpPrinter, stdout);
        return (int)CLE_Ok;
    }

    if (self->cfg.showVersion.val) {
        puts(BOLD "Version: " RESET "clines " CLINES_VERSION);
        return (int)CLE_Ok;
    }

    if (self->cfg.showRepo.val) {
        puts(BOLD "Repository: " RESET "https://github.com/maqi-x/clines");
        return (int)CLE_Ok;
    }

    return 0;
}

static int ProcessSinglePath(CLinesApp* self, const char* path) {
    CL_Error err;

    err = CL_CountRecursive(self, path, 0);
    if (err != CLE_Ok) return (int)CL_MapAndExceptCL(self, err);

    err = CL_ApplySort(self);
    if (err != CLE_Ok) return (int)CL_MapAndExceptCL(self, err);

    err = CL_PrintFiles(self);
    if (err != CLE_Ok) return (int)CL_MapAndExceptCL(self, err);

    printf(BOLD "Total Lines:" RESET " %zu\n", self->linesCount);
    printf(BOLD "Total Files:" RESET " %zu\n", self->fileCount);
    if (self->cfg.recursive.val) {
        printf(BOLD "Total Directories:" RESET " %zu\n", self->dirCount);
    }

    return 0;
}

static int ProcessMultiplePaths(CLinesApp* self) {
    CL_Error err;
    usize totalLinesCount = 0;
    usize totalFileCount = 0;
    usize totalDirCount = 0;

    for (usize i = 0; i < self->cfg.includedPaths.len; ++i) {
        INS_Clear(&self->seen);
        SL_Get(&self->cfg.includedPaths, i, &self->currentPath);

        printf("\033[1m-------- %s/ --------\033[0m\n", self->currentPath);
        err = CL_CountRecursive(self, self->currentPath, 0);
        if (err != CLE_Ok) return (int)CL_MapAndExceptCL(self, err);

        err = CL_ApplySort(self);
        if (err != CLE_Ok) return (int)CL_MapAndExceptCL(self, err);

        err = CL_PrintFiles(self);
        if (err != CLE_Ok) return (int)CL_MapAndExceptCL(self, err);

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

    return 0;
}

int CL_Run(CLinesApp* self, int argc, char** argv) {
    CL_Error err;

    err = CL_LoadConfig(self, argc, argv);
    if (err != CLE_Ok) return (int)CL_MapAndExceptCL(self, err);

    debug = self->cfg.debugMode.val;
    if (debug) {
        CFG_DebugBump(&self->cfg, stdout, NULL);
    }

    int flagHandled = HandleBasicFlags(self);
    if (flagHandled != 0) return flagHandled;

    err = CL_LoadExcludedRegexes(self);
    if (err != CLE_Ok) return (int)CL_MapAndExceptCL(self, err);

    err = CL_LoadIncludedRegexes(self);
    if (err != CLE_Ok) return (int)CL_MapAndExceptCL(self, err);

    err = CL_LoadExcludedPaths(self);
    if (err != CLE_Ok) return (int)CL_MapAndExceptCL(self, err);

    if (self->cfg.includedPaths.len > 1) {
        return ProcessMultiplePaths(self);
    } else {
        SL_Get(&self->cfg.includedPaths, 0, &self->currentPath);
        return ProcessSinglePath(self, self->currentPath);
    }
}
