#define OS2EMX_PLAIN_CHAR
    // this is needed for "os2emx.h"; if this is defined,
    // emx will define PSZ as _signed_ char, otherwise
    // as unsigned char

#define INCL_DOSPROCESS
#define INCL_DOSMISC
#define INCL_DOSERRORS

#define INCL_WINWINDOWMGR
#define INCL_WINFRAMEMGR
#define INCL_WINMENUS
#define INCL_WINBUTTONS
#define INCL_WINPOINTERS
#define INCL_WINSTDFILE

#include <os2.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "setup.h"                      // code generation and debugging options

#include "..\..\..\xworkplace\include\xwpapi.h"

#include "helpers\call_file_dlg.c"
#include "helpers\comctl.h"
#include "helpers\standards.h"
#include "helpers\winh.h"
#include "helpers\gpih.h"

PCSZ    WC_CLIENT = "MyClient";

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

/*
 *@@ ShowFileDlg:
 *
 */

VOID ShowFileDlg(HWND hwndFrame)
{
    FILEDLG         fd;

    memset(&fd, 0, sizeof(FILEDLG));
    fd.cbSize = sizeof(FILEDLG);
    fd.fl = FDS_CENTER | FDS_OPEN_DIALOG;

    strcpy(fd.szFullFile, "C:\\*");

    if (NewWinFileDlg(hwndFrame,
                      &fd))
    {
        CHAR sz[1000];
        sprintf(sz, "got: \"%s\"", fd.szFullFile);
        WinMessageBox(HWND_DESKTOP, hwndFrame,
                      sz,
                      "File:",
                      0,
                      MB_OK | MB_MOVEABLE);
    }
    else
        WinMessageBox(HWND_DESKTOP, hwndFrame,
                      "file dlg returned FALSE",
                      "File:",
                      0,
                      MB_OK | MB_MOVEABLE);
}

/*
 *@@ fnwpClient:
 *
 */

MRESULT EXPENTRY fnwpClient(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    MRESULT mrc = 0;

    switch (msg)
    {
        case WM_PAINT:
        {
            HPS hps;
            RECTL rcl;
            if (hps = WinBeginPaint(hwnd, NULLHANDLE, &rcl))
            {
                gpihSwitchToRGB(hps);
                WinFillRect(hps, &rcl, RGBCOL_GRAY);
                WinEndPaint(hps);
            }
        }
        break;

        default:
            mrc = WinDefWindowProc(hwnd, msg, mp1, mp2);
    }

    return mrc;
}

/*
 *@@ main:
 *
 */

