#ifndef HELP_SETTINGS_H
#define HELP_SETTINGS_H

#include <Definitions.h>

#include <stdbool.h>

typedef struct HelpItem {
    const char* name;
    const char* desc;
} HelpItem;

typedef struct HelpCategory {
    const char* name;
    const char* desc;

    const HelpItem* items;
} HelpCategory;

const HelpCategory* GetHelp();
usize GetHelpCategoryCount();

#endif // HELP_SETTINGS_H
