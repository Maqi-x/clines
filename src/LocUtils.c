#include <LocUtils.h>
#include <LocSettings.h>

#include <Utils.h>

#include <string.h>
#include <stdio.h>
#include <stdbool.h>

bool GetLocLangFor(const char* filename, const LocEntry** out) {
    if (filename == NULL || out == NULL) {
        return false;
    }

    const char* fileext = GetExtension(filename);

    const LocEntry* locEntries = GetLocEntries();
    for (usize i = 0; i < GetLocEntriesCount(); ++i) {
        const LocEntry* entry = &locEntries[i];

        if (fileext != NULL && entry->extensions != NULL) {
            for (const char** ext = entry->extensions; *ext != NULL; ++ext) {
                if (StrEql(fileext, *ext)) {
                    *out = entry;
                    return true;
                }
            }
        }

        if (entry->names != NULL) {
            for (const char** name = entry->names; *name != NULL; ++name) {
                if (StrEql(filename, *name)) {
                    *out = entry;
                    return true;
                }
            }
        }
    }

    *out = NULL;
    return false;
}
