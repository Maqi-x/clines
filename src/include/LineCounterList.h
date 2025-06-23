#ifndef FILE_COUNTER_LIST_H
#define FILE_COUNTER_LIST_H

#include <Config.h>
#include <Definitions.h>

#include <LocSettings.h>

#include <stdio.h>

typedef enum LCL_Error {
    LCLE_Ok = 0,
    LCLE_AllocFailed,
    LCLE_InvalidArgument,
    LCLE_IndexOutOfRange,
} LCL_Error;

typedef struct LineCounter {
    char* toPrint;
    usize lines;

    FileMeta meta;
    bool hasLocStat;
    LocStat locStat;
} LineCounter;

typedef struct LineCounterList {
    LineCounter* data;
    usize len;
    usize cap;
} LineCounterList;

LCL_Error LCL_AllocBuf(LineCounterList* self, usize newCap);
LCL_Error LCL_Resize(LineCounterList* self, usize newCap);
LCL_Error LCL_Expand(LineCounterList* self);

LCL_Error LCL_Init(LineCounterList* self);
LCL_Error LCL_InitReserved(LineCounterList* self, usize minCap);

LCL_Error LCL_Clear(LineCounterList* self);
LCL_Error LCL_Destroy(LineCounterList* self);
LCL_Error LCL_Shrink(LineCounterList* self);

/// @note Recognizes that dst is not initialized and does not store data.
//        if it does, you need to call @ref LCL_Destroy before this operation
LCL_Error LCL_Copy(LineCounterList* dst, const LineCounterList* src);

/// @note Recognizes that dst is not initialized and does not store data.
//        if it does, you need to call @ref LCL_Destroy before this operation
LCL_Error LCL_Move(LineCounterList* dst, LineCounterList* src);

LCL_Error LCL_Append(LineCounterList* self, const char* name, usize lines, FileMeta* meta, LocStat locStat, bool hasLocStat);
LCL_Error LCL_Set(LineCounterList* self, usize index, const char* name, usize lines, FileMeta* meta, LocStat locStat, bool hasLocStat);
LCL_Error LCL_Get(LineCounterList* self, usize index, LineCounter** out);

LCL_Error LCL_SortBy(LineCounterList* self, CFG_SortMode mode, bool reverse);
LCL_Error LCL_Print(const LineCounterList* self, FILE* out);

#endif // FILE_COUNTER_LIST_H
