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
#define INCL_WINSTDCNR
#define INCL_WINSTDFILE

#include <os2.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "setup.h"                      // code generation and debugging options

#include "..\..\..\xworkplace\include\xwpapi.h"

#include "helpers\call_file_dlg.c"
#include "helpers\cnrh.h"
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

    return hwndReturn;
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
 *@@ GROUPRECORD:
 *
 */

typedef struct _GROUPRECORD
{
    RECORDCORE      recc;

    ULONG           gid;
    CHAR            szGroupName[100];    // group name
    PSZ             pszMembers;

} GROUPRECORD, *PGROUPRECORD;

/*
 *@@ main:
 *
 */

int main(int argc, char *argv[])
{
    HAB             hab;
    HMQ             hmq;

    #undef TBBS_TEXT
    #define TBBS_TEXT 0

    #define TBBS_COMMON TBBS_AUTORESIZE | TBBS_FLAT | TBBS_HILITE | WS_VISIBLE

    CHAR            szOpen[200],
                    szExit[200];

    TOOLBARCONTROL  aControls[] =
        {
            /*
            WC_CCTL_TBBUTTON,
            szExit,
            TBBS_COMMON | TBBS_BIGICON | TBBS_TEXT | TBBS_SYSCOMMAND,
            SC_CLOSE,
            10,
            10,

            WC_CCTL_TBBUTTON,
            szExit,
            TBBS_COMMON | TBBS_BIGICON,
            0,
            10,
            10,

            WC_CCTL_TBBUTTON,
            szExit,
            TBBS_COMMON | TBBS_MINIICON,
            0,
            10,
            10,
            */

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
            TBBS_COMMON | TBBS_TEXT | TBBS_RADIO | TBBS_CHECKINITIAL,
            1101,
            10,
            10,

            WC_CCTL_TBBUTTON,
            "Group 2",
            TBBS_COMMON | TBBS_TEXT | TBBS_RADIO,
            1102,
            10,
            10,

            WC_CCTL_TBBUTTON,
            "Group 3",
            TBBS_COMMON | TBBS_TEXT | TBBS_RADIO,
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
            0, // XFCF_TOOLBAR | XFCF_FORCETBOWNER | XFCF_STATUSBAR,
            WS_VISIBLE,                         // ulFrameStyle
            "Test File Dialog",                 // pcszFrameTitle
            0,                                  // ulResourcesID
#if 1
            WC_CCTL_CNR,
#else
            WC_CONTAINER,
#endif
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
    ctlRegisterXCnr(hab);

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

    WinSendMsg(hwndFrame, WM_UPDATEFRAME, MPNULL, MPNULL);

    {
        XFIELDINFO  xfi[4];
        PFIELDINFO      pfi = NULL;
        int i = 0;
        PGROUPRECORD preccFirst;

        // set up cnr details view
        xfi[i].ulFieldOffset = FIELDOFFSET(GROUPRECORD, gid);
        xfi[i].pszColumnTitle = "Group ID";     // @@todo localize
        xfi[i].ulDataType = CFA_ULONG;
        xfi[i++].ulOrientation = CFA_RIGHT;

        xfi[i].ulFieldOffset = FIELDOFFSET(GROUPRECORD, recc.pszIcon);
        xfi[i].pszColumnTitle = "Group name";   // @@todo localize
        xfi[i].ulDataType = CFA_STRING;
        xfi[i++].ulOrientation = CFA_CENTER;

        xfi[i].ulFieldOffset = FIELDOFFSET(GROUPRECORD, pszMembers);
        xfi[i].pszColumnTitle = "Members";   // @@todo localize
        xfi[i].ulDataType = CFA_STRING;
        xfi[i++].ulOrientation = CFA_LEFT;

        pfi = cnrhSetFieldInfos(hwndClient,
                                xfi,
                                i,             // array item count
                                TRUE,          // draw lines
                                0);            // return first column

        BEGIN_CNRINFO()
        {
            cnrhSetView(CV_DETAIL | CA_DETAILSVIEWTITLES);
            CnrInfo_.cyLineSpacing = 10;
            ulSendFlags_ |= CMA_LINESPACING;
        } END_CNRINFO(hwndClient);

        #define RECORD_COUNT        200

        if (preccFirst = (PGROUPRECORD)cnrhAllocRecords(hwndClient,
                                                        sizeof(GROUPRECORD),
                                                        RECORD_COUNT))
        {
            PGROUPRECORD preccThis = preccFirst;
            ULONG   ul = 0;
            while (preccThis)
            {
                preccThis->gid = ul++;
                sprintf(preccThis->szGroupName, "group %d", preccThis->gid);
                preccThis->recc.pszIcon = preccThis->szGroupName;

                preccThis->pszMembers = "longer string than title";

                preccThis = (PGROUPRECORD)preccThis->recc.preccNextRecord;
            }

            cnrhInsertRecords(hwndClient,
                              NULL,
                              (PRECORDCORE)preccFirst,
                              TRUE,
                              NULL,
                              CRA_RECORDREADONLY,
                              RECORD_COUNT);
        }
    }

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

    return 0;
}


