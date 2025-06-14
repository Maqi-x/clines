#include <Definitions.h>

#include <stdbool.h>
#include <string.h>

#ifndef UTILS_H
#define UTILS_H

static inline bool HasPrefix(const char* str, const char* prefix) {
    usize prefixLen = strlen(prefix);
    if (strlen(str) < prefixLen) return false;
    return memcmp(str, prefix, prefixLen) == 0;
}

static inline bool HasSuffix(const char* str, const char* suffix) {
    usize suffixLen = strlen(suffix);
    usize strLen = strlen(str);

    if (strLen < suffixLen) return false;
    return memcmp(str + strLen - suffixLen, suffix, suffixLen) == 0;
}

static inline bool StrEql(const char* s1, const char* s2) {
    return strcmp(s1, s2) == 0;
}

static inline const char* GetExtension(const char* filename) {
    usize filenameLen = strlen(filename);
    for (isize i = (isize)filenameLen - 1; i >= 0; --i) {
        if (filename[i] == '.') {
            return filename + i + 1;
        }
    }

    return NULL;
}

#endif // UTILS_H
