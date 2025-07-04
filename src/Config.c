#include <Config.h>
#include <Definitions.h>
#include <StringList.h>
#include <Utils.h>

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static CFG_Error SetSwitch(CFG_Switch* pswitch, bool value) {
    if (pswitch->setted) {
        return CFGE_RedeclaredFlag;
    }

    pswitch->val = value;
    pswitch->setted = true;
    return CFGE_Ok;
}

CFG_Error CFG_SetPrintMode(Config* self, bool value) {
    return SetSwitch(&self->printMode, value);
}
CFG_Error CFG_SetRecursive(Config* self, bool value) {
    return SetSwitch(&self->recursive, value);
}
CFG_Error CFG_SetDebugMode(Config* self, bool value) {
    return SetSwitch(&self->debugMode, value);
}
CFG_Error CFG_SetLocEnabled(Config* self, bool value) {
    return SetSwitch(&self->locEnabled, value);
}
CFG_Error CFG_SetShowHidden(Config* self, bool value) {
    return SetSwitch(&self->showHidden, value);
}

CFG_Error CFG_SetShowHelp(Config* self, bool value) {
    return SetSwitch(&self->showHelp, value);
}
CFG_Error CFG_SetShowVersion(Config* self, bool value) {
    return SetSwitch(&self->showVersion, value);
}
CFG_Error CFG_SetShowRepo(Config* self, bool value) {
    return SetSwitch(&self->showRepo, value);
}

CFG_Error CFG_SetErrorDetails(Config* self, const char* msg) {
    free(self->errorDetails);

    self->errorDetails = strdup(msg);
    if (self->errorDetails == NULL) {
        return CFGE_AllocFailed;
    }

    return CFGE_Ok;
}

CFG_Error CFG_SetLastUnexpectedArg(Config* self, const char* arg, usize argLen, const char* prefix) {
    free(self->errorDetails);

    usize prefixLen = strlen(prefix);
    self->errorDetails = malloc(prefixLen + argLen + 1); // + null terminator
    if (self->errorDetails == NULL) {
        return CFGE_AllocFailed;
    }

    strcpy(self->errorDetails, prefix);
    strcpy(self->errorDetails + prefixLen, arg);
    return CFGE_Ok;
}

static inline bool parseInt(const char* input, long long* out) {
    errno = 0;
    char* end;
    long long val = strtoll(input, &end, 10);

    if (errno == ERANGE) return false; // out of range
    if (end == input || *end != '\0') return false; // not fully parsed
    *out = val;
    return true;
}

CFG_Error CFG_SetMaxDepth(Config* self, usize maxDepth) {
    if (self->maxDepthSetted) {
        return CFGE_RedeclaredFlag;
    }

    self->maxDepth = (usize)maxDepth;
    self->maxDepthSetted = true;
    return CFGE_Ok;
}

CFG_Error CFG_SetMaxDepthStr(Config* self, const char* maxDepthStr) {
    if (self->maxDepthSetted) {
        return CFGE_RedeclaredFlag;
    }

    long long maxDepth = 0;
    if (!parseInt(maxDepthStr, &maxDepth)) {
        return CFGE_InvalidInputNumber;
    }

    self->maxDepth = (usize)maxDepth;
    self->maxDepthSetted = true;
    return CFGE_Ok;
}

CFG_Error CFG_SetTop(Config* self, usize top) {
    if (self->topSetted) {
        return CFGE_RedeclaredFlag;
    }

    self->top = (usize)top;
    self->topSetted = true;
    return CFGE_Ok;
}

CFG_Error CFG_SetTopStr(Config* self, const char* topStr) {
    if (self->topSetted) {
        return CFGE_RedeclaredFlag;
    }

    long long top = 0;
    if (!parseInt(topStr, &top)) {
        return CFGE_InvalidInputNumber;
    }

    self->top = (usize)top;
    self->topSetted = true;
    return CFGE_Ok;
}

CFG_Error CFG_SetReverse(Config* self, bool reverse) {
    return SetSwitch(&self->reverse, reverse);
}

CFG_Error CFG_SetSortMode(Config* self, CFG_SortMode mode, bool reverse) {
    if (self->sortMode != _SM_NotSetted) {
        return CFGE_RedeclaredFlag;
    }

    self->sortMode = mode;
    if (reverse) {
        return CFG_SetReverse(self, reverse);
    } else {
        self->reverse.val = reverse;
        return CFGE_Ok;
    }
}

