#include <LocParser.h>

#include <LocSettings.h>
#include <LocUtils.h>

#include <Definitions.h>
#include <Utils.h>

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

LP_Error LP_Init(LocParser* self) {
    self->state = LPS_Default;
    self->continueSingleLineComment = false;

    self->hasCode = false;
    self->hasComment = false;
    self->hasPPDirective = false;
    return LPE_Ok;
}

LP_Error LP_Destroy(LocParser* self) {
    self->state = (LP_State) 0;
    return LPE_Ok;
}

LP_Error LP_Refresh(LocParser* self, LocStat* result) {
    if (self->state == LPS_Default || self->state == LPS_InMultilineComment || self->state == LPS_InMultilineString) {
        if (self->hasCode) result->codeLines++;
        if (self->hasComment) result->commentLines++;
        if (self->hasPPDirective) result->preprocessorLines++;

        if (!self->hasCode && !self->hasComment && !self->hasPPDirective) {
            result->emptyLines++;
        }
    }

    result->totalLines++;

    self->hasCode = false;
    self->hasComment = false;
    self->hasPPDirective = false;
    return LPE_Ok;
}

static inline bool IsAllSpace(const char* line, usize len) {
    // iteration from the end, at the beginning of the line there are usually indentations
    for (isize i = len-1; i >= 0; --i) {
        if (!isspace(line[i])) return false;
    }
    return true;
}

LP_Error LP_ParseLine(LocParser* self, const LocEntry* lang, const char* line, usize len, LocStat* result) {
    if (IsAllSpace(line, len)) {
        self->hasCode = false;
        self->hasComment = false;
        self->hasPPDirective = false;
        LP_Refresh(self, result);
        return LPE_Ok;
    }

    if (self->continueSingleLineComment) {
        self->hasComment = true;
        LP_Refresh(self, result);

        if (len > 0 && line[len - 1] == '\\') {
            self->continueSingleLineComment = true;
        } else {
            self->continueSingleLineComment = false;
        }

        return LPE_Ok;
    }

    if (self->continuePPDirective) {
        self->hasPPDirective = true;
        LP_Refresh(self, result);

        if (len > 0 && line[len - 1] == '\\') {
            self->continuePPDirective = true;
        } else {
            self->continuePPDirective = false;
        }

        return LPE_Ok;
    }

    usize i = 0;
    while (i < len && isspace(line[i])) ++i;

    for (; i < len; ++i) {
        const char* ptr = &line[i];
        char current = line[i];

        const char* s;
        const StringDelimPair* p;

        switch (self->state) {
        case LPS_Default:
            if ((s = StartsWithAny(ptr, lang->stringDelims))) {
                self->state = LPS_InString;
                self->lastStringDelim = s;
                self->hasCode = true;
                i += strlen(s) - 1;
                break;
            }

            if ((s = StartsWithAny(ptr, lang->multilineStringDelims))) {
                self->state = LPS_InMultilineString;
                self->lastMultilineStringDelim = s;
                self->hasCode = true;
                i += strlen(s) - 1;
                break;
            }

            if ((s = StartsWithAny(ptr, lang->charDelims))) {
                self->state = LPS_InChar;
                self->lastCharDelim = s;
                self->hasCode = true;
                i += strlen(s) - 1;
                break;
            }

            if ((s = StartsWithAny(ptr, lang->commentStarts))) {
                if (lang->allowCommentContinues) {
                    bool hasBackslash = len > 0 && line[len - 1] == '\\';
                    self->continueSingleLineComment = hasBackslash;
                }
                self->hasComment = true;
                goto end;
            }

            if ((p = StartsWithAnyPair(ptr, lang->multilineCommentDelimPairs))) {
                self->state = LPS_InMultilineComment;
                self->lastMultilineCommentDelimPair = p;
                self->hasComment = true;
                i += strlen(p->start) - 1;
                break;
            }

            if ((s = StartsWithAny(ptr, lang->ppDirectiveStarts))) {
                if (lang->allowPPDirectiveContinues) {
                    bool hasBackslash = len > 0 && line[len - 1] == '\\';
                    self->continuePPDirective = hasBackslash;
                }
                self->hasPPDirective = true;
                goto fixAndEnd;
            }

            if (!isspace(current)) {
                self->hasCode = true;
            }

            break;

        case LPS_InString:
            if (current == '\\' && i + 1 < len) {
                i++; // skip escaped character
                break;
            }
            if (HasPrefix(ptr, self->lastStringDelim)) {
                self->state = LPS_Default;
                i += strlen(self->lastStringDelim) - 1;
            }
            break;

        case LPS_InMultilineString:
            self->hasCode = true;
            if (current == '\\' && i + 1 < len) {
                i++; // skip escaped character
                break;
            }
            if (HasPrefix(ptr, self->lastMultilineStringDelim)) {
                self->state = LPS_Default;
                i += strlen(self->lastMultilineStringDelim) - 1;
            }
            break;

        case LPS_InChar:
            if (current == '\\' && i + 1 < len) {
                i++; // skip escaped character
                break;
            }
            if (HasPrefix(ptr, self->lastCharDelim)) {
                self->state = LPS_Default;
                i += strlen(self->lastCharDelim) - 1;
            }
            break;

        case LPS_InComment:
            self->hasComment = true;
            goto end;

        case LPS_InMultilineComment:
            self->hasComment = true;
            if (HasPrefix(ptr, self->lastMultilineCommentDelimPair->end)) {
                self->state = LPS_Default;
                i += strlen(self->lastMultilineCommentDelimPair->end) - 1;
            }
            break;
        }
    }

fixAndEnd:
    if (self->state != LPS_InMultilineComment && self->state != LPS_InMultilineString) {
        self->state = LPS_Default;
    }
end:
    LP_Refresh(self, result);
    return LPE_Ok;
}

