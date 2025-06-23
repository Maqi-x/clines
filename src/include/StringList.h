#ifndef STRING_LIST_H
#define STRING_LIST_H

#include <Definitions.h>

#include <stdbool.h>
#include <stdio.h>

typedef enum SL_Error {
    SLE_Ok = 0,
    SLE_AllocFailed,
    SLE_IndexOutOfRange,
} SL_Error;

typedef struct StringList {
    char** data; ///< Pointer to heap allocated array (NULL if cap is equal to 0)
    usize len;   ///< Current length of the list
    usize cap;   ///< Current capacity of the list
} StringList;

SL_Error SL_Resize(StringList* self, usize newCap);
SL_Error SL_Expand(StringList* self);

SL_Error SL_Init(StringList* self);
SL_Error SL_InitReserved(StringList* self, usize minCap);

SL_Error SL_Clear(StringList* self);
SL_Error SL_Destroy(StringList* self);
SL_Error SL_Shrink(StringList* self);

/// @note Recognizes that dst is not initialized and does not store data.
//        if it does, you need to call @ref SL_Destroy before this operation
SL_Error SL_Copy(StringList* dst, StringList* src);

/// @note Recognizes that dst is not initialized and does not store data.
//        if it does, you need to call @ref SL_Destroy before this operation
SL_Error SL_Move(StringList* dst, StringList* src);

SL_Error SL_Append(StringList* self, const char* str);
SL_Error SL_Pop(StringList* self, char** out);
SL_Error SL_Set(StringList* self, usize index, const char* str);
SL_Error SL_Get(StringList* self, usize index, char** out);

bool SL_EqTo(StringList* lhs, const char* const* rhs);
bool SL_IneqTo(StringList* lhs, const char* const* rhs);

bool SL_Eq(StringList* lhs, StringList* rhs);
bool SL_Ineq(StringList* lhs, StringList* rhs);

SL_Error SL_Print(StringList* self, FILE* out);

#endif // STRING_LIST_H