CFG_Error CFG_SetSortModeStr(Config* self, const char* modeStr, bool reverse) {
    CFG_SortMode mode = _SM_NotSetted;
    if (StrEql(modeStr, "lines")) {
        mode = SM_Lines;
    } else if (StrEql(modeStr, "path")) {
        mode = SM_Path;
    } else if (StrEql(modeStr, "name")) {
        mode = SM_Name;
    } else if (StrEql(modeStr, "ext")) {
        mode = SM_Ext;
    } else if (StrEql(modeStr, "mtime")) {
        mode = SM_MTime;
    } else if (StrEql(modeStr, "size")) {
        mode = SM_Size;
    } else {
        CFG_SetErrorDetails(self, modeStr);
        return CFGE_InvalidSortMode;
    }

    return CFG_SetSortMode(self, mode, reverse);
}

CFG_Error CFG_Init(Config* self) {
    self->printMode = (CFG_Switch) { false, false };
    self->recursive = (CFG_Switch) { false, false };
    self->reverse   = (CFG_Switch) { false, false };

    self->errorDetails = NULL;
    self->maxDepth = 0;
    self->maxDepthSetted = false;

    self->sortMode = _SM_NotSetted;

    // clang-format off
    StringList* listsToInit[] = {
        &self->includedExtensions, &self->excludedExtensions,
        &self->includedRegexes,    &self->excludedRegexes,
        &self->includedPaths,      &self->excludedPaths,
    };
    // clang-format on
    for (usize i = 0; i < sizeof(listsToInit) / sizeof(StringList*); ++i) {
        SL_Error err = SL_InitReserved(listsToInit[i], 16);
        if (err != SLE_Ok) return CFGE_ListError;
    }

    self->mode = CFGM_Pass;
    return CFGE_Ok;
}

CFG_Error CFG_Destroy(Config* self) {
    // clang-format off
    StringList* listsToDestroy[] = {
        &self->includedExtensions, &self->excludedExtensions,
        &self->includedRegexes,    &self->excludedRegexes,
        &self->includedPaths,      &self->excludedPaths,
    };
    // clang-format on
    for (usize i = 0; i < sizeof(listsToDestroy) / sizeof(StringList*); ++i) {
        SL_Destroy(listsToDestroy[i]);
    }

    free(self->errorDetails);
    self->errorDetails = NULL;

    self->maxDepth = 0;
    self->maxDepthSetted = false;
    self->printMode  =  (CFG_Switch) { false, false };
    self->recursive  =  (CFG_Switch) { false, false };
    self->reverse    =  (CFG_Switch) { false, false };
    self->showHidden =  (CFG_Switch) { false, false };
    self->sortMode = _SM_NotSetted;

    self->mode = CFGM_Pass;
    return CFGE_Ok;
}

CFG_Error CFG_HandleShortOption(Config* self, const char* flag) {
    self->mode = CFGM_Pass;

    while (*flag) {
        switch (*flag) {
        case 'r': {
            CFG_Error err = CFG_SetRecursive(self, true);
            if (err != CFGE_Ok) return err;
        } break;
        case 'v':
        case 'p': {
            CFG_Error err = CFG_SetPrintMode(self, true);
            if (err != CFGE_Ok) return err;
        } break;
        case 'l': {
            CFG_Error err = CFG_SetLocEnabled(self, true);
            if (err != CFGE_Ok) return err;
        }

        case 'e': {
            self->mode = CFGM_CollectingIncludedExtensions;
        } break;
        case 'g': {
            self->mode = CFGM_CollectingExcludedRegexes;
        } break;
        case 'i': {
            self->mode = CFGM_CollectingIncludedPaths;
        } break;
        case 'x': {
            self->mode = CFGM_CollectingExcludedPaths;
        } break;

        default: {
            CFG_Error err = CFG_SetLastUnexpectedArg(self, flag, strlen(flag), "-");
            if (err != CFGE_Ok) return err;
            return CFGE_UnexpectedArgument;
        } break;
        }
        flag++;
    }

    return CFGE_Ok;
}

