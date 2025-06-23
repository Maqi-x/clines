#ifndef UTILS_H
#define UTILS_H

#include <Definitions.h>

#include <stdbool.h>
#include <string.h>

#define EXPAND(expr) expr
#define STRINGIFY(expr) #expr
#define STRINGIFY_EXPANDED(expr) STRINGIFY(expr)

#define DEBUG_HERE (MSG_ShowDebugLog("HERE (" STRINGIFY_EXPANDED(__FILE__) ": " STRINGIFY_EXPANDED(__LINE__) ")"))

static inline bool HasPrefix(const char* str, const char* prefix) {
    const usize prefixLen = strlen(prefix);
    const usize strLen = strlen(str);

    if (strLen < prefixLen) return false;
    return memcmp(str, prefix, prefixLen) == 0;
}

static inline bool HasSuffix(const char* str, const char* suffix) {
    const usize suffixLen = strlen(suffix);
    const usize strLen = strlen(str);

    if (strLen < suffixLen) return false;
    return memcmp(str + strLen - suffixLen, suffix, suffixLen) == 0;
}

static inline bool HasPrefixChar(const char* str, char prefix) {
    if (str == NULL || str[0] == '\0') return false;
    return str[0] == prefix;
}

static inline bool HasSuffixChar(const char* str, char suffix) {
    if (str == NULL || str[0] == '\0') return false;
    const usize strLen = strlen(str);
    return str[strLen - 1] == suffix;
}

static inline bool StrEql(const char* s1, const char* s2) {
    return strcmp(s1, s2) == 0;
}

static inline bool StrNEql(const char* s1, const char* s2, usize n) {
    return strncmp(s1, s2, n) == 0;
}

static inline const char* GetExtension(const char* filename) {
    const usize filenameLen = strlen(filename);
    for (isize i = (isize)filenameLen - 1; i >= 0; --i) {
        if (filename[i] == '.') {
            return filename + i + 1;
        }
    }

    return NULL;
}

// Returns the base name of the path
static inline const char* GetBaseName(const char* path) {
    const char* lastSlash = path;
    for (const char* p = path; *p; ++p) {
        if (*p == '/') lastSlash = p + 1;
    }
    return lastSlash;
}

// Returns the directory name of the path
static inline const char* GetDirName(const char* path) {
    const char* lastSlash = path;
    for (const char* p = path; *p; ++p) {
        if (*p == '/') lastSlash = p;
    }
    return lastSlash;
}

// Returns if the string is contained in the array
static inline bool ContainsStr(const char* const* array, const char* target) {
    for (const char* const* p = array; *p; ++p) {
        if (StrEql(*p, target)) return true;
    }
    return false;
}

// Returns if the character is contained in the array
static inline bool ContainsStrChar(const char* const* array, char target) {
    for (const char* const* p = array; *p; ++p) {
        if ((*p)[0] == target && (*p)[1] == '\0') {
            return true;
        }
    }
    return false;
}

#if defined(__GNUC__) || defined(__clang__)
#    define ATTR_FORMAT(type, fmtIndex, argsIndex) __attribute__((format(type, fmtIndex, argsIndex)))
#    define ATTR_MALLOC __attribute__((malloc))
#else
#    define ATTR_FORMAT(type, fmtIndex, argsIndex)
#    define ATTR_MALLOC
#endif

#endif // UTILS_H
