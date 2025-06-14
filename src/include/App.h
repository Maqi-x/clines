#ifndef CLINES_APP_H
#define CLINES_APP_H

#include <Config.h>
#include <Definitions.h>
#include <LineCounterList.h>

#include <regex.h>

typedef enum CL_Error {
    CLE_Ok,
    CLE_ReadDirError,
    CLE_CloseDirError,
    CLE_FileOpenError,
    CLE_RegexError,
    CLE_AllocFailed,
    CLE_InvalidArgError,
    CLE_NoSuchFileOrDir,
    CLE_ConfigError,
    CLE_ListError,

    CLE_Todo,
    CLE_InternalError,
} CL_Error;

typedef struct CLines {
    Config cfg;

    usize linesCount;
    usize fileCount;
    usize dirCount;

    regex_t* includedRegexes;
    usize includedRegexesCount;

    regex_t* excludedRegexes;
    usize excludedRegexesCount;

    char** excludedPaths;
    usize excludedPathsCount;

    LineCounterList files;

    char* currentPath;
    char* errorDetails;
} CLinesApp;

CL_Error CL_Init(CLinesApp* self);
CL_Error CL_Destroy(CLinesApp* self);

bool CL_ShouldIncludePath(CLinesApp* self, const char* filename);
CL_Error CL_HandleFile(CLinesApp* self, const char* path, const char* name);
CL_Error CL_CountRecursive(CLinesApp* self, const char* path, usize depth);
CL_Error CL_ResetCounter(CLinesApp* self);

bool CL_IsExcluded(CLinesApp* self, const char* fullPath);
CL_Error CL_LoadIncludedRegexes(CLinesApp* self);
CL_Error CL_LoadExcludedRegexes(CLinesApp* self);

CL_Error CL_LoadConfig(CLinesApp* self, int argc, char** argv);

int CL_Run(CLinesApp* self, int argc, char** argv);

#endif
