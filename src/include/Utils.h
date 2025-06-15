#ifndef UTILS_H
#define UTILS_H

#include <Definitions.h>

#include <stdbool.h>
#include <string.h>

#define EXPAND(expr) expr
#define STRINGIFY(expr) #expr
#define STRINGIFY_EXPANDED(expr) STRINGIFY(expr)

#define HERE (MSG_ShowDebugLog("HERE (" STRINGIFY_EXPANDED(__FILE__) ": " STRINGIFY_EXPANDED(__LINE__) ")"))

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

static inline const char* GetBaseName(const char* path) {
    const char* lastSlash = path;
    for (const char* p = path; *p; ++p) {
        if (*p == '/') lastSlash = p + 1;
    }
    return lastSlash;
}

#if defined(__GNUC__) || defined(__clang__)
#    define ATTR_FORMAT(type, fmtIndex, argsIndex) __attribute__((format(type, fmtIndex, argsIndex)))
#    define ATTR_MALLOC __attribute__((malloc))
#else
#    define ATTR_FORMAT(type, fmtIndex, argsIndex)
#    define ATTR_MALLOC
#endif

#endif // UTILS_H
