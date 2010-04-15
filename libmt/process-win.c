/* vim: set et sts=2 ts=2 sw=2 tw=78: */

#ifdef _WIN32

#include "internal.h"
#include <dmem/vector.h>

DVECTOR_INIT(wchar_t, wchar_t)

static void AppendAscii(d_Vector(wchar_t)* s, const char* str)
{
    int wlen = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    if (wlen > 0) {
        wchar_t* dest = dv_push(wchar_t, s, wlen);
        MultiByteToWideChar(CP_ACP, 0, str, -1, dest, wlen);
    }
}

static void Append(d_Vector(wchar_t)* s, const wchar_t* str, size_t sz)
{
    wchar_t* dest = dv_push(wchar_t, s, sz);
    memcpy(dest, str, sz * sizeof(wchar_t));
}

static wchar_t* Finish(d_Vector(wchar_t)* s)
{
    if (dv_size(s) == 0) {
        return NULL;
    }

    Append(s, L"\0", 1);
    return dv_data(s);
}

static void AppendCurrentDirectory(d_Vector(wchar_t)* s)
{
    DWORD req = GetCurrentDirectoryW(0, NULL);
    wchar_t* dest = dv_push(wchar_t, s, req);
    GetCurrentDirectoryW(req, dest);
    Append(s, L"\\", 1);
}

int MT_Process_Start(const char* app, const char* dir, const char* args[], size_t argnum)
{
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    d_Vector(wchar_t) cmdline, abscmd, abspath;
    const char* appfile;
    BOOL ret;
    size_t i;

    memset(&cmdline, 0, sizeof(cmdline));
    memset(&abscmd, 0, sizeof(abscmd));
    memset(&abspath, 0, sizeof(abspath));
    memset(&si, 0, sizeof(si));
    memset(&pi, 0, sizeof(pi));
    si.cb = sizeof(si);

    Append(&cmdline, L"\"", 1);
    AppendAscii(&cmdline, app);
    Append(&cmdline, L"\"", 1);

    for (i = 0; i < argnum; i++) {
        Append(&cmdline, L"\"", 1);
        AppendAscii(&cmdline, args[i]);
        Append(&cmdline, L"\"", 1);
    }

    if (dir) {
        if (dir[0] != '/' && dir[0] != '\\' && strchr(dir, ':') == NULL) {
            AppendCurrentDirectory(&abspath);
        }
        AppendAscii(&abspath, dir);
    }

    if (app[0] != '/' && app[0] != '\\' && strchr(app, ':') == NULL) {
        AppendCurrentDirectory(&abscmd);
    }
    AppendAscii(&abscmd, app);

    /* See if app has an extension. If it does not append .exe to abscmd */
    appfile = app + strlen(app);
    while (appfile > app && *appfile != '\\' && *appfile != '/') {
        appfile--;
    }
    if (strchr(appfile, '.') == NULL) {
        Append(&abscmd, L".exe", 4);
    }

    ret = CreateProcessW(
          Finish(&abscmd),                        // lpApplicationName
          Finish(&cmdline),                       // lpCommandLine
          NULL,                                   // lpProcessAttributes
          NULL,                                   // lpThreadAttributes
          FALSE,                                  // bInheritHandles
          CREATE_NO_WINDOW | DETACHED_PROCESS,    // dwCreationFlags
          NULL,                                   // lpEnvironment
          Finish(&abspath),                       // lpCurrentDirectory
          &si,                                    // lpStartupInfo
          &pi);                                   // lpProcessInformation

    dv_free(wchar_t, &abscmd);
    dv_free(wchar_t, &cmdline);
    dv_free(wchar_t, &abspath);

    if (!ret) {
        return -1;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return 0;
}

#endif