CFG_Error CFG_HandleLongOption(Config* self, const char* flag) {
    self->mode = CFGM_Pass;

    usize flagLen = strlen(flag);
    if (StrEql(flag, "help")) {
        CFG_SetShowHelp(self, true);
    } else if (StrEql(flag, "version")) {
        CFG_SetShowVersion(self, true);
    } else if (StrEql(flag, "repo")) {
        CFG_SetShowRepo(self, true);
    }

    else if (StrEql(flag, "recursive")) {
        CFG_Error err = CFG_SetRecursive(self, true);
        if (err != CFGE_Ok) return err;
    } else if (StrEql(flag, "no-recursive")) {
        CFG_Error err = CFG_SetRecursive(self, false);
        if (err != CFGE_Ok) return err;
    } else if (StrEql(flag, "verbose") || StrEql(flag, "print")) {
        CFG_Error err = CFG_SetPrintMode(self, true);
        if (err != CFGE_Ok) return err;
    } else if (StrEql(flag, "no-verbose") || StrEql(flag, "no-print")) {
        CFG_Error err = CFG_SetPrintMode(self, false);
        if (err != CFGE_Ok) return err;
    } else if (StrEql(flag, "no-debug")) {
        CFG_Error err = CFG_SetDebugMode(self, false);
        if (err != CFGE_Ok) return err;
    } else if (StrEql(flag, "debug")) {
        CFG_Error err = CFG_SetDebugMode(self, true);
        if (err != CFGE_Ok) return err;
    } else if (StrEql(flag, "loc")) {
        CFG_Error err = CFG_SetLocEnabled(self, true);
        if (err != CFGE_Ok) return err;
    } else if (StrEql(flag, "no-loc")) {
        CFG_Error err = CFG_SetLocEnabled(self, false);
        if (err != CFGE_Ok) return err;
    } else if (StrEql(flag, "show-hidden")) {
        CFG_Error err = CFG_SetShowHidden(self, true);
        if (err != CFGE_Ok) return err;
    } else if (StrEql(flag, "no-show-hidden")) {
        CFG_Error err = CFG_SetShowHidden(self, false);
        if (err != CFGE_Ok) return err;
    }

    else if (StrEql(flag, "ext") || StrEql(flag, "include-ext")) {
        self->mode = CFGM_CollectingIncludedExtensions;
    } else if (StrEql(flag, "regex") || StrEql(flag, "include-regex")) {
        self->mode = CFGM_CollectingIncludedRegexes;
    } else if (StrEql(flag, "exclude-ext")) {
        self->mode = CFGM_CollectingExcludedExtensions;
    } else if (StrEql(flag, "exclude-regex")) {
        self->mode = CFGM_CollectingExcludedRegexes;
    } else if (StrEql(flag, "include") || StrEql(flag, "include-path")) {
        self->mode = CFGM_CollectingIncludedPaths;
    } else if (StrEql(flag, "exclude") || StrEql(flag, "exclude-path")) {
        self->mode = CFGM_CollectingExcludedPaths;
    }

    else if (HasPrefix(flag, "max-depth=")) {
        CFG_Error err = CFG_SetMaxDepthStr(self, flag + strlen("max-depth="));
        if (err != CFGE_Ok) return err;
    } else if (HasPrefix(flag, "sort=")) {
        CFG_Error err = CFG_SetSortModeStr(self, flag + strlen("sort="), false);
        if (err != CFGE_Ok) return err;
    } else if (HasPrefix(flag, "sort-by-")) {
        CFG_Error err = CFG_SetSortModeStr(self, flag + strlen("sort-by-"), false);
        if (err != CFGE_Ok) return err;
    } else if (HasPrefix(flag, "reversed-sort-by-")) {
        CFG_Error err = CFG_SetSortModeStr(self, flag + strlen("reversed-sort-by-"), true);
        if (err != CFGE_Ok) return err;
    } else if (StrEql(flag, "reverse") || StrEql(flag, "reverse-sort")) {
        CFG_Error err = CFG_SetReverse(self, true);
        if (err != CFGE_Ok) return err;
    }

    else {
        CFG_Error err = CFG_SetLastUnexpectedArg(self, flag, flagLen, "--");
        if (err != CFGE_Ok) return err;

        return CFGE_UnexpectedArgument;
    }

    return CFGE_Ok;
}

CFG_Error CFG_SetDefauts(Config* self) {
    const bool defaultRecursiveVal = true;
    const bool defaultVerboseVal = false;
    const bool defaultReverseVal = false;
    const bool defaultShowHiddenVal = false;
    const usize defaultMaxDepthVal = 50;
    const char* const defaultPathVal = ".";

    CFG_Error err = CFGE_Ok;
    if (!self->recursive.setted) {
        err = CFG_SetRecursive(self, defaultRecursiveVal);
    }
    if (!self->printMode.setted) {
        err = CFG_SetPrintMode(self, defaultVerboseVal);
    }
    if (!self->reverse.setted) {
        err = CFG_SetReverse(self, defaultReverseVal);
    }
    if (!self->showHidden.setted) {
        err = CFG_SetShowHidden(self, defaultShowHiddenVal);
    }

    if (!self->maxDepthSetted) {
        err = CFG_SetMaxDepth(self, defaultMaxDepthVal);
    }
    if (self->includedPaths.len <= 0) {
        SL_Append(&self->includedPaths, defaultPathVal);
    }
    if (self->sortMode == _SM_NotSetted) {
        self->sortMode = SM_NotSort;
    }

    self->reverse.val = !self->reverse.val;
    return err;
}

