#include <LineCounterList.h>

#include <Definitions.h>
#include <Utils.h>

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
        free(c->meta.path);
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

/// @note Recognizes that dst is not initialized and does not store data.
//        if it does, you need to call @ref LCL_Destroy before this operation
LCL_Error LCL_Copy(LineCounterList* dst, const LineCounterList* src) {
    if (src == dst) return LCLE_Ok;

    LCL_Error err = LCL_AllocBuf(dst, src->cap);
    if (err != LCLE_Ok) return err;

    dst->len = 0;
    for (usize i = 0; i < src->len; ++i) {
        dst->data[i].lines = src->data[i].lines;

        dst->data[i].toPrint = strdup(src->data[i].toPrint);
        if (dst->data[i].toPrint == NULL) return LCLE_AllocFailed; // LCL_Destroy should free alredy allocated strings

        FileMeta* srcMeta = &src->data[i].meta;
        dst->data[i].meta = (FileMeta) {
            .path = strdup(srcMeta->path),
            .mtime = srcMeta->mtime,
            .size = srcMeta->size,
        };

        if (dst->data[i].meta.path == NULL) {
            return LCLE_AllocFailed; // LCL_Destroy should free alredy allocated paths
        }

        dst->len++;
    }

    return LCLE_Ok;
}

/// @note Recognizes that dst is not initialized and does not store data.
//        if it does, you need to call @ref LCL_Destroy before this operation
LCL_Error LCL_Move(LineCounterList* dst, LineCounterList* src) {
    memcpy(dst, src, sizeof(LineCounterList));
    memset(src, 0, sizeof(LineCounterList));
    return LCLE_Ok;
}

LCL_Error LCL_Append(LineCounterList* self, const char* name, usize lines, FileMeta* meta, LocStat locStat, bool hasLocStat) {
    LCL_Error err = LCL_Expand(self);
    if (err != LCLE_Ok) return err;

    char* dname = strdup(name);
    if (dname == NULL) return LCLE_AllocFailed;

    self->data[self->len++] = (LineCounter) {
        .toPrint = dname,
        .lines = lines,
        .meta = (FileMeta) {
            .path = strdup(meta->path),
            .mtime = meta->mtime,
            .size = meta->size,
        },
        .hasLocStat = hasLocStat,
        .locStat = locStat,
    };
    if (self->data[self->len - 1].meta.path == NULL) {
        return LCLE_AllocFailed;
    }
    return LCLE_Ok;
}

LCL_Error LCL_Set(LineCounterList* self, usize index, const char* toPrint, usize lines, FileMeta* meta, LocStat locStat, bool hasLocStat) {
    if (index >= self->len) return LCLE_IndexOutOfRange;

    free(self->data[index].toPrint);
    self->data[index].toPrint = strdup(toPrint);
    if (self->data[index].toPrint == NULL) {
        return LCLE_AllocFailed;
    }

    self->data[index].lines = lines;
    self->data[index].meta = (FileMeta) {
        .path = strdup(meta->path),
        .mtime = meta->mtime,
        .size = meta->size,
    };
    if (self->data[index].meta.path == NULL) {
        return LCLE_AllocFailed;
    }

    self->data[index].hasLocStat = hasLocStat;
    self->data[index].locStat = locStat;
    return LCLE_Ok;
}

LCL_Error LCL_Get(LineCounterList* self, usize index, LineCounter** out) {
    if (index >= self->len) return LCLE_IndexOutOfRange;

    *out = &self->data[index];
    return LCLE_Ok;
}

static int cmpByLines(const void* p1, const void* p2) {
    usize lines1 = ((const LineCounter*)p1)->lines;
    usize lines2 = ((const LineCounter*)p2)->lines;

    if (lines1 < lines2) return -1;
    if (lines1 > lines2) return 1;
    return 0;
}

static int cmpByLinesReversed(const void* p1, const void* p2) {
    return -cmpByLines(p1, p2);
}

static int cmpByName(const void* p1, const void* p2) {
    const LineCounter* a = (const LineCounter*)p1;
    const LineCounter* b = (const LineCounter*)p2;

    const char* name1 = strrchr(a->meta.path, '/');
    const char* name2 = strrchr(b->meta.path, '/');

    name1 = name1 ? name1 + 1 : a->meta.path;
    name2 = name2 ? name2 + 1 : b->meta.path;

    int cmp = strcmp(name1, name2);
    if (cmp != 0) return cmp;

    // tie-break by lines count
    if (a->lines < b->lines) return -1;
    if (a->lines > b->lines) return 1;
    return 0;
}

static int cmpByNameReversed(const void* p1, const void* p2) {
    return -cmpByName(p1, p2);
}

static int cmpByExt(const void* p1, const void* p2) {
    const char* path1 = ((const LineCounter*)p1)->meta.path;
    const char* path2 = ((const LineCounter*)p2)->meta.path;

    const char* ext1 = GetExtension(path1);
    const char* ext2 = GetExtension(path2);

    return strcmp(ext1, ext2);
}

static int cmpByExtReversed(const void* p1, const void* p2) {
    return -cmpByExt(p1, p2);
}

static int cmpByPath(const void* p1, const void* p2) {
    const char* path1 = ((const LineCounter*)p1)->meta.path;
    const char* path2 = ((const LineCounter*)p2)->meta.path;

    return strcmp(path1, path2);
}

static int cmpByPathReversed(const void* p1, const void* p2) {
    return -cmpByPath(p1, p2);
}

LCL_Error LCL_SortBy(LineCounterList* self, CFG_SortMode mode, bool reverse) {
    if (mode == SM_NotSort) return LCLE_Ok;

    typedef int CmpCallback(const void*, const void*);
    CmpCallback* cmp = NULL;

    switch (mode) {
    case SM_Lines:
        cmp = reverse ? cmpByLinesReversed : cmpByLines;
        break;
    case SM_Path:
        cmp = reverse ? cmpByPathReversed : cmpByPath;
        break;
    case SM_Name:
        cmp = reverse ? cmpByNameReversed : cmpByName;
        break;
    case SM_Ext:
        cmp = reverse ? cmpByExtReversed : cmpByExt;
        break;
    default:
        return LCLE_InvalidArgument;
    }

    if (cmp == NULL) return LCLE_InvalidArgument;

    qsort(self->data, self->len, sizeof(LineCounter), cmp);
    return LCLE_Ok;
}

LCL_Error LCL_Print(const LineCounterList* self, FILE* out) {
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
