
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
#include "helpers\standards.h"

#pragma hdrstop

int main (int argc, char *argv[])
{
    ULONG ul;

            printf("Drive checker ("__DATE__")\n");
            printf("    type   fs      remot fixed parrm bootd       media audio eas   longn\n");

    for (ul = 1;
         ul <= 26;
         ul++)
    {
        XDISKINFO xdi;
        APIRET arc = doshGetDriveInfo(ul,
                                      0, // DRVFL_TOUCHFLOPPIES,
                                      &xdi);

        printf(" %c: ", xdi.cDriveLetter, ul);

        if (!xdi.fPresent)
            printf("not present\n");
        else
        {
            if (arc)
                printf("error %d (IsFixedDisk: %d, QueryDiskParams %d, QueryMedia %d)\n",
                        arc,
                        xdi.arcIsFixedDisk,
                        xdi.arcQueryDiskParams,
                        xdi.arcQueryMedia);
            else
            {
                ULONG   aulFlags[] =
                    {
                        DFL_REMOTE,
                        DFL_FIXED,
                        DFL_PARTITIONABLEREMOVEABLE,
                        DFL_BOOTDRIVE,
                        0,
                        DFL_MEDIA_PRESENT,
                        DFL_AUDIO_CD,
                        DFL_SUPPORTS_EAS,
                        DFL_SUPPORTS_LONGNAMES
                    };
                ULONG ul2;

                PCSZ pcsz = NULL;

                switch (xdi.bType)
                {
                    case DRVTYPE_HARDDISK:  pcsz = "HDISK ";    break;
                    case DRVTYPE_FLOPPY:    pcsz = "FLOPPY"; break;
                    case DRVTYPE_TAPE:      pcsz = "TAPE  "; break;
                    case DRVTYPE_VDISK:     pcsz = "VDISK "; break;
                    case DRVTYPE_CDROM:     pcsz = "CDROM "; break;
                    case DRVTYPE_LAN:       pcsz = "LAN   "; break;
                    case DRVTYPE_PARTITIONABLEREMOVEABLE:
                                            pcsz = "PRTREM"; break;
                    default:
                                            printf("bType=%d, BPB.bDevType=%d",
                                                    xdi.bType,
                                                    xdi.bpb.bDeviceType);
                                            printf("\n           ");
                }

                if (pcsz)
                    printf("%s ", pcsz);

                if (xdi.lFileSystem < 0)
                    // negative means error
                    printf("E%3d    ", xdi.lFileSystem); // , xdi.bFileSystem);
                else
                    printf("%7s ", xdi.szFileSystem); // , xdi.bFileSystem);

                for (ul2 = 0;
                     ul2 < ARRAYITEMCOUNT(aulFlags);
                     ul2++)
                {
                    if (xdi.flDevice & aulFlags[ul2])
                        printf("  X   ");
                    else
                        if (    (xdi.arcOpenLongnames)
                             && (aulFlags[ul2] == DFL_SUPPORTS_LONGNAMES)
                           )
                            printf(" E%03d ", xdi.arcOpenLongnames);
                        else
                            printf("  -   ");
                }
                printf("\n");
            }
        }
    }
}
