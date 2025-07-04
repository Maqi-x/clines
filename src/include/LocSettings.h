#ifndef LOC_SETTINGS_H
#define LOC_SETTINGS_H

#include <Definitions.h>

#include <stdbool.h>
#include <stdio.h>

typedef struct StringDelimPair {
    const char* start;
    const char* end;
} StringDelimPair;

typedef struct CharDelimPair {
    char start;
    char end;
} CharDelimPair;

typedef struct LocEntry {
    const char* langName;
    const char** extensions;
    const char** names; // e.g. "Makefile"

    const char** stringDelims;
    const char** multilineStringDelims;

    const char** charDelims;

    const char** commentStarts;
    bool allowCommentContinues;

    const StringDelimPair* multilineCommentDelimPairs;

    const char** ppDirectiveStarts;
    bool allowPPDirectiveContinues;
} LocEntry;

typedef struct LocStat {
    usize emptyLines;
    usize commentLines;
    usize codeLines;
    usize preprocessorLines;

    usize totalLines;
} LocStat;

static inline bool LS_Eql(const LocStat* a, const LocStat* b) {
    if (a->totalLines != b->totalLines) return false;
    return a->emptyLines == b->emptyLines &&
           a->commentLines == b->commentLines &&
           a->codeLines == b->codeLines &&
           a->preprocessorLines == b->preprocessorLines;
}

static inline void LS_DebugBump(LocStat* stat, FILE* out, const char* indent) {
    if (out == NULL) out = stdout;
    if (indent == NULL) indent = "    ";

    fprintf(out, "(LocStat) {\n");
    fprintf(out, "%s.emptyLines = %zu,\n", indent, stat->emptyLines);
    fprintf(out, "%s.commentLines = %zu,\n", indent, stat->commentLines);
    fprintf(out, "%s.codeLines: %zu,\n", indent, stat->codeLines);
    fprintf(out, "%s.preprocessorLines = %zu,\n", indent, stat->preprocessorLines);

    fprintf(out, "%s.totalLines = %zu,\n", indent, stat->totalLines);
    fprintf(out, "}\n");
}

#define LOC_LANG_C (&(GetLocEntries()[0]))
#define LOC_LANG_CPP (&(GetLocEntries()[1]))
#define LOC_LANG_GO (&(GetLocEntries()[2]))
#define LOC_LANG_JAVA (&(GetLocEntries()[3]))
#define LOC_LANG_PYTHON (&(GetLocEntries()[4]))
#define LOC_LANG_MAKEFILE (&(GetLocEntries()[5]))
#define LOC_LANG_SHELL (&(GetLocEntries()[6]))
#define LOC_LANG_RUST (&(GetLocEntries()[7]))
#define LOC_LANG_JS (&(GetLocEntries()[8]))
#define LOC_LANG_TS (&(GetLocEntries()[9]))
#define LOC_LANG_HTML (&(GetLocEntries()[10]))
#define LOC_LANG_CSS (&(GetLocEntries()[11]))
#define LOC_LANG_JSON (&(GetLocEntries()[12]))
#define LOC_LANG_YAML (&(GetLocEntries()[13]))
#define LOC_LANG_PERL (&(GetLocEntries()[14]))
#define LOC_LANG_PHP (&(GetLocEntries()[15]))

const LocEntry* GetLocEntries();
usize GetLocEntriesCount();

#endif // LOC_SETTINGS_H