int main(int argc, char *argv[])
{
    HAB             hab;
    HMQ             hmq;

    #define TBBS_COMMON TBBS_AUTORESIZE | TBBS_FLAT | TBBS_HILITE | WS_VISIBLE

    CHAR            szOpen[200],
                    szExit[200];

    TOOLBARCONTROL  aControls[] =
        {
            WC_CCTL_TBBUTTON,
            szExit,
            TBBS_COMMON | TBBS_BIGICON | TBBS_TEXT | TBBS_SYSCOMMAND,
            SC_CLOSE,
            10,
            10,

            WC_CCTL_TBBUTTON,
            szExit,
            TBBS_COMMON | TBBS_BIGICON /* TBBS_TEXT | */ ,
            0,
            10,
            10,

            WC_CCTL_TBBUTTON,
            szExit,
            TBBS_COMMON | TBBS_MINIICON /* TBBS_TEXT | */ ,
            0,
            10,
            10,

            WC_CCTL_TBBUTTON,
            szOpen,
            TBBS_COMMON | TBBS_MINIICON | TBBS_TEXT,
            1000,
            10,
            10,

            WC_CCTL_TBBUTTON,
            "Toggle\ntest",
            TBBS_COMMON | TBBS_TEXT | TBBS_CHECK,
            1001,
            10,
            10,

            WC_CCTL_SEPARATOR,
            NULL,
            WS_VISIBLE | SEPS_VERTICAL,
            1002,
            10,
            10,

            WC_CCTL_TBBUTTON,
            "Group 1",
            TBBS_COMMON | TBBS_TEXT | TBBS_CHECKGROUP | TBBS_CHECKINITIAL,
            1101,
            10,
            10,

            WC_CCTL_TBBUTTON,
            "Group 2",
            TBBS_COMMON | TBBS_TEXT | TBBS_CHECKGROUP,
            1102,
            10,
            10,

            WC_CCTL_TBBUTTON,
            "Group 3",
            TBBS_COMMON | TBBS_TEXT | TBBS_CHECKGROUP,
            1103,
            10,
            10,

        };

    EXTFRAMECDATA   xfd =
        {
            NULL,                               // pswpFrame
            FCF_TITLEBAR
                  | FCF_SYSMENU
                  | FCF_MINMAX
                  | FCF_SIZEBORDER
                  | FCF_NOBYTEALIGN
                  | FCF_SHELLPOSITION
                  | FCF_TASKLIST,
            XFCF_TOOLBAR | XFCF_FORCETBOWNER | XFCF_STATUSBAR,
            WS_VISIBLE,                         // ulFrameStyle
            "Test File Dialog",                 // pcszFrameTitle
            0,                                  // ulResourcesID
            WC_CLIENT,                          // pcszClassClient
            WS_VISIBLE,                         // flStyleClient
            0,                                  // ulID
            NULL,
            HINI_USER,
            "XWorkplace Test Apps",
            "CallFileDlgPos",

            ARRAYITEMCOUNT(aControls),
            aControls
        };

    HWND            hwndFrame,
                    hwndClient,
                    hwndStatusBar,
                    hwndToolBar,
                    hwndMenu,
                    hwndSubmenu;
    QMSG            qmsg;

    hab = WinInitialize(0);
    hmq = WinCreateMsgQueue(hab, 0);

    winhInitGlobals();

    ctlRegisterToolbar(hab);
    ctlRegisterSeparatorLine(hab);

    WinRegisterClass(hab,
                     (PSZ)WC_CLIENT,
                     fnwpClient,
                     0,
                     4);

    sprintf(szOpen,
            "#%d#Open",
            WinQuerySysPointer(HWND_DESKTOP,
                               SPTR_ICONINFORMATION,
                               FALSE));

    sprintf(szExit,
            "#%d#Exit",
            WinQuerySysPointer(HWND_DESKTOP,
                               SPTR_ICONWARNING,
                               FALSE));

    hwndFrame = ctlCreateStdWindow(&xfd, &hwndClient);

    hwndToolBar = WinWindowFromID(hwndFrame, FID_TOOLBAR);
    hwndStatusBar = WinWindowFromID(hwndFrame, FID_STATUSBAR);

    WinSetWindowText(hwndToolBar, "Tool bar");
    WinSetWindowText(hwndStatusBar, "Status bar");

    hwndMenu = WinCreateMenu(hwndFrame,
                             NULL);

    hwndSubmenu = winhInsertSubmenu(hwndMenu,
                                    MIT_END,
                                    1,
                                    "~File",
                                    MIS_TEXT | MIS_SUBMENU,
                                    1000,
                                    "Open...",
                                    MIS_TEXT,
                                    0);

    winhInsertMenuItem(hwndSubmenu,
                       MIT_END,
                       SC_CLOSE,
                       "~Quit",
                       MIS_SYSCOMMAND | MIS_TEXT,
                       0);

    WinSendMsg(hwndFrame, WM_UPDATEFRAME, MPNULL, MPNULL);

    while (WinGetMsg(hab, &qmsg, NULLHANDLE, 0, 0))
    {
        BOOL fDispatch = TRUE;

        if (qmsg.hwnd == hwndFrame)
        {
            switch (qmsg.msg)
            {
                case WM_COMMAND:
                    if (SHORT1FROMMP(qmsg.mp1) == 1000)
                    {
                        ShowFileDlg(hwndFrame);
                        fDispatch = FALSE;
                    }
                break;
            }
        }

        if (fDispatch)
            WinDispatchMsg(hab, &qmsg);
    }

    WinDestroyMsgQueue(hmq);
    WinTerminate(hab);

    return (0);
}


