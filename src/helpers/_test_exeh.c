
#define OS2EMX_PLAIN_CHAR
    // this is needed for "os2emx.h"; if this is defined,
    // emx will define PSZ as _signed_ char, otherwise
    // as unsigned char

#define INCL_DOSMODULEMGR
#define INCL_DOSPROCESS
#define INCL_DOSEXCEPTIONS
#define INCL_DOSSESMGR
#define INCL_DOSQUEUES
#define INCL_DOSSEMAPHORES
#define INCL_DOSMISC
#define INCL_DOSDEVICES
#define INCL_DOSDEVIOCTL
#define INCL_DOSERRORS

#define INCL_KBD
#include <os2.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

#include "setup.h"                      // code generation and debugging options

#include "helpers\dosh.h"
#include "helpers\exeh.h"
#include "helpers\standards.h"

#pragma hdrstop

int main (int argc, char *argv[])
{
    int arc = 0;
    PEXECUTABLE pExe;

    if (argc != 2)
    {
        printf("exeh (built " __DATE__ "): displays the bldlevel of an executable.\n");
        printf("Usage: exeh <name.exe>\n");
        arc = 1003;
    }
    else
    {
        if (!(arc = exehOpen(argv[1], &pExe)))
        {
            if (!(arc = exehQueryBldLevel(pExe)))
            {
                #define STRINGORNA(p) ((pExe->p) ? (pExe->p) : "n/a")

                printf("exeh: dumping bldlevel of \"%s\"\n", argv[1]);
                printf("    Description:      \"%s\"\n", STRINGORNA(pszDescription));
                printf("    Vendor:           \"%s\"\n", STRINGORNA(pszVendor));
                printf("    Version:          \"%s\"\n", STRINGORNA(pszVersion));
                printf("    Info:             \"%s\"\n", STRINGORNA(pszInfo));
                printf("    Build date/time:  \"%s\"\n", STRINGORNA(pszBuildDateTime));
                printf("    Build machine:    \"%s\"\n", STRINGORNA(pszBuildMachine));
                printf("    ASD:              \"%s\"\n", STRINGORNA(pszASD));
                printf("    Language:         \"%s\"\n", STRINGORNA(pszLanguage));
                printf("    Country:          \"%s\"\n", STRINGORNA(pszCountry));
                printf("    Revision:         \"%s\"\n", STRINGORNA(pszRevision));
                printf("    Unknown string:   \"%s\"\n", STRINGORNA(pszUnknown));
                printf("    Fixpak:           \"%s\"\n", STRINGORNA(pszFixpak));

            }

            exehClose(&pExe);
        }

        if (arc)
        {
            printf("exeh: Error %d occured with \"%s\".\n", arc, argv[1]);
        }
    }

    return arc;
}
