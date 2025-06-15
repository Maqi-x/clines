#include <CLines/App.h>
#include <Log.h>

#include <Definitions.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Sets error details message.
 * @param msg The error message to set
 * @return CLE_Ok on success, CLE_AllocFailed if allocation fails
 */
CL_Error CL_SetErrorDetails(CLinesApp* self, const char* msg) {
    free(self->errorDetails);

    self->errorDetails = strdup(msg);
    if (msg == NULL) return CLE_AllocFailed;

    return CLE_Ok;
}

/**
 * Sets formatted error details message.
 * @param fmt The format string
 * @param ... Variable arguments for the format string
 * @return CLE_Ok on success, CLE_AllocFailed if allocation fails
 */
CL_Error CL_SetErrorDetailsf(CLinesApp* self, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    free(self->errorDetails);

    int res = vasprintf(&self->errorDetails, fmt, args);
    if (res == -1) return CLE_AllocFailed;

    va_end(args);
    return CLE_Ok;
}

CL_Error CL_MapAndExceptCFG(CLinesApp* self, CFG_Error cerr) {
    if (cerr == CFGE_Ok) return CLE_Ok;

    switch (cerr) {
    case CFGE_UnexpectedArgument:
        MSG_ShowError("Unexpected argument: %s.", self->cfg.errorDetails);
        break;
    case CFGE_RedeclaredFlag:
        MSG_ShowError("Flag redeclared.");
        break;
    case CFGE_InvalidInputNumber:
        MSG_ShowError("Invalid input number.");
        break;
    case CFGE_InvalidSortMode:
        MSG_ShowError("Invalid sort mode: %s.", self->cfg.errorDetails);
        break;

    case CFGE_AllocFailed:
    case CFGE_ListError:
    case CFGE_Ok: // average -Wswitch experimence
        MSG_ShowError("Internal error.\n");
        return CLE_InternalError;
    }

    return CLE_ConfigError;
}

CL_Error CL_MapAndExceptLCL(CLinesApp* self, LCL_Error lcerr) {
    switch (lcerr) {
    case LCLE_Ok:
        return CLE_Ok;
    case LCLE_IndexOutOfRange:
        MSG_ShowError("Internal error.");
        MSG_ShowDebugLog("Index Out Of the Range! Check indices...");
    case LCLE_AllocFailed:
        MSG_ShowError("Internal error.");
        MSG_ShowDebugLog("Out of memory. (malloc failed)");
    case LCLE_InvalidArgument:
        MSG_ShowError("Internal error.");
        MSG_ShowDebugLog("LineCounterList: InvalidArgument");
    }

    return CLE_ListError;
}

CL_Error CL_MapAndExceptCL(CLinesApp* self, CL_Error err) {
    switch (err) {
    case CLE_Ok:
        return CLE_Ok;
    case CLE_Todo:
        MSG_ShowDebugLog("TODO!");
        break;
    case CLE_ConfigError:
    case CLE_ListError:
    case CLE_SetError:
        // assume CL_MapAndExceptLCL / CL_MapAndExceptCFG / CL_MapAndExceptINS alredy called
        break;

    case CLE_ReadDirError:
    case CLE_CloseDirError:
    case CLE_FileOpenError:
    case CLE_FileReadError:
        if (self->errorDetails)
            MSG_ShowError("Failed to open file. (%s)", self->errorDetails);
        else
            MSG_ShowError("Failed to open file.");
        MSG_ShowTip("Are you sure you have read permissions to the specified directories?");
        break;
    case CLE_AllocFailed:
        MSG_ShowError("Internal error.");
        MSG_ShowDebugLog("Out of memory. (malloc failed)");
        break;
    case CLE_InternalError:
        if (self->errorDetails)
            MSG_ShowError("Internal error. (%s)", self->errorDetails);
        else
            MSG_ShowError("Internal error.");
        MSG_ShowDebugLog("Yes, CLE_InternalError");
        break;

    case CLE_InvalidArgError:
        MSG_ShowError("Invalid Argument.");
        break;
    case CLE_NoSuchFileOrDir:
        MSG_ShowError("No such file or directory: %s", self->errorDetails);
        break;
    case CLE_RegexError:
        MSG_ShowError("Invalid Regex.");
        break;
    }

    return err;
}

CL_Error CL_MapAndExceptINS(CLinesApp* self, INS_Error inerr) {
    switch (inerr) {
    case INSE_Ok:
        return CLE_Ok;
    case INSE_AlredyExists:
        MSG_ShowDebugLog("INodeSet: key alredy exists");
        return CLE_Ok; // not hard error
    case INSE_AllocFailed:
        MSG_ShowError("Internal error.");
        MSG_ShowDebugLog("Out of memory (malloc failed).");
    case INSE_Todo:
        MSG_ShowError("Internal error.");
        MSG_ShowDebugLog("TODO!");
    }

    return CLE_SetError;
}
