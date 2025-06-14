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

CFG_Error CFG_SetVerbose(Config* self, bool value) {
    return SetSwitch(&self->verbose, value);
}

CFG_Error CFG_SetRecursive(Config* self, bool value) {
    return SetSwitch(&self->recursive, value);
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

CFG_Error CFG_SetSortMode(Config* self, CFG_SortMode mode) {
    if (self->sortMode != _SM_NotSetted) {
        return CFGE_RedeclaredFlag;
    }
    self->sortMode = mode;
    return CFGE_Ok;
}

CFG_Error CFG_SetSortModeStr(Config* self, const char* modeStr) {
    CFG_SortMode mode = _SM_NotSetted;
    if (StrEql(modeStr, "lines")) {
        mode = SM_Lines;
    } else if (StrEql(modeStr, "path")) {
        mode = SM_Path;
    } else if (StrEql(modeStr, "name")) {
        mode = SM_Name;
    } else if (StrEql(modeStr, "ext")) {
        mode = SM_Ext;
    } else {
        printf("%s\n", modeStr);
        CFG_SetErrorDetails(self, modeStr);
        return CFGE_InvalidSortMode;
    }

    return CFG_SetSortMode(self, mode);
}

CFG_Error CFG_Init(Config* self) {
    self->verbose = (CFG_Switch) { false, false };
    self->recursive = (CFG_Switch) { false, false };

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
    self->verbose = (CFG_Switch) { false };
    self->recursive = (CFG_Switch) { false };
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
        case 'v': {
            CFG_Error err = CFG_SetVerbose(self, true);
            if (err != CFGE_Ok) return err;
        } break;

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
    if (StrEql(flag, "recursive")) {
        CFG_Error err = CFG_SetRecursive(self, true);
        if (err != CFGE_Ok) return err;
    } else if (StrEql(flag, "no-recursive")) {
        CFG_Error err = CFG_SetRecursive(self, false);
        if (err != CFGE_Ok) return err;
    } else if (StrEql(flag, "verbose")) {
        CFG_Error err = CFG_SetVerbose(self, true);
        if (err != CFGE_Ok) return err;
    } else if (StrEql(flag, "no-verbose")) {
        CFG_Error err = CFG_SetVerbose(self, false);
        if (err != CFGE_Ok) return err;

    } else if (StrEql(flag, "ext") || StrEql(flag, "include-ext")) {
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

    } else if (HasPrefix(flag, "max-depth=")) {
        CFG_Error err = CFG_SetMaxDepthStr(self, flag + strlen("max-depth="));
        if (err != CFGE_Ok) return err;
    } else if (HasPrefix(flag, "sort=")) {
        CFG_Error err = CFG_SetSortModeStr(self, flag + strlen("sort="));
        if (err != CFGE_Ok) return err;
    } else if (HasPrefix(flag, "sort-by-")) {
        CFG_Error err = CFG_SetSortModeStr(self, flag + strlen("sort-by-"));
        if (err != CFGE_Ok) return err;
    } else {
        CFG_Error err = CFG_SetLastUnexpectedArg(self, flag, flagLen, "--");
        if (err != CFGE_Ok) return err;

        return CFGE_UnexpectedArgument;
    }

    return CFGE_Ok;
}

CFG_Error CFG_SetDefauts(Config* self) {
    const bool defaultRecursiveVal = false;
    const bool defaultVerboseVal = false;
    const usize defaultMaxDepthVal = 50;
    const char* const defaultPathVal = ".";

    CFG_Error err = CFGE_Ok;
    if (!self->recursive.setted) {
        err = CFG_SetRecursive(self, defaultRecursiveVal);
    }
    if (!self->verbose.setted) {
        err = CFG_SetVerbose(self, defaultVerboseVal);
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

    return err;
}

CFG_Error CFG_Parse(Config* self, int argc, char** argv) {
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
    return CFGE_Ok;
}

static const char* s(bool val) {
    if (val) return "true";
    return "false";
}

CFG_Error CFG_DebugBump(Config* self, FILE* out, const char* indent) {
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

    fprintf(out, "%s.recursive = %s\n", indent, s(self->recursive.val));
    fprintf(out, "%s.verbose = %s\n", indent, s(self->verbose.val));

    fprintf(out, "%s.maxDepth = %zu\n", indent, self->maxDepth);
    fprintf(out, "%s.maxDepthSetted = %s\n", indent, s(self->maxDepthSetted));

    fprintf(out, "%s.sortMode = %d\n", indent, self->sortMode);

    fprintf(out, "%s.errorDetails = '%s'\n", indent, self->errorDetails);

    fputc('}', out);
    fputc('\n', out);
    return CFGE_Ok;
}
