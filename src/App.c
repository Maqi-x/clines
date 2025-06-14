#include <App.h>
#include <Log.h>
#include <Utils.h>

#include <Config.h>
#include <Definitions.h>
#include <LineCounterList.h>
#include <StringList.h>

#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <regex.h>
#include <sys/stat.h>
#include <unistd.h>

CL_Error CL_Init(CLinesApp* self) {
    memset(self, 0, sizeof(CLinesApp));

    CFG_Error cerr = CFG_Init(&self->cfg);
    if (cerr != CFGE_Ok) return CLE_ConfigError;

    LCL_Error lcerr = LCL_InitReserved(&self->files, 128);
    if (lcerr != LCLE_Ok) return CLE_ListError;

    return CLE_Ok;
}

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

    self->linesCount = 0;
    self->fileCount = 0;
    self->dirCount = 0;

    self->errorDetails = NULL;

    return CLE_Ok;
}

CL_Error CL_SetErrorDetails(CLinesApp* self, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    free(self->errorDetails);

    int res = vasprintf(&self->errorDetails, fmt, args);
    if (res == -1) return CLE_AllocFailed;

    va_end(args);
    return CLE_Ok;
}

CL_Error CL_MapAndExceptCFG(CLinesApp* self, CFG_Error cerr) {
    if (cerr == CFGE_Ok) return CLE_Ok;

    switch (cerr) {
    case CFGE_UnexpectedArgument:
        MSG_ShowError("Unexpected argument: %s.", self->cfg.errorDetails);
        break;
    case CFGE_RedeclaredFlag:
        MSG_ShowError("Flag redeclared.");
        break;
    case CFGE_InvalidInputNumber:
        MSG_ShowError("Invalid input number.");
        break;
    case CFGE_InvalidSortMode:
        MSG_ShowError("Invalid sort mode: %s.", self->cfg.errorDetails);
        break;

    case CFGE_AllocFailed:
    case CFGE_ListError:
    case CFGE_Ok: // average -Wswitch experimence
        MSG_ShowError("Internal error.\n");
        return CLE_InternalError;
    }

    return CLE_ConfigError;
}

CL_Error CL_MapAndExceptLCL(CLinesApp* self, LCL_Error lcerr) {
    if (lcerr == LCLE_Ok) return CLE_Ok;

    switch (lcerr) {
    case LCLE_IndexOutOfRange:
        MSG_ShowError("Internal error.");
        MSG_ShowDebugLog("Index Out Of the Range! Check indices...");
        return CLE_InternalError;
    case LCLE_AllocFailed:
        MSG_ShowError("Internal error.");
        MSG_ShowDebugLog("Out of memory. (malloc failed)");
        return CLE_AllocFailed;
    case LCLE_InvalidArgument:
        MSG_ShowError("Internal error.");
        MSG_ShowDebugLog("LineCounterList: InvalidArgument");
        return CLE_InternalError;
    case LCLE_Ok:
        return CLE_Ok;
    }

    return CLE_InternalError;
}

CL_Error CL_MapAndExceptCL(CLinesApp* self, CL_Error err) {
    if (err == CLE_Ok) return CLE_Ok;

    switch (err) {
    case CLE_Ok:
        return CLE_Ok;
    case CLE_Todo:
        MSG_ShowDebugLog("TODO!");
        break;
    case CLE_ConfigError:
    case CLE_ListError:
        // assume CL_MapAndExceptLCL / CL_MapAndExceptCFG alredy called
        break;

    case CLE_ReadDirError:
    case CLE_CloseDirError:
    case CLE_FileOpenError:
        if (self->errorDetails)
            MSG_ShowError("Failed to open file. (%s)", self->errorDetails);
        else
            MSG_ShowError("Failed to open file.");
        MSG_ShowTip("Are you sure you have read permissions to the specified directories?");
        break;
    case CLE_AllocFailed:
        MSG_ShowError("Internal error.");
        MSG_ShowDebugLog("Out of memory. (malloc failed)");
        break;
    case CLE_InternalError:
        if (self->errorDetails)
            MSG_ShowError("Internal error. (%s)", self->errorDetails);
        else
            MSG_ShowError("Internal error.");
        MSG_ShowDebugLog("Yes, CLE_InternalError");
        break;

    case CLE_InvalidArgError:
        MSG_ShowError("Invalid Argument.");
        break;
    case CLE_NoSuchFileOrDir:
        MSG_ShowError("No such file or directory: %s", self->errorDetails);
        break;
    case CLE_RegexError:
        MSG_ShowError("Invalid Regex.");
        break;
    }

    return err;
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

bool CL_ShouldIncludePath(CLinesApp* self, const char* path) {
    if (CL_IsExcluded(self, path)) return false;
    if (HasExcludedExtension(self, path)) return false;
    if (MatchesExcludedRegex(self, path)) return false;

    if (self->cfg.includedExtensions.len > 0 && !HasIncludedExtension(self, path)) {
        return false;
    }
    if (self->includedRegexesCount > 0 && !MatchesIncludedRegex(self, path)) {
        return false;
    }

    return true;
}

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

CL_Error CL_HandleFile(CLinesApp* self, const char* path, const char* name, FileMeta* meta) {
    if (!CL_ShouldIncludePath(self, name)) return CLE_Ok;

    char* basePath = realpath(self->currentPath, NULL);
    char* realFullPath = realpath(path, NULL);

    const char* toPrint = NULL;
    if (strncmp(realFullPath, basePath, strlen(basePath)) == 0) {
        const char* relative = realFullPath + strlen(basePath);
        if (*relative == '/') relative++; // skip leading slash
        toPrint = relative;
    } else {
        toPrint = realFullPath;
    }

    if (CL_IsExcluded(self, realFullPath)) return CLE_Ok;

    self->fileCount++;
    usize count = 0;

    FILE* file = fopen(path, "r");
    if (file == NULL) {
        return CLE_FileOpenError;
    }

    int ch;
    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\n') count++;
    }
    fclose(file);

    LCL_Error lcerr = LCL_Append(&self->files, toPrint, count, meta);
    if (lcerr != LCLE_Ok) return CL_MapAndExceptLCL(self, lcerr);
    self->linesCount += count;

    free(basePath);
    free(realFullPath);
    return CLE_Ok;
}

