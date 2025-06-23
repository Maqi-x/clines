#include <INodeSet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline usize INS_ComputeIndex(const INodeSet* self, INode fi) {
    return self->hashFunc(fi) % self->cap;
}

static inline INS_Error INS_GetEntry(INode fi, INS_Entry** out) {
    INS_Entry* entry = malloc(sizeof(INS_Entry));
    if (entry == NULL) return INSE_AllocFailed;

    entry->key = fi;
    entry->next = NULL;
    *out = entry;
    return INSE_Ok;
}

static INS_Error INS_Rehash(INodeSet* self, usize newCap) {
    INS_Entry** newBuckets = calloc(newCap, sizeof(INS_Entry*));
    if (newBuckets == NULL) return INSE_AllocFailed;

    for (usize i = 0; i < self->cap; ++i) {
        INS_Entry* entry = self->buckets[i];
        while (entry) {
            INS_Entry* next = entry->next;

            usize newIndex = self->hashFunc(entry->key) % newCap;

            // insert at the head of the new bucket
            entry->next = newBuckets[newIndex];
            newBuckets[newIndex] = entry;

            entry = next;
        }
    }

    free(self->buckets);
    self->buckets = newBuckets;
    self->cap = newCap;

    return INSE_Ok;
}

static inline void INS_DropEntry(INS_Entry* entry) {
    free(entry);
}

INS_Error INS_InitReserved(INodeSet* self, INS_HashFunc* hash, INS_EqlFunc* eql, usize initCap) {
    self->size = 0;
    self->cap = initCap;
    self->buckets = calloc(initCap, sizeof(INS_Entry*));

    if (self->buckets == NULL) return INSE_AllocFailed;

    self->hashFunc = hash;
    self->eqlFunc = eql;
    return INSE_Ok;
}

INS_Error INS_Init(INodeSet* self, INS_HashFunc* hash, INS_EqlFunc* eql) {
    const usize DEFAULT_INIT_CAP = 16;
    return INS_InitReserved(self, hash, eql, DEFAULT_INIT_CAP);
}

INS_Error INS_DefaultInit(INodeSet* self) {
    return INS_Init(self, IN_Hash, IN_Equals);
}

INS_Error INS_Clear(INodeSet* self) {
    for (usize i = 0; i < self->cap; ++i) {
        INS_Entry* entry = self->buckets[i];
        while (entry != NULL) {
            INS_Entry* next = entry->next;
            INS_DropEntry(entry);
            entry = next;
        }
        self->buckets[i] = NULL;
    }
    self->size = 0;
    return INSE_Ok;
}


INS_Error INS_Destroy(INodeSet* self) {
    INS_Clear(self);
    free(self->buckets);
    self->buckets = NULL;
    self->cap = 0;

    self->hashFunc = NULL;
    self->eqlFunc = NULL;
    return INSE_Ok;
}

INS_Error INS_Copy(INodeSet* dst, const INodeSet* src) {
    INS_Clear(dst);

    INS_Error err = INS_Rehash(dst, src->cap);
    if (err != INSE_Ok) return err;

    for (usize i = 0; i < src->cap; ++i) {
        INS_Entry* entry = src->buckets[i];
        while (entry != NULL) {
            err = INS_Insert(dst, entry->key);
            if (err != INSE_Ok && err != INSE_AlredyExists) return err;
            entry = entry->next;
        }
    }
    return INSE_Ok;
}

INS_Error INS_Move(INodeSet* dst, INodeSet* src) {
    memcpy(dst, src, sizeof(INodeSet));
    memset(src, 0, sizeof(INodeSet));
    return INSE_Ok;
}

bool INS_Contains(const INodeSet* self, INode fi) {
    usize index = INS_ComputeIndex(self, fi);
    INS_Entry* bucket = self->buckets[index];
    for (INS_Entry* entry = bucket; entry != NULL; entry = entry->next) {
        if (self->eqlFunc(entry->key, fi)) return true;
    }

    return false;
}

INS_Error INS_Insert(INodeSet* self, INode fi) {
    if (self->size > self->cap * 0.75) {
        INS_Error err = INS_Rehash(self, self->cap * 2);
        if (err != INSE_Ok) return err;
    }

    // INSE_AlredyExists can be ignored
    if (INS_Contains(self, fi)) return INSE_AlredyExists;

    usize index = INS_ComputeIndex(self, fi);

    INS_Entry* newEntry;
    INS_Error err = INS_GetEntry(fi, &newEntry);
    if (err != INSE_Ok) return err;

    newEntry->next = self->buckets[index];
    self->buckets[index] = newEntry;

    self->size++;
    return INSE_Ok;
}

INS_Error INS_InsertFrom(INodeSet* self, const INodeSet* src) {
    if (self->cap < src->size) {
        INS_Error err = INS_Rehash(self, src->size + 1);
        if (err != INSE_Ok) return err;
    }

    for (usize i = 0; i < src->cap; ++i) {
        INS_Entry* entry = src->buckets[i];
        while (entry != NULL) {
            INS_Error err = INS_Insert(self, entry->key);
            if (err != INSE_Ok) return err;
            entry = entry->next;
        }
    }
    return INSE_Ok;
}
