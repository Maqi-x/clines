#ifndef INODE_SET_H
#define INODE_SET_H

#include <Definitions.h>

#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>

typedef struct INode {
    dev_t dev;
    ino_t ino;
} INode;

static inline usize IN_Hash(INode self) {
    uint64_t a = (uint64_t)self.dev;
    uint64_t b = (uint64_t)self.ino;

    uint64_t hash = a ^ (b + 0x9e3779b97f4a7c15 + (a << 6) + (a >> 2));
    return (usize)hash;
}

static inline bool IN_Equals(INode a, INode b) {
    return a.dev == b.dev && a.ino == b.ino;
}

typedef enum INS_Error {
    INSE_Ok,
    INSE_AllocFailed,
    INSE_AlredyExists,
    INSE_Todo,
} INS_Error;

typedef struct INS_Entry {
    INode key;
    struct INS_Entry* next;
} INS_Entry;

typedef usize INS_HashFunc(INode);
typedef bool INS_EqlFunc(INode, INode);

typedef struct INodeSet {
    usize size;
    usize cap;
    INS_Entry** buckets;

    INS_HashFunc* hashFunc;
    INS_EqlFunc* eqlFunc;
} INodeSet;

INS_Error INS_InitReserved(INodeSet* self, INS_HashFunc* hash, INS_EqlFunc* eql, usize initCap);
INS_Error INS_Init(INodeSet* self, INS_HashFunc* hash, INS_EqlFunc* eql);
INS_Error INS_DefaultInit(INodeSet* self);

INS_Error INS_Clear(INodeSet* self);
INS_Error INS_Destroy(INodeSet* self);
INS_Error INS_Copy(INodeSet* dst, const INodeSet* src);
INS_Error INS_Move(INodeSet* dst, INodeSet* src);

bool INS_Contains(const INodeSet* self, INode fi);
INS_Error INS_Insert(INodeSet* self, INode fi);
INS_Error INS_InsertFrom(INodeSet* self, const INodeSet* src);

#endif // INODE_SET_H
