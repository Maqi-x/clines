#ifndef CLINES_APP_H
#define CLINES_APP_H

#include <Config.h>
#include <Utils.h>

#include <Definitions.h>
#include <INodeSet.h>
#include <LineCounterList.h>
#include <LocSettings.h>
#include <LocParser.h>

#include <regex.h>

typedef enum CL_Error {
    CLE_Ok,
    CLE_ReadDirError,
    CLE_CloseDirError,
    CLE_FileOpenError,
    CLE_FileReadError,
    CLE_RegexError,
    CLE_AllocFailed,
    CLE_InvalidArgError,
    CLE_NoSuchFileOrDir,

    CLE_ConfigError,
    CLE_ListError,
    CLE_SetError,
    CLE_LocError,

    CLE_Todo,
    CLE_InternalError,
} CL_Error;

typedef struct CLines {
    Config cfg;

    usize linesCount;
    usize fileCount;
    usize dirCount;

    INodeSet seen;

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

CL_Error CL_SetErrorDetails(CLinesApp* self, const char* msg);
CL_Error CL_SetErrorDetailsf(CLinesApp* self, const char* fmt, ...) ATTR_FORMAT(printf, 2, 3);

CL_Error CL_MapAndExceptCFG(CLinesApp* self, CFG_Error cerr);
CL_Error CL_MapAndExceptLCL(CLinesApp* self, LCL_Error lcerr);
CL_Error CL_MapAndExceptINS(CLinesApp* self, INS_Error inerr);
CL_Error CL_MapAndExceptCL(CLinesApp* self, CL_Error err);
CL_Error MapAndExceptLP(CLinesApp* self, LP_Error lperr);

bool CL_ShouldIncludePath(CLinesApp* self, const char* path, const char* name, bool isDir);
CL_Error CL_HandleFile(CLinesApp* self, const char* formattedPath, const char* resolvedPath, const char* name, FileMeta* meta);
CL_Error CL_HandleFileWithLoc(CLinesApp* self, const char* formattedPath, const char* resolvedPath, const char* name, FileMeta* meta);
CL_Error CL_CountRecursive(CLinesApp* self, const char* path, usize depth);
CL_Error CL_ResetCounter(CLinesApp* self);

bool CL_IsExcluded(CLinesApp* self, const char* fullPath);
CL_Error CL_LoadIncludedRegexes(CLinesApp* self);
CL_Error CL_LoadExcludedRegexes(CLinesApp* self);
CL_Error CL_LoadExcludedPaths(CLinesApp* self);
CL_Error CL_LoadConfig(CLinesApp* self, int argc, char** argv);

CL_Error CL_PrintFiles(CLinesApp* self);
CL_Error CL_ApplySort(CLinesApp* self);
CL_Error CL_PrintLocStat(CLinesApp* self, LocStat* stat, usize indentLevel);

int CL_Run(CLinesApp* self, int argc, char** argv);

#endif
