#include <StringList.h>

#include <Definitions.h>
#include <Utils.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SL_Error SL_Init(StringList* self) {
    self->data = NULL;
    self->len = 0;
    self->cap = 0;
    return SLE_Ok;
}

SL_Error SL_InitReserved(StringList* self, usize minCap) {
    SL_Error err;

    err = SL_Init(self);
    if (err != SLE_Ok) return err;

    err = SL_Resize(self, minCap);
    if (err != SLE_Ok) return err;

    return SLE_Ok;
}

SL_Error SL_Resize(StringList* self, usize newCap) {
    char** newData = malloc(newCap * sizeof(char*));
    if (newData == NULL) return SLE_AllocFailed;

    memcpy(newData, self->data, self->len * sizeof(char*));

    free(self->data);
    self->data = newData;
    self->cap = newCap;
    return SLE_Ok;
}

SL_Error SL_Expand(StringList* self) {
    if (self->len == self->cap) {
        usize newCap = self->cap > 0 ? self->cap * 2 : 8;
        return SL_Resize(self, newCap);
    }
    return SLE_Ok;
}

SL_Error SL_Destroy(StringList* self) {
    return SL_Clear(self);
}

SL_Error SL_Clear(StringList* self) {
    for (usize i = 0; i < self->len; ++i) {
        free(self->data[i]);
    }
    free(self->data);

    self->len = 0;
    self->cap = 0;
    return SLE_Ok;
}

SL_Error SL_Shrink(StringList* self) {
    return SL_Resize(self, self->len);
}

/// @note Recognizes that dst is not initialized and does not store data.
//        if it does, you need to call @ref SL_Destroy before this operation
SL_Error SL_Copy(StringList* dst, StringList* src) {
    if (src == dst) return SLE_Ok;

    SL_Error err = SL_Resize(dst, src->cap);
    if (err != SLE_Ok) return err;

    dst->len = 0;
    for (usize i = 0; i < src->len; ++i) {
        dst->data[i] = strdup(src->data[i]);
        if (dst->data[i] == NULL) return SLE_AllocFailed;

        dst->len++;
    }

    return SLE_Ok;
}

/// @note Recognizes that dst is not initialized and does not store data.
//        if it does, you need to call @ref SL_Destroy before this operation
SL_Error SL_Move(StringList* dst, StringList* src) {
    memcpy(dst, src, sizeof(StringList));
    memset(src, 0, sizeof(StringList));
    return SLE_Ok;
}

SL_Error SL_Append(StringList* self, const char* str) {
    SL_Error err = SL_Expand(self);
    if (err != SLE_Ok) return err;

    self->data[self->len++] = strdup(str);
    if (self->data[self->len - 1] == NULL) {
        return SLE_AllocFailed;
    }

    return SLE_Ok;
}

SL_Error SL_Pop(StringList* self, char** out) {
    if (self->len <= 0) {
        return SLE_IndexOutOfRange;
    }

    *out = strdup(self->data[self->len - 1]);
    if (*out == NULL) {
        return SLE_AllocFailed;
    }

    free(self->data[self->len - 1]);
    self->len--;

    return SLE_Ok;
}

SL_Error SL_Set(StringList* self, usize index, const char* str) {
    if (index >= self->len) {
        return SLE_IndexOutOfRange;
    }

    free(self->data[index]);
    self->data[index] = strdup(str);
    if (self->data[index] == NULL) {
        return SLE_AllocFailed;
    }

    return SLE_Ok;
}

SL_Error SL_Get(StringList* self, usize index, char** out) {
    if (index >= self->len) {
        return SLE_IndexOutOfRange;
    }
    *out = self->data[index];
    return SLE_Ok;
}

bool SL_EqTo(StringList* lhs, const char* const* rhs) {
    for (usize i = 0; rhs[i] != NULL && i < lhs->len; ++i) {
        if (!StrEql(lhs->data[i], rhs[i])) {
            return false;
        }
    }

    return true;
}

bool SL_IneqTo(StringList* lhs, const char* const* rhs) {
    for (usize i = 0; rhs[i] != NULL && i < lhs->len; ++i) {
        if (StrEql(lhs->data[i], rhs[i])) {
            return false;
        }
    }

    return true;
}

bool SL_Eq(StringList* lhs, StringList* rhs) {
    if (lhs->len != rhs->len) return false;

    const usize len = lhs->len;
    for (usize i = 0; i < len; ++i) {
        if (!StrEql(lhs->data[i], rhs->data[i])) {
            return false;
        }
    }

    return true;
}

bool SL_Ineq(StringList* lhs, StringList* rhs) {
    if (lhs->len != rhs->len) return true;

    const usize len = lhs->len;
    for (usize i = 0; i < len; ++i) {
        if (StrEql(lhs->data[i], rhs->data[i])) {
            return false;
        }
    }

    return true;
}

SL_Error SL_Print(StringList* self, FILE* out) {
    if (self->len == 0) {
        fputc('[', out);
        fputc(']', out);
        return SLE_Ok;
    }

    fputc('[', out);

    fprintf(out, "%s", self->data[0]);
    for (char** str = self->data + 1; str < self->data + self->len; ++str) {
        fprintf(out, ", %s", *str);
    }

    fputc(']', out);
    return SLE_Ok;
}
