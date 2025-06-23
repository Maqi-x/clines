#include <CLines/App.h>
#include <Log.h>
#include <Utils.h>

#include <Definitions.h>
#include <LocParser.h>
#include <LocSettings.h>
#include <LocUtils.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef READ_BUF_SIZE
#    define READ_BUF_SIZE (64 * 1024)
#endif

#ifndef TMP_PATH_BUF_CAP
#    ifdef PATH_MAX
#        define TMP_PATH_BUF_CAP (PATH_MAX + 256)
#    else
#        define TMP_PATH_BUF_CAP 32768
#    endif
#endif

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
bool CL_IsExcluded(CLinesApp* self, const char* resolvedPath) {
    for (usize i = 0; i < self->excludedPathsCount; ++i) {
        if (HasPrefix(resolvedPath, self->excludedPaths[i])) {
            return true;
        }
    }
    return false;
}

static inline CL_Error CountLines(const char* path, usize* out) {
    usize count = 0;

    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        return CLE_FileOpenError;
    }

    char buf[READ_BUF_SIZE];
    isize bytesRead;
    while ((bytesRead = read(fd, buf, READ_BUF_SIZE)) > 0) {
        for (isize i = 0; i < bytesRead; ++i) {
            if (buf[i] == '\n') count++;
        }
    }

    close(fd);

    if (bytesRead < 0) {
        return CLE_FileReadError;
    }

    *out = count;
    return CLE_Ok;
}

CL_Error CL_HandleFileWithLoc(CLinesApp* self, const char* formattedPath, const char* resolvedPath, const char* name, FileMeta* meta) {
    bool hasLocStat = false;
    LocStat stat = {0};
    const LocEntry* lang = NULL;

    bool res = GetLocLangFor(name, &lang);
    if (!res) {
        // no loc lang associated with this file
        CL_Error err = CountLines(formattedPath, &stat.totalLines);
        if (err != CLE_Ok) return err;

        hasLocStat = false;
        goto add;
    }

    LocParser parser;
    LP_Init(&parser);

    LP_Error lperr = LP_ParseFile(&parser, lang, formattedPath, &stat);
    if (lperr != LPE_Ok) {
        CL_SetErrorDetails(self, name);
        return MapAndExceptLP(self, lperr);
    }

    LP_Destroy(&parser);

    hasLocStat = true;
    goto add;

add:
    {
        LCL_Error lcerr = LCL_Append(&self->files, formattedPath, stat.totalLines, meta, stat, hasLocStat);
        if (lcerr != LCLE_Ok) return CL_MapAndExceptLCL(self, lcerr);
        self->linesCount += stat.totalLines;

        return CLE_Ok;
    }
}

CL_Error CL_HandleFile(CLinesApp* self, const char* formattedPath, const char* resolvedPath, const char* name, FileMeta* meta) {
    if (!CL_ShouldIncludePath(self, resolvedPath, name, false)) return CLE_Ok;
    if (CL_IsExcluded(self, resolvedPath)) return CLE_Ok;

    self->fileCount++;

    if (self->cfg.locEnabled.val) {
        return CL_HandleFileWithLoc(self, formattedPath, resolvedPath, name, meta);
    }

    usize count = 0;
    CL_Error err = CountLines(formattedPath, &count);
    if (err != CLE_Ok) return err;

    LCL_Error lcerr = LCL_Append(&self->files, formattedPath, count, meta, (LocStat) {0}, false);
    if (lcerr != LCLE_Ok) return CL_MapAndExceptLCL(self, lcerr);
    self->linesCount += count;

    return CLE_Ok;
}

static char* BuildFullPath(const char* path, const char* name, char* tmpBuf, char** allocatedPath) {
    usize pathLen = strlen(path);
    usize nameLen = strlen(name);

    bool hasTrailingSlash = pathLen > 0 && path[pathLen - 1] == '/';
    usize totalLen = pathLen + (hasTrailingSlash ? 0 : 1) + nameLen;

    char* fullPath = NULL;

    if (totalLen + 1 < TMP_PATH_BUF_CAP) {
        memcpy(tmpBuf, path, pathLen);
        usize offset = pathLen;
        if (!hasTrailingSlash) {
            tmpBuf[offset++] = '/';
        }
        memcpy(tmpBuf + offset, name, nameLen);
        tmpBuf[totalLen] = '\0';
        fullPath = tmpBuf;
    } else {
        *allocatedPath = malloc(totalLen + 1);
        if (*allocatedPath == NULL) return NULL;

        memcpy(*allocatedPath, path, pathLen);
        usize offset = pathLen;
        if (!hasTrailingSlash) {
            (*allocatedPath)[offset++] = '/';
        }
        memcpy(*allocatedPath + offset, name, nameLen);
        (*allocatedPath)[totalLen] = '\0';

        fullPath = *allocatedPath;
    }

    return fullPath;
}

