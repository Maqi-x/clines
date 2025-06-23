#ifndef LOC_PARSER_H
#define LOC_PARSER_H

#include <LocSettings.h>

#include <Definitions.h>
#include <stdbool.h>

typedef enum LP_Error {
    LPE_Ok,
    LPE_Unterminated,
    LPE_FileOpenError,
    LPE_AllocFailed,
} LP_Error;

typedef enum LP_State {
    LPS_Default,
    LPS_InString,
    LPS_InMultilineString,
    LPS_InChar,
    LPS_InComment,
    LPS_InMultilineComment,
} LP_State;

typedef struct LocParser {
    LP_State state;

    bool hasCode;
    bool hasComment;
    bool hasPPDirective;

    union {
        const char* lastStringDelim;
        const char* lastMultilineStringDelim;
        const StringDelimPair* lastMultilineCommentDelimPair;
        const char* lastCharDelim;
    };

    bool continueSingleLineComment;
    bool continuePPDirective;
} LocParser;

LP_Error LP_Init(LocParser* self);
LP_Error LP_Destroy(LocParser* self);
LP_Error LP_Refresh(LocParser* self, LocStat* result);

LP_Error LP_ParseLine(LocParser* self, const LocEntry* lang, const char* line, usize len, LocStat* result);
LP_Error LP_ParseCode(LocParser* self, const LocEntry* lang, const char* code, LocStat* result);
LP_Error LP_ParseFile(LocParser* self, const LocEntry* lang, const char* path, LocStat* result);

#endif // LOC_PARSER_H