LP_Error LP_ParseCode(LocParser* self, const LocEntry* lang, const char* code, LocStat* result) {
    const char* lineStart = code;
    const char* lineEnd = code;

    while (*lineEnd) {
        if (*lineEnd == '\n') {
            LocStat res = {0};
            LP_Error err = LP_ParseLine(self, lang, lineStart, lineEnd - lineStart, &res);
            if (err != LPE_Ok) return err;

            MergeResults(result, &res);
            lineStart = lineEnd + 1;
        }
        lineEnd++;
    }

    // process last line if it doesn't end with newline
    if (lineEnd > lineStart) {
        LocStat res = {0};
        LP_Error err = LP_ParseLine(self, lang, lineStart, lineEnd - lineStart, &res);
        if (err != LPE_Ok) return err;

        MergeResults(result, &res);
    }

    if (self->state != LPS_Default) {
        return LPE_Unterminated;
    }

    return LPE_Ok;
}

#ifndef LINE_BUF_SIZE
#    define LINE_BUF_SIZE 4096
#endif

LP_Error LP_ParseFile(LocParser* self, const LocEntry* lang, const char* path, LocStat* result) {
    FILE* fp = fopen(path, "r");
    if (fp == NULL) {
        return LPE_FileOpenError;
    }

    char buf[LINE_BUF_SIZE];
    char* line = buf;
    usize totalLen = 0;
    usize cap = sizeof(buf);
    bool usingBuf = true;

    while (!feof(fp)) {
        line = buf;
        totalLen = 0;
        cap = sizeof(buf);
        usingBuf = true;

        while (fgets(line + totalLen, cap - totalLen, fp)) {
            usize len = strlen(line + totalLen);
            totalLen += len;

            if (line[totalLen - 1] == '\n' || feof(fp)) {
                break;
            }

            if (usingBuf) {
                cap = totalLen * 2;
                char* tmp = malloc(cap);
                if (tmp == NULL) {
                    fclose(fp);
                    return LPE_AllocFailed;
                }

                memcpy(tmp, buf, totalLen);
                line = tmp;
                usingBuf = false;
            } else {
                cap *= 2;
                char* tmp = realloc(line, cap);
                if (tmp == NULL) {
                    free(line);
                    fclose(fp);
                    return LPE_AllocFailed;
                }
                line = tmp;
            }
        }

        if (totalLen == 0 && feof(fp)) {
            break;
        }

        // Strip newline if present
        if (totalLen > 0 && line[totalLen - 1] == '\n') {
            totalLen--;
        }

        LocStat res = {0};
        LP_ParseLine(self, lang, line, totalLen, &res);
        MergeResults(result, &res);

        if (!usingBuf) {
            free(line);
        }
    }

    fclose(fp);
    return LPE_Ok;
}
