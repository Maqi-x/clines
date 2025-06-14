#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define BOLD "\033[1m"
#define RESET "\033[0m"
#define BLACK "\033[30m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"

#define BRIGHT_BLACK "\033[90m"
#define BRIGHT_RED "\033[91m"
#define BRIGHT_GREEN "\033[92m"
#define BRIGHT_YELLOW "\033[93m"
#define BRIGHT_BLUE "\033[94m"
#define BRIGHT_MAGENTA "\033[95m"
#define BRIGHT_CYAN "\033[96m"
#define BRIGHT_WHITE "\033[97m"

#define DIM "\033[2m"
#define ITALIC "\033[3m"
#define UNDERLINE "\033[4m"
#define BLINK "\033[5m"
#define INVERT "\033[7m"
#define STRIKE "\033[9m"

#define COLOR_DEBUGLOG1 BOLD MAGENTA
#define COLOR_DEBUGLOG2 BRIGHT_MAGENTA

#define COLOR_ERROR1 BOLD RED
#define COLOR_ERROR2 BRIGHT_RED

#define COLOR_NOTE1 BOLD CYAN
#define COLOR_NOTE2 BRIGHT_CYAN

#define COLOR_INFO1 BOLD GREEN
#define COLOR_INFO2 BRIGHT_GREEN

#define COLOR_WARN1 BOLD YELLOW
#define COLOR_WARN2 BRIGHT_YELLOW

#define COLOR_TIP1 BOLD BLUE
#define COLOR_TIP2 BRIGHT_BLUE

#define COLOR_SUCCESS1 BOLD GREEN
#define COLOR_SUCCESS2 BRIGHT_GREEN

#define DEF_MSG_PRINTF_WRAPPER(name, prefix, color, isErr)                                                             \
    static int MSG_Show##name(const char* fmt, ...) __attribute__((format(printf, 1, 2)));                             \
    static inline int MSG_Show##name(const char* fmt, ...) {                                                           \
        va_list args;                                                                                                  \
        va_start(args, fmt);                                                                                           \
        fputs(COLOR_##color##1 "[ " prefix " ]: " COLOR_##color##2, isErr ? stderr : stdout);                          \
        int res = vfprintf(isErr ? stderr : stdout, fmt, args);                                                        \
        fputs("\033[0m\n", isErr ? stderr : stdout);                                                                   \
        va_end(args);                                                                                                  \
        return res + strlen(COLOR_##color##1 "[ " prefix " ]:" COLOR_##color##2);                                      \
    }

DEF_MSG_PRINTF_WRAPPER(Error, "ERROR", ERROR, true);
DEF_MSG_PRINTF_WRAPPER(Note, "NOTE", NOTE, false);
DEF_MSG_PRINTF_WRAPPER(Info, "INFO", INFO, false);
DEF_MSG_PRINTF_WRAPPER(Warn, "WARNING", WARN, true);
DEF_MSG_PRINTF_WRAPPER(Tip, "TIP", TIP, false);
DEF_MSG_PRINTF_WRAPPER(Success, "SUCCESS", SUCCESS, false);

extern bool debug;

static inline int MSG_ShowDebugLog(const char* fmt, ...) {
    if (!debug) return 0;

    va_list args;
    va_start(args, fmt);

    puts(COLOR_DEBUGLOG1 "[ DEBUG ]:" COLOR_DEBUGLOG2);
    int res = vprintf(fmt, args);

    va_end(args);
    return res + strlen(COLOR_DEBUGLOG1 "[ DEBUG ]:" COLOR_DEBUGLOG2);
}