CL_Error CL_CountRecursive(CLinesApp* self, const char* path, usize depth) {
    if (depth > self->cfg.maxDepth) return CLE_Ok;

    self->dirCount++;
    DIR* dir = opendir(path);
    if (!dir) {
        return CLE_ReadDirError;
    }

    CL_Error err = CLE_Ok;
    struct dirent* entry = NULL;
    while ((entry = readdir(dir)) != NULL) {
        // skip "." and ".."
        if (strcmp(entry->d_name, ".") == 0) continue;
        if (strcmp(entry->d_name, "..") == 0) continue;

        usize size = strlen(path) + strlen(entry->d_name) + 2; // + '\0' + '/'
        char* fullPath = malloc(size);
        snprintf(fullPath, size, "%s/%s", path, entry->d_name);

        struct stat st;
        if (stat(fullPath, &st) == -1) {
            continue;
        }

        if (S_ISDIR(st.st_mode) && self->cfg.recursive.val) {
            err = CL_CountRecursive(self, fullPath, depth + 1);
            if (err != CLE_Ok) {
                free(fullPath);
                goto cleanup;
            }
        } else if (S_ISREG(st.st_mode)) {
            FileMeta meta = {
                .path = fullPath,
                .size = st.st_size,
                .mtime = st.st_mtime,
            };
            err = CL_HandleFile(self, fullPath, entry->d_name, &meta);
            if (err != CLE_Ok) {
                free(fullPath);
                goto cleanup;
            }
        }

        free(fullPath);
        continue;
    }

cleanup:
    if (closedir(dir) == -1) {
        return CLE_CloseDirError;
    }
    return err;
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

    for (usize i = 0; i < self->cfg.excludedPaths.len; ++i) {
        char* excluded;
        SL_Get(&self->cfg.excludedPaths, i, &excluded);

        self->excludedPaths[i] = realpath(excluded, NULL);
        if (self->excludedPaths[i] == NULL)
            return CLE_AllocFailed; // CL_Destroy should destroy the paths created so far

        if (access(self->excludedPaths[i], F_OK) == -1) {
            CL_SetErrorDetails(self, excluded);
            return CLE_NoSuchFileOrDir;
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
    for (usize i = 0; i < self->files.len; ++i) {
        LineCounter* f;
        LCL_Get(&self->files, i, &f);

        printf("[+] %s - %zu lines\n", f->toPrint, f->lines);
    }
    return CLE_Ok;
}

CL_Error CL_ApplySort(CLinesApp* self) {
    return CLE_Todo;
}

int CL_Run(CLinesApp* self, int argc, char** argv) {
    CL_Error err;

    err = CL_LoadConfig(self, argc, argv);
    if (err != CLE_Ok) {
        return (int)CL_MapAndExceptCL(self, err);
    }
    // CFG_DebugBump(&self->cfg, stdout, NULL);

    err = CL_LoadExcludedRegexes(self);
    if (err != CLE_Ok) return (int)CL_MapAndExceptCL(self, err);

    err = CL_LoadIncludedRegexes(self);
    if (err != CLE_Ok) return (int)CL_MapAndExceptCL(self, err);

    err = CL_LoadExcludedPaths(self);
    if (err != CLE_Ok) {
        return (int)CL_MapAndExceptCL(self, err);
    }

    if (self->cfg.includedPaths.len > 1) {
        usize totalLinesCount = 0;
        usize totalFileCount = 0;
        usize totalDirCount = 0;
        for (usize i = 0; i < self->cfg.includedPaths.len; ++i) {
            SL_Get(&self->cfg.includedPaths, i, &self->currentPath);

            printf("\033[1m-------- %s/ --------\033[0m\n", self->currentPath);
            err = CL_CountRecursive(self, self->currentPath, 0);
            if (err != CLE_Ok) {
                return (int)CL_MapAndExceptCL(self, err);
            }

            LCL_Error lcerr = LCL_SortBy(&self->files, self->cfg.sortMode);
            if (lcerr != LCLE_Ok) {
                return (int)CL_MapAndExceptLCL(self, lcerr);
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

        LCL_Error lcerr = LCL_SortBy(&self->files, self->cfg.sortMode);
        if (lcerr != LCLE_Ok) {
            return (int)CL_MapAndExceptLCL(self, lcerr);
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
