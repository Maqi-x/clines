#include <HelpSettings.h>

#include <bits/posix1_lim.h>
#include <stdarg.h>
#include <stdbool.h>

/**
[ Category 1 ]
  --flag1:              Does something
  --flag2:              Does something else
  --include [paths...]: Includes given paths
 */

#define SEPARATOR ((HelpItem) { .name = "", .desc = "" })
#define FINISH ((HelpItem) { NULL })

static const HelpCategory help[] = {
    (HelpCategory) {
        .name = "General",
        .desc = "General options",
        .items = (const HelpItem[]) {
            (HelpItem) {
                .name = "--help|-h",
                .desc = "Prints this help message and exits",
            },
            (HelpItem) {
                .name = "--version",
                .desc = "Prints the version of the program and exits",
            },
            (HelpItem) {
                .name = "--repo",
                .desc = "Prints the repository information of the program and exits",
            },

            SEPARATOR,

            (HelpItem) {
                .name = "--print|-p|--verbose|-v",
                .desc = "Prints all files to stdout",
            },
            (HelpItem) {
                .name = "--no-print|--no-verbose",
                .desc = "Does not print files to stdout",
            },
            (HelpItem) {
                .name = "--recursive|-r",
                .desc = "Recursively prints all files in subdirectories (default)",
            },
            (HelpItem) {
                .name = "--no-recursive|-n",
                .desc = "Does not recursively print files in subdirectories",
            },
            (HelpItem) {
                .name = "--loc|-l",
                .desc = "Enables line-of-code (loc) mode (prints number of code/comments/pp/blank lines in supported languages files)",
            },
            (HelpItem) {
                .name = "--no-loc",
                .desc = "Disables line-of-code (loc) mode (default)",
            },
            (HelpItem) {
                .name = "--show-hidden",
                .desc = "Shows hidden files (default)",
            },
            (HelpItem) {
                .name = "--no-show-hidden",
                .desc = "Does not show hidden files",
            },

            SEPARATOR,

            (HelpItem) {
                .name = "--max-depth={depth}",
                .desc = "Sets maximum depth of recursion to {depth} (default: unlimited)",
            },
            (HelpItem) {
                .name = "--debug",
                .desc = "Enables debug mode",
            },
            (HelpItem) {
                .name = "--no-debug",
                .desc = "Disables debug mode (default)",
            },

            FINISH,
        },
    },
    (HelpCategory) {
        .name = "Include/Exclude",
        .desc = "Options for including/excluding files and directories",
        .items = (const HelpItem[]) {
            (HelpItem) {
                .name = "--include|--include-path|-i [paths...]",
                .desc = "Includes given paths",
            },
            (HelpItem) {
                .name = "--exclude|--exclude-path|-x [paths...]",
                .desc = "Excludes given paths",
            },
            (HelpItem) {
                .name = "--include-ext|-e [extensions...]",
                .desc = "Includes files with given extensions",
            },
            (HelpItem) {
                .name = "--exclude-ext [extensions...]",
                .desc = "Excludes files with given extensions",
            },
            (HelpItem) {
                .name = "--include-regex [regexes...]",
                .desc = "Includes files matching given regex",
            },
            (HelpItem) {
                .name = "--exclude-regex|-g [regexes...]",
                .desc = "Excludes files matching given regex",
            },

            FINISH,
        }
    },
    (HelpCategory) {
        .name = "Sorting",
        .desc = "Options for sorting output",
        .items = (const HelpItem[]) {
            (HelpItem) {
                .name = "--sort-by-lines",
                .desc = "Sorts output by number of lines (descending)",
            },
            (HelpItem) {
                .name = "--sort-by-ext",
                .desc = "Sorts output by file extension (alphabetically, descending)",
            },
            (HelpItem) {
                .name = "--sort-by-name",
                .desc = "Sorts output by name (alphabetically, descending)",
            },
            (HelpItem) {
                .name = "--sort-by-path",
                .desc = "Sorts output by file full path (alphabetically, descending)",
            },
            (HelpItem) {
                .name = "--sort-by-mtime",
                .desc = "Sorts output by file modification time (descending)",
            },
            (HelpItem) {
                .name = "--sort-by-size",
                .desc = "Sorts output by file size (descending)",
            },

            (HelpItem) {
                .name = "--reversed-sort-by-lines",
                .desc = "Sorts output by number of lines (ascending)",
            },
            (HelpItem) {
                .name = "--reversed-sort-by-ext",
                .desc = "Sorts output by file extension (alphabetically, ascending)",
            },
            (HelpItem) {
                .name = "--reversed-sort-by-name",
                .desc = "Sorts output by name (alphabetically, ascending)",
            },
            (HelpItem) {
                .name = "--reversed-sort-by-path",
                .desc = "Sorts output by file full path (alphabetically, ascending)",
            },
            (HelpItem) {
                .name = "--reversed-sort-by-mtime",
                .desc = "Sorts output by file modification time (ascending)",
            },
            (HelpItem) {
                .name = "--reversed-sort-by-size",
                .desc = "Sorts output by file size (ascending)",
            },

            SEPARATOR,

            (HelpItem) {
                .name = "--sort=lines|ext|name|path|mtime|size",
                .desc = "Same as --sort-by-...",
            },
            (HelpItem) {
                .name = "--reverse|--reverse-sort",
                .desc = "Reverses the order of the output",
            },

            SEPARATOR,

            (HelpItem) {
                .name = "--top={n}",
                .desc = "Shows only the top {n} files (TODO!)",
            },

            FINISH,
        },
    },
};

const HelpCategory* GetHelp() {
    return help;
}

usize GetHelpCategoryCount() {
    return sizeof(help) / sizeof(HelpCategory);
}
