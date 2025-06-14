#include <Definitions.h>
#include <StringList.h>

#include <stdbool.h>
#include <stdio.h>

#ifndef CONFIG_H
#define CONFIG_H

typedef enum CFG_Error {
    CFGE_Ok = 0,
    CFGE_UnexpectedArgument,
    CFGE_RedeclaredFlag,
    CFGE_AllocFailed,
    CFGE_ListError,
    CFGE_InvalidInputNumber,
} CFG_Error;

typedef enum CFG_Mode {
    CFGM_Pass = 0,
    CFGM_CollectingIncludedExtensions,
    CFGM_CollectingIncludedRegexes,
    CFGM_CollectingExcludedExtensions,
    CFGM_CollectingExcludedRegexes,
    CFGM_CollectingIncludedPaths,
    CFGM_CollectingExcludedPaths,
} CFG_Mode;

typedef enum CFG_SortMode {
    SM_NotSort,
    SM_Lines,
    SM_Ext,
    SM_Name,
    SM_Path,
} CFG_SortMode;

typedef struct CFG_Switch {
    bool val;
    bool setted;
} CFG_Switch;

typedef struct Config {
    StringList includedExtensions, excludedExtensions;
    StringList includedRegexes, excludedRegexes;

    StringList includedPaths;
    StringList excludedPaths;

    CFG_Switch verbose;
    CFG_Switch recursive;

    usize maxDepth;
    bool maxDepthSetted;

    CFG_Mode mode;
    char* lastUnexpectedArg;
} Config;

CFG_Error CFG_SetVerbose(Config* self, bool value);
CFG_Error CFG_SetRecursive(Config* self, bool value);
CFG_Error CFG_SetLastUnexpectedArg(Config* self, const char* arg, usize argLen, const char* prefix);

CFG_Error CFG_Init(Config* self);
CFG_Error CFG_Destroy(Config* self);

CFG_Error CFG_HandleShortOption(Config* self, const char* flag);
CFG_Error CFG_HandleLongOption(Config* self, const char* flag);

CFG_Error CFG_SetDefaults(Config* self);
CFG_Error CFG_Parse(Config* self, int argc, char** argv);
CFG_Error CFG_DebugBump(Config* self, FILE* out, const char* indent);

#endif // CONFIG_H
