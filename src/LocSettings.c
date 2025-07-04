#include <LocSettings.h>
#include <bits/posix1_lim.h>
#include <tgmath.h>

#include <Definitions.h>
#include <stdlib.h>

static LocEntry locEntries[] = {
    (LocEntry) {
        .langName = "C",
        .extensions = (const char*[]) { "c", "h", NULL },
        .names = NULL,

        .stringDelims = (const char*[]) { "\"", NULL },
        .multilineStringDelims = NULL,
        .charDelims = (const char*[]) { "'", NULL },

        .commentStarts = (const char*[]) { "//", NULL },
        .allowCommentContinues = true,

        .multilineCommentDelimPairs = (const StringDelimPair[]) { {"/*", "*/"}, {NULL} },

        .ppDirectiveStarts = (const char*[]) { "#", NULL },
        .allowPPDirectiveContinues = true,
    },
    (LocEntry) {
        .langName = "C++",
        .extensions = (const char*[]) { "cpp", "cxx", "cc", "hpp", "hxx", "tpp", NULL },
        .names = NULL,

        .stringDelims = (const char*[]) { "\"", NULL },
        .multilineStringDelims = NULL, // currently C++ raw strings R“...()...” are too complicated for LocParser. TODO
        .charDelims = (const char*[]) { "'", NULL },

        .commentStarts = (const char*[]) { "//", NULL },
        .allowCommentContinues = true,

        .multilineCommentDelimPairs = (const StringDelimPair[]) { {"/*", "*/"}, {NULL} },

        .ppDirectiveStarts = (const char*[]) { "#", NULL },
        .allowPPDirectiveContinues = true,
    },
    (LocEntry) {
        .langName = "Go",
        .extensions = (const char*[]) { "go", NULL },
        .names = NULL,

        .stringDelims = (const char*[]) { "\"", NULL },
        .multilineStringDelims = (const char*[]) { "`", NULL },

        .charDelims = (const char*[]) { "'", NULL },

        .commentStarts = (const char*[]) { "//", NULL },
        .allowCommentContinues = false,

        .multilineCommentDelimPairs = (const StringDelimPair[]) { {"/*", "*/"}, {NULL} },

        .ppDirectiveStarts = (const char*[]) { "//go:", NULL },
        .allowPPDirectiveContinues = true,
    },
    (LocEntry) {
        .langName = "Java",
        .extensions = (const char*[]) { "java", NULL },
        .names = NULL,

        .stringDelims = (const char*[]) { "\"", NULL },
        .multilineStringDelims = (const char*[]) { "\"\"\"", NULL },

        .charDelims = (const char*[]) { "'", NULL },

        .commentStarts = (const char*[]) { "//", NULL },
        .allowCommentContinues = false,

        .multilineCommentDelimPairs = (const StringDelimPair[]) { {"/*", "*/"}, {NULL} },

        .ppDirectiveStarts = NULL,
        .allowPPDirectiveContinues = false,
    },
    (LocEntry) {
        .langName = "Python",
        .extensions = (const char*[]) { "py", "py3", "pyi", NULL },
        .names = NULL,

        .stringDelims = (const char*[]) { "\"", "'", NULL },
        .multilineStringDelims = (const char*[]) { "\"\"\"", "'''", NULL },

        .charDelims = NULL,

        .commentStarts = (const char*[]) { "#", NULL },
        .allowCommentContinues = false,

        // """ Hello! """ is not an comment
        .multilineCommentDelimPairs = NULL,

        .ppDirectiveStarts = NULL,
        .allowPPDirectiveContinues = false,
    },
    (LocEntry) {
        .langName = "Makefile",
        .extensions = NULL,
        .names = (const char*[]) { "Makefile", "makefile", "GNUmakefile", NULL },

        .stringDelims = NULL,
        .multilineStringDelims = NULL,

        .charDelims = NULL,

        .commentStarts = (const char*[]) { "#", NULL },
        .allowCommentContinues = false,

        .multilineCommentDelimPairs = NULL,

        .ppDirectiveStarts = NULL,
        .allowPPDirectiveContinues = false,
    },
    (LocEntry) {
        .langName = "Shell Script",
        .extensions = (const char*[]) { "sh", "bash", "zsh", "ksh", "fish", NULL },
        .names = NULL,

        .stringDelims = NULL,
        .multilineStringDelims = (const char*[]) { "\"", "'", NULL },

        .charDelims = NULL,

        .commentStarts = (const char*[]) { "#", NULL },
        .allowCommentContinues = false,

        .multilineCommentDelimPairs = NULL,

        .ppDirectiveStarts = NULL,
        .allowPPDirectiveContinues = false,
    },
    (LocEntry) {
        .langName = "Rust",
        .extensions = (const char*[]) { "rs", NULL },
        .names = NULL,

        .stringDelims = (const char*[]) { "\"", NULL },
        .multilineStringDelims = NULL,

        .charDelims = (const char*[]) { "'", NULL },

        .commentStarts = (const char*[]) { "//", NULL },
        .allowCommentContinues = true,

        .multilineCommentDelimPairs = (const StringDelimPair[]) { {"/*", "*/"}, {NULL} },

        .ppDirectiveStarts = NULL,
        .allowPPDirectiveContinues = false,
    },
    (LocEntry) {
        .langName = "JavaScript",
        .extensions = (const char*[]) { "js", "mjs", "cjs", NULL },
        .names = NULL,

        .stringDelims = (const char*[]) { "\"", "'", "`", NULL },
        .multilineStringDelims = (const char*[]) { "`", NULL },

        .charDelims = NULL,

        .commentStarts = (const char*[]) { "//", NULL },
        .allowCommentContinues = true,

        .multilineCommentDelimPairs = (const StringDelimPair[]) { {"/*", "*/"}, {NULL} },

        .ppDirectiveStarts = NULL,
        .allowPPDirectiveContinues = false,
    },
    (LocEntry) {
        .langName = "TypeScript",
        .extensions = (const char*[]) { "ts", "tsx", NULL },
        .names = NULL,

        .stringDelims = (const char*[]) { "\"", "'", "`", NULL },
        .multilineStringDelims = (const char*[]) { "`", NULL },

        .charDelims = NULL,

        .commentStarts = (const char*[]) { "//", NULL },
        .allowCommentContinues = true,

        .multilineCommentDelimPairs = (const StringDelimPair[]) { {"/*", "*/"}, {NULL} },

        .ppDirectiveStarts = NULL,
        .allowPPDirectiveContinues = false,
    },
    (LocEntry) {
        .langName = "HTML",
        .extensions = (const char*[]) { "html", "htm", NULL },
        .names = NULL,

        .stringDelims = (const char*[]) { "\"", "'", NULL },
        .multilineStringDelims = NULL,

        .charDelims = NULL,

        .commentStarts = NULL,
        .allowCommentContinues = false,

        .multilineCommentDelimPairs = (const StringDelimPair[]) { {"<!--", "-->"}, {NULL} },

        .ppDirectiveStarts = NULL,
        .allowPPDirectiveContinues = false,
    },
    (LocEntry) {
        .langName = "CSS",
        .extensions = (const char*[]) { "css", NULL },
        .names = NULL,

        .stringDelims = (const char*[]) { "\"", "'", NULL },
        .multilineStringDelims = NULL,

        .charDelims = NULL,

        .commentStarts = NULL,
        .allowCommentContinues = false,

        .multilineCommentDelimPairs = (const StringDelimPair[]) { {"/*", "*/"}, {NULL} },

        .ppDirectiveStarts = NULL,
        .allowPPDirectiveContinues = false,
    },
    (LocEntry) {
        .langName = "JSON",
        .extensions = (const char*[]) { "json", NULL },
        .names = NULL,

        .stringDelims = (const char*[]) { "\"", NULL },
        .multilineStringDelims = NULL,

        .charDelims = NULL,

        .commentStarts = NULL,
        .allowCommentContinues = false,

        .multilineCommentDelimPairs = NULL,

        .ppDirectiveStarts = NULL,
        .allowPPDirectiveContinues = false,
    },
    (LocEntry) {
        .langName = "YAML",
        .extensions = (const char*[]) { "yaml", "yml", NULL },
        .names = NULL,

        .stringDelims = (const char*[]) { "\"", "'", NULL },
        .multilineStringDelims = NULL,

        .charDelims = NULL,

        .commentStarts = (const char*[]) { "#", NULL },
        .allowCommentContinues = false,

        .multilineCommentDelimPairs = NULL,

        .ppDirectiveStarts = NULL,
        .allowPPDirectiveContinues = false,
    },
    (LocEntry) {
        .langName = "Perl",
        .extensions = (const char*[]) { "pl", "pm", "t", NULL },
        .names = NULL,

        .stringDelims = (const char*[]) { "\"", "'", NULL },
        .multilineStringDelims = NULL,

        .charDelims = NULL,

        .commentStarts = (const char*[]) { "#", NULL },
        .allowCommentContinues = false,

        .multilineCommentDelimPairs = NULL,

        .ppDirectiveStarts = NULL,
        .allowPPDirectiveContinues = false,
    },
    (LocEntry) {
        .langName = "Ruby",
        .extensions = (const char*[]) { "rb", NULL },
        .names = NULL,

        .stringDelims = (const char*[]) { "\"", "'", NULL },
        .multilineStringDelims = (const char*[]) { "\"\"\"", NULL },

        .charDelims = NULL,

        .commentStarts = (const char*[]) { "#", NULL },
        .allowCommentContinues = false,

        .multilineCommentDelimPairs = NULL,

        .ppDirectiveStarts = NULL,
        .allowPPDirectiveContinues = false,
    },
    (LocEntry) {
        .langName = "PHP",
        .extensions = (const char*[]) { "php", NULL },
        .names = NULL,

        .stringDelims = (const char*[]) { "\"", "'", NULL },
        .multilineStringDelims = NULL,

        .charDelims = NULL,

        .commentStarts = (const char*[]) { "//", "#", NULL },
        .allowCommentContinues = true,

        .multilineCommentDelimPairs = (const StringDelimPair[]) { {"/*", "*/"}, {NULL} },

        .ppDirectiveStarts = NULL,
        .allowPPDirectiveContinues = false,
    },
};

const LocEntry* GetLocEntries() {
    return locEntries;
}

usize GetLocEntriesCount() {
    return sizeof(locEntries) / sizeof(LocEntry);
}
