#ifndef DEFINTIONS_H
#define DEFINTIONS_H

#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>

typedef size_t usize;
typedef ssize_t isize;

typedef struct FileMeta {
    char* path; /// malloc'ed
    off_t size;
    time_t mtime;
} FileMeta;

typedef struct LineCounter {
    char* toPrint;
    usize lines;

    FileMeta meta;
} LineCounter;

#endif // DEFINTIONS_H
