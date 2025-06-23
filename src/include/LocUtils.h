#ifndef LOC_UTILS_H
#define LOC_UTILS_H

#include <LocSettings.h>

#include <Definitions.h>
#include <Utils.h>

#include <stdbool.h>

bool GetLocLangFor(const char* filename, const LocEntry** out);

// Returns pointer to the matching delimiter if found, else NULL
static inline const char* StartsWithAny(const char* str, const char* const* delimiters) {
    if (delimiters == NULL || str == NULL) return NULL;

    for (int i = 0; delimiters[i]; ++i) {
        const usize len = strlen(delimiters[i]);
        if (StrNEql(str, delimiters[i], len)) {
            return delimiters[i]; // match found
        }
    }
    return NULL;
}

// Returns pointer to the matching delimiter if found, else NULL
static inline const StringDelimPair* StartsWithAnyPair(const char* str, const StringDelimPair* pairs) {
    if (pairs == NULL || str == NULL) return NULL;

    for (int i = 0; pairs[i].start != NULL; ++i) {
        const usize len = strlen(pairs[i].start);
        if (StrNEql(str, pairs[i].start, len)) {
            return &pairs[i]; // match found
        }
    }
    return NULL;
}

static inline LocStat* MergeResults(LocStat* dst, const LocStat* src) {
    dst->codeLines += src->codeLines;
    dst->commentLines += src->commentLines;
    dst->preprocessorLines += src->preprocessorLines;

    dst->emptyLines += src->emptyLines;
    dst->totalLines += src->totalLines;
    return dst;
}

#endif // LOC_UTILS_H
