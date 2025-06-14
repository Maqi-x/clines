#include <LineCounterList.h>

#include <Definitions.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LCL_Error LCL_AllocBuf(LineCounterList* self, usize newCap) {
    LineCounter* newData = malloc(newCap * sizeof(LineCounter));
    if (newData == NULL) return LCLE_AllocFailed;

    free(self->data);
    self->data = newData;
    self->cap = newCap;
    return LCLE_Ok;
}

LCL_Error LCL_Resize(LineCounterList* self, usize newCap) {
    LineCounter* newData = malloc(newCap * sizeof(LineCounter));
    if (newData == NULL) return LCLE_AllocFailed;

    memcpy(newData, self->data, self->len * sizeof(LineCounter));

    free(self->data);
    self->data = newData;
    self->cap = newCap;
    return LCLE_Ok;
}

LCL_Error LCL_Expand(LineCounterList* self) {
    if (self->len == self->cap) {
        return LCL_Resize(self, self->len > 0 ? self->len * 2 : 8);
    }
    return LCLE_Ok;
}

LCL_Error LCL_Init(LineCounterList* self) {
    self->len = 0;
    self->cap = 0;
    self->data = NULL;
    return LCLE_Ok;
}

LCL_Error LCL_InitReserved(LineCounterList* self, usize minCap) {
    LCL_Error err;

    err = LCL_Init(self);
    if (err != LCLE_Ok) return err;

    err = LCL_Resize(self, minCap);
    if (err != LCLE_Ok) return err;

    return LCLE_Ok;
}

LCL_Error LCL_Clear(LineCounterList* self) {
    for (LineCounter* c = self->data; c < self->data + self->len; ++c) {
        free(c->toPrint);
    }

    free(self->data);
    self->data = NULL;
    self->len = 0;
    self->cap = 0;

    return LCLE_Ok;
}

LCL_Error LCL_Destroy(LineCounterList* self) {
    return LCL_Clear(self);
}

LCL_Error LCL_Shrink(LineCounterList* self) {
    return LCL_Resize(self, self->len);
}

LCL_Error LCL_Copy(LineCounterList* dst, LineCounterList* src) {
    if (src == dst) return LCLE_Ok;

    LCL_Error err = LCL_AllocBuf(dst, src->cap);
    if (err != LCLE_Ok) return err;

    dst->len = 0;
    for (usize i = 0; i < src->len; ++i) {
        dst->data[i].lines = src->data[i].lines;

        dst->data[i].toPrint = strdup(src->data[i].toPrint);
        if (dst->data[i].toPrint == NULL) return LCLE_AllocFailed; // LCL_Destroy should free alredy allocated strings

        dst->len++;
    }

    return LCLE_Ok;
}

LCL_Error LCL_Move(LineCounterList* dst, LineCounterList* src) {
    memcpy(dst, src, sizeof(LineCounterList));
    memset(src, 0, sizeof(LineCounterList));
    return LCLE_Ok;
}

LCL_Error LCL_Append(LineCounterList* self, const char* name, usize lines) {
    LCL_Expand(self);

    char* dname = strdup(name);
    if (dname == NULL) return LCLE_AllocFailed;

    self->data[self->len++] = (LineCounter){
        .toPrint = dname,
        .lines = lines,
    };
    return LCLE_Ok;
}

LCL_Error LCL_Set(LineCounterList* self, usize index, const char* name, usize lines) {
    if (index >= self->len) return LCLE_IndexOutOfRange;

    free(self->data[index].toPrint);
    self->data[index].toPrint = strdup(name);
    if (self->data[index].toPrint == NULL) return LCLE_AllocFailed;

    self->data[index].lines = lines;
    return LCLE_Ok;
}

LCL_Error LCL_Get(LineCounterList* self, usize index, LineCounter** out) {
    if (index >= self->len) return LCLE_IndexOutOfRange;

    *out = &self->data[index];
    return LCLE_Ok;
}

LCL_Error LCL_Print(LineCounterList* self, FILE* out) {
    if (self->len == 0) {
        fputc('[', out);
        fputc(']', out);
        return LCLE_Ok;
    }

    fputc('[', out);

    fprintf(out, "%s: %zu", self->data[0].toPrint, self->data[0].lines);
    for (LineCounter* c = self->data + 1; c < self->data + self->len; ++c) {
        fprintf(out, ", %s: %zu", c->toPrint, c->lines);
    }

    fputc(']', out);
    return LCLE_Ok;
}
