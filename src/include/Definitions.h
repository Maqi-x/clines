#ifndef DEFINTIONS_H
#define DEFINTIONS_H

#include <stddef.h>
#include <sys/types.h>

#include <iso646.h>

typedef size_t usize;
typedef ssize_t isize;

typedef struct LineCounter {
    char* toPrint;
    usize lines;
} LineCounter;

#endif // DEFINTIONS_H
