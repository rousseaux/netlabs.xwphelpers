#define OS2EMX_PLAIN_CHAR
    // this is needed for "os2emx.h"; if this is defined,
    // emx will define PSZ as _signed_ char, otherwise
    // as unsigned char

#define INCL_DOSPROCESS
#define INCL_DOSMISC
#define INCL_DOSERRORS

#define INCL_WINWINDOWMGR
#define INCL_WINFRAMEMGR
#define INCL_WINSTDFILE

#include <os2.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "setup.h"                      // code generation and debugging options

#include "R:\projects\R_cvs\xworkplace\include\xwpapi.h"

#include "helpers\call_file_dlg.c"

/*
 *@@ NewWinFileDlg:
 *      replacement for WinFileDlg. Use similarly.
 */

HWND APIENTRY NewWinFileDlg(HWND hwndOwner,
                            PFILEDLG pfd)       // WinFileDlg
{
    HWND        hwndReturn = NULLHANDLE;
    BOOL        fCallDefault = TRUE;

    hwndReturn = ImplCallFileDlg(hwndOwner, pfd, &fCallDefault);

    if (fCallDefault)
        // something went wrong:
        hwndReturn = WinFileDlg(HWND_DESKTOP,
                                hwndOwner,
                                pfd);

    return (hwndReturn);
}


int main (int argc, char *argv[])
{
    HAB             hab;
    HMQ             hmq;

    FILEDLG         fd;

    hab = WinInitialize(0);
    hmq = WinCreateMsgQueue(hab, 0);

    memset(&fd, 0, sizeof(FILEDLG));
    fd.cbSize = sizeof(FILEDLG);
    fd.fl = FDS_CENTER | FDS_OPEN_DIALOG;

    strcpy(fd.szFullFile, "C:\\*");

    if (NewWinFileDlg(NULLHANDLE,
                      &fd))
    {
        CHAR sz[1000];
        sprintf(sz, "got: \"%s\"", fd.szFullFile);
        WinMessageBox(HWND_DESKTOP, NULLHANDLE,
                      sz,
                      "File:",
                      0,
                      MB_OK | MB_MOVEABLE);
    }
    else
        WinMessageBox(HWND_DESKTOP, NULLHANDLE,
                      "file dlg returned FALSE",
                      "File:",
                      0,
                      MB_OK | MB_MOVEABLE);

    WinDestroyMsgQueue(hmq);
    WinTerminate(hab);

    return (0);
}


