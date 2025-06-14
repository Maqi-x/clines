#include <App.h>

int main(int argc, char** argv) {
    CLinesApp app;
    CL_Error err;

    err = CL_Init(&app);
    if (err != CLE_Ok) {
        fprintf(stderr, "internal error.\n");
        return (int)CLE_InternalError;
    }

    int exitCode = CL_Run(&app, argc, argv);

    CL_Destroy(&app);
    return exitCode;
}
