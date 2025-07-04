#include <HelpPrinter.h>

#include <HelpSettings.h>

#include <Log.h>

#include <stdio.h>
#include <string.h>

void HP_Init(
    HelpPrinter* self, const char* name, const char* desc,
    const HelpCategory* categories, usize categoriesCount, const char* descAfter
) {
    self->name = name;
    self->desc = desc;

    self->categories = categories;
    self->categoriesCount = categoriesCount;
    self->descAfter = descAfter;
}

void HP_Destroy(HelpPrinter* self) {
    /* nothing yet */
}

void HP_Print(HelpPrinter* self, FILE* out) {
    const char* const s = "---------------------";
    fprintf(out, BOLD "%s %s %s\n" RESET, s, self->name, s);
    fputc(' ', out);
    fputs(self->desc, out);
    fputc('\n', out);

    for (const HelpCategory* cat = self->categories; cat < self->categories + self->categoriesCount; ++cat) {
        HP_PrintCategory(self, cat, out);
        fputc('\n', out);
    }

    fputc(' ', out);
    fputs(self->descAfter, out);
    fputc('\n', out);
}

static const usize OFFSET = 2;

void HP_PrintCategory(HelpPrinter* self, const HelpCategory* category, FILE* out) {
    fprintf(out, BOLD "[ %s ]\n" RESET, category->name);
    fputc(' ', out);
    fputs(category->desc, out);
    fputc('\n', out);

    usize maxNameLen = 0;
    usize count = 0;
    for (const HelpItem* item = category->items; item->name != NULL; ++item) {
        usize nameLen = strlen(item->name);
        if (nameLen > maxNameLen) {
            maxNameLen = nameLen;
        }

        ++count;
    }

    for (const HelpItem* item = category->items; item < category->items + count; ++item) {
        usize nameLen = strlen(item->name);
        usize pad = maxNameLen + OFFSET - nameLen;

        fprintf(out, "  %s", item->name);

        for (usize i = 0; i < pad; ++i) fputc(' ', out);
        fprintf(out, "%s\n", item->desc);
    }
}