// Returns NULL on failure. If outAllocated is non-NULL, the result must be freed.
static char* GetResolvedPath(const char* path, char* tmpBuf, char** outAllocated) {
    char* resolved = realpath(path, tmpBuf);
    if (!resolved) return NULL;

    if (resolved == tmpBuf) {
        *outAllocated = NULL;
        return tmpBuf;
    }

    *outAllocated = resolved;
    return resolved;
}

CL_Error CL_CountRecursive(CLinesApp* self, const char* path, usize depth) {
    if (depth > self->cfg.maxDepth) return CLE_Ok;

    struct stat pathStat;
    if (stat(path, &pathStat) == -1) {
        CL_SetErrorDetails(self, path);
        return CLE_NoSuchFileOrDir;
    }

    FileMeta pathMeta = {
        .path = (char*)path,
        .size = pathStat.st_size,
        .mtime = pathStat.st_mtime,
    };

    if (S_ISREG(pathStat.st_mode)) {
        return CL_HandleFile(self, path, path, GetBaseName(path), &pathMeta);
    }

    DIR* dir = opendir(path);
    if (!dir) {
        return CLE_ReadDirError;
    }

    CL_Error err = CLE_Ok;
    struct dirent* entry = NULL;

    char tmpFormmattedBuf[TMP_PATH_BUF_CAP];
    char tmpResolvedBuf[TMP_PATH_BUF_CAP];
    while ((entry = readdir(dir)) != NULL) {
        // skip "." and ".."
        if (strcmp(entry->d_name, ".") == 0) continue;
        if (strcmp(entry->d_name, "..") == 0) continue;

        char* allocatedFormattedPath = NULL;
        char* formattedPath = BuildFullPath(path, entry->d_name, tmpFormmattedBuf, &allocatedFormattedPath);

        if (formattedPath == NULL) {
            return CLE_AllocFailed;
        }

        char* allocatedResolved = NULL;

        char* resolvedPath = GetResolvedPath(formattedPath, tmpResolvedBuf, &allocatedResolved);
        if (resolvedPath == NULL) {
            free(allocatedFormattedPath);
            continue;
        }

        struct stat st;
        if (stat(formattedPath, &st) == -1) {
            continue;
        }

        if (S_ISDIR(st.st_mode) && self->cfg.recursive.val) {
            INode inode = { .dev = st.st_dev, .ino = st.st_ino };
            if (!CL_ShouldIncludePath(self, resolvedPath, entry->d_name, true)) {
                free(allocatedFormattedPath);
                continue;
            }

            if (INS_Contains(&self->seen, inode)) {
                free(allocatedFormattedPath);
                free(allocatedResolved);
                continue;
            }

            INS_Insert(&self->seen, inode);
            self->dirCount++;

            err = CL_CountRecursive(self, formattedPath, depth + 1);
            if (err != CLE_Ok) {
                free(allocatedFormattedPath);
                free(allocatedResolved);
                goto cleanup;
            }
        } else if (S_ISREG(st.st_mode)) {
            if (st.st_size == 0) continue;

            FileMeta meta = {
                .path = formattedPath,
                .size = st.st_size,
                .mtime = st.st_mtime,
            };

            err = CL_HandleFile(self, formattedPath, resolvedPath, entry->d_name, &meta);
            if (err != CLE_Ok) {
                free(allocatedFormattedPath);
                free(allocatedResolved);
                goto cleanup;
            }
        }

        free(allocatedFormattedPath);
        free(allocatedResolved);
        continue;
    }

cleanup:
    if (closedir(dir) == -1) {
        return CLE_CloseDirError;
    }
    return err;
}
