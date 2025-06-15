#include <CLines/App.h>
#include <Log.h>

#include <Definitions.h>

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

CL_Error CL_HandleFile(CLinesApp* self, const char* path, const char* name, FileMeta* meta) {
    if (!CL_ShouldIncludePath(self, path, name, false)) return CLE_Ok;
    if (CL_IsExcluded(self, path)) return CLE_Ok;

    self->fileCount++;
    usize count = 0;

    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        return CLE_FileOpenError;
    }

    char buf[READ_BUF_SIZE];
    ssize_t bytesRead;
    while ((bytesRead = read(fd, buf, READ_BUF_SIZE)) > 0) {
        for (ssize_t i = 0; i < bytesRead; ++i) {
            if (buf[i] == '\n') count++;
        }
    }

    close(fd);

    if (bytesRead < 0) {
        return CLE_FileReadError; // define if needed
    }

    LCL_Error lcerr = LCL_Append(&self->files, path, count, meta);
    if (lcerr != LCLE_Ok) return CL_MapAndExceptLCL(self, lcerr);
    self->linesCount += count;

    return CLE_Ok;
}

static char* BuildFullPath(const char* path, const char* name, char* tmpBuf, char** allocatedPath) {
    usize pathLen = strlen(path);
    usize nameLen = strlen(name);

    bool hasTrailingSlash = pathLen > 0 && path[pathLen - 1] == '/';
    size_t totalLen = pathLen + (hasTrailingSlash ? 0 : 1) + nameLen;

    char* fullPath = NULL;

    if (totalLen + 1 < TMP_PATH_BUF_CAP) {
        memcpy(tmpBuf, path, pathLen);
        size_t offset = pathLen;
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
        size_t offset = pathLen;
        if (!hasTrailingSlash) {
            (*allocatedPath)[offset++] = '/';
        }
        memcpy(*allocatedPath + offset, name, nameLen);
        (*allocatedPath)[totalLen] = '\0';

        fullPath = *allocatedPath;
    }

    return fullPath;
}

CL_Error CL_CountRecursive(CLinesApp* self, const char* path, usize depth) {
    if (depth > self->cfg.maxDepth) return CLE_Ok;

    struct stat pathStat;
    if (stat(path, &pathStat) == -1) {
        return CLE_NoSuchFileOrDir;
    }

    FileMeta pathMeta = {
        .path = (char*)path,
        .size = pathStat.st_size,
        .mtime = pathStat.st_mtime,
    };

    if (S_ISREG(pathStat.st_mode)) {
        return CL_HandleFile(self, path, GetBaseName(path), &pathMeta);
    }

    DIR* dir = opendir(path);
    if (!dir) {
        return CLE_ReadDirError;
    }

    CL_Error err = CLE_Ok;
    struct dirent* entry = NULL;

    char tmpBuf[TMP_PATH_BUF_CAP];
    while ((entry = readdir(dir)) != NULL) {
        // skip "." and ".."
        if (strcmp(entry->d_name, ".") == 0) continue;
        if (strcmp(entry->d_name, "..") == 0) continue;

        char* allocatedFullPath = NULL;
        char* fullPath = BuildFullPath(path, entry->d_name, tmpBuf, &allocatedFullPath);

        if (fullPath == NULL) {
            return CLE_AllocFailed;
        }

        struct stat st;
        if (stat(fullPath, &st) == -1) {
            continue;
        }

        if (S_ISDIR(st.st_mode) && self->cfg.recursive.val) {
            INode inode = { .dev = st.st_dev, .ino = st.st_ino };
            if (!CL_ShouldIncludePath(self, fullPath, entry->d_name, true)) {
                free(allocatedFullPath);
                continue;
            }

            if (INS_Contains(&self->seen, inode)) {
                free(allocatedFullPath);
                continue;
            }

            INS_Insert(&self->seen, inode);
            self->dirCount++;

            err = CL_CountRecursive(self, fullPath, depth + 1);
            if (err != CLE_Ok) {
                free(allocatedFullPath);
                goto cleanup;
            }
        } else if (S_ISREG(st.st_mode)) {
            if (st.st_size == 0) continue;

            FileMeta meta = {
                .path = fullPath,
                .size = st.st_size,
                .mtime = st.st_mtime,
            };

            err = CL_HandleFile(self, fullPath, entry->d_name, &meta);
            if (err != CLE_Ok) {
                free(allocatedFullPath);
                goto cleanup;
            }
        }

        free(allocatedFullPath);
        continue;
    }

cleanup:
    if (closedir(dir) == -1) {
        return CLE_CloseDirError;
    }
    return err;
}