CFG_Error CFG_Parse(Config* self, int argc, char** argv) {
    self->mode = CFGM_CollectingIncludedPaths;

    for (usize i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        if (HasPrefix(arg, "--")) {
            const char* flag = arg + 2;
            CFG_Error err = CFG_HandleLongOption(self, flag);
            if (err != CFGE_Ok) return err;
        } else if (HasPrefix(arg, "-")) {
            const char* flag = arg + 1;
            CFG_Error err = CFG_HandleShortOption(self, flag);
            if (err != CFGE_Ok) return err;
        } else {
            switch (self->mode) {
            case CFGM_CollectingIncludedExtensions: {
                SL_Error err = SL_Append(&self->includedExtensions, arg);
                if (err != SLE_Ok) return CFGE_ListError;
            } break;
            case CFGM_CollectingExcludedExtensions: {
                SL_Error err = SL_Append(&self->excludedExtensions, arg);
                if (err != SLE_Ok) return CFGE_ListError;
            } break;

            case CFGM_CollectingIncludedRegexes: {
                SL_Error err = SL_Append(&self->includedRegexes, arg);
                if (err != SLE_Ok) return CFGE_ListError;
            } break;
            case CFGM_CollectingExcludedRegexes: {
                SL_Error err = SL_Append(&self->excludedRegexes, arg);
                if (err != SLE_Ok) return CFGE_ListError;
            } break;

            case CFGM_CollectingIncludedPaths: {
                SL_Error err = SL_Append(&self->includedPaths, arg);
                if (err != SLE_Ok) return CFGE_ListError;
            } break;
            case CFGM_CollectingExcludedPaths: {
                SL_Error err = SL_Append(&self->excludedPaths, arg);
                if (err != SLE_Ok) return CFGE_ListError;
            } break;

            case CFGM_Pass:
                return CFGE_UnexpectedArgument;
            }
        }
    }

    CFG_SetDefauts(self);
    self->reverse.val = -self->reverse.val;
    return CFGE_Ok;
}

static inline const char* s(bool val) {
    if (val) return "true";
    return "false";
}

CFG_Error CFG_DebugPrint(Config* self, FILE* out, const char* indent) {
    if (indent == NULL) indent = "    ";
    if (out == NULL) out = stdout;

    fprintf(out, "&(Config) {\n");

    fprintf(out, "%s.includedExtensions = ", indent);
    SL_Print(&self->includedExtensions, out);
    fputc('\n', out);

    fprintf(out, "%s.includedRegexes = ", indent);
    SL_Print(&self->includedRegexes, out);
    fputc('\n', out);

    fprintf(out, "%s.excludedExtensions = ", indent);
    SL_Print(&self->excludedExtensions, out);
    fputc('\n', out);

    fprintf(out, "%s.excludedRegexes = ", indent);
    SL_Print(&self->excludedRegexes, out);
    fputc('\n', out);

    fprintf(out, "%s.includedPaths = ", indent);
    SL_Print(&self->includedPaths, out);
    fputc('\n', out);

    fprintf(out, "%s.excludedPaths = ", indent);
    SL_Print(&self->excludedPaths, out);
    fputc('\n', out);

    CFG_Switch* switches[] = {
        &self->printMode,
        &self->recursive,
        &self->reverse,
        &self->debugMode,
        &self->locEnabled,
        &self->showHidden,
        &self->showHelp,
        &self->showVersion,
        &self->showRepo,
    };
    const char* names[] = {
        "printMode",
        "recursive",
        "reverse",
        "debugMode",
        "locEnabled",
        "showHidden",
        "showHelp",
        "showVersion",
        "showRepo",
    };

    usize switchesCount = sizeof(switches) / sizeof(switches[0]);

    for (size_t i = 0; i < switchesCount; i++) {
        fprintf(out, "%s.%s = %s\n", indent, names[i], s(switches[i]->val));
    }

    fprintf(out, "%s.maxDepth = %zu\n", indent, self->maxDepth);
    fprintf(out, "%s.maxDepthSetted = %s\n", indent, s(self->maxDepthSetted));

    fprintf(out, "%s.sortMode = %d\n", indent, self->sortMode);

    fprintf(out, "%s.errorDetails = '%s'\n", indent, self->errorDetails);

    fputc('}', out);
    fputc('\n', out);
    return CFGE_Ok;
}
