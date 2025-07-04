#ifndef HELP_PRINTER_H
#define HELP_PRINTER_H

#include <HelpSettings.h>

#include <stdio.h>

typedef enum HP_Error {
    HPE_Ok,
    HPE_InvalidArgument,
} HP_Error;

typedef struct HelpPrinter {
    const char* name;
    const char* desc;

    const HelpCategory* categories;
    usize categoriesCount;

    const char* descAfter;
} HelpPrinter;

void HP_Init(
    HelpPrinter* self, const char* name, const char* desc,
    const HelpCategory* categories, usize categoriesCount, const char* descAfter
);
void HP_Destroy(HelpPrinter* self);

void HP_Print(HelpPrinter* self, FILE* out);
void HP_PrintCategory(HelpPrinter* self, const HelpCategory* category, FILE* out);

#endif // HELP_PRINTER_H
