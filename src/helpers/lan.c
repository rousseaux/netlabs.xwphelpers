
/*
 *@@sourcefile lan.c:
 *      LAN helpers.
 *
 *      Usage: All OS/2 programs.
 *
 *      Function prefixes (new with V0.81):
 *      --  nls*        LAN helpers
 *
 *      This file is new with 0.9.16.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@header "helpers\nls.h"
 *@@added V0.9.16 (2001-10-19) [umoeller]
 */

/*
 *      Copyright (C) 2001 Ulrich M”ller.
 *      This file is part of the "XWorkplace helpers" source package.
 *      This is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published
 *      by the Free Software Foundation, in version 2 as it comes in the
 *      "COPYING" file of the XWorkplace main distribution.
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 */

#define OS2EMX_PLAIN_CHAR
    // this is needed for "os2emx.h"; if this is defined,
    // emx will define PSZ as _signed_ char, otherwise
    // as unsigned char

#define INCL_DOS
#include <os2.h>

#define PURE_32
#include <neterr.h>
#include <netcons.h>
#include <server.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "setup.h"                      // code generation and debugging options

#include "helpers\dosh.h"
#include "helpers\lan.h"
#include "helpers\standards.h"

#pragma hdrstop

/*
 *@@category: Helpers\Network helpers
 *      See lan.c.
 */

/* ******************************************************************
 *
 *   Prototypes
 *
 ********************************************************************/

typedef API32_FUNCTION NET32WKSTAGETINFO(const unsigned char *pszServer,
                                         unsigned long ulLevel,
                                         unsigned char *pbBuffer,
                                         unsigned long ulBuffer,
                                         unsigned long *pulTotalAvail);

typedef API32_FUNCTION NET32SERVERENUM2(const unsigned char *pszServer,
                                        unsigned long ulLevel,
                                        unsigned char *pbBuffer,
                                        unsigned long cbBuffer,
                                        unsigned long *pcEntriesRead,
                                        unsigned long *pcTotalAvail,
                                        unsigned long flServerType,
                                        unsigned char *pszDomain);

/* ******************************************************************
 *
 *   Globals
 *
 ********************************************************************/

HMODULE         hmodLan = NULLHANDLE;

NET32WKSTAGETINFO *pNet32WkstaGetInfo = NULL;
NET32SERVERENUM2 *pNet32ServerEnum2 = NULL;

RESOLVEFUNCTION NetResolves[] =
    {
        "Net32WkstaGetInfo", (PFN*)&pNet32WkstaGetInfo,
        "Net32ServerEnum2", (PFN*)&pNet32ServerEnum2
    };

static ULONG    G_ulNetsResolved = -1;      // -1 == not init'd
                                            // 0  == success
                                            // APIRET == failure

/*
 *@@ lanInit:
 *
 */

APIRET lanInit(VOID)
{
    if (G_ulNetsResolved == -1)
    {
        G_ulNetsResolved = doshResolveImports("NETAPI32",   // \MUGLIB\DLL
                                              &hmodLan,
                                              NetResolves,
                                              ARRAYITEMCOUNT(NetResolves));
    }

    printf(__FUNCTION__ ": arc %d\n", G_ulNetsResolved);

    return (G_ulNetsResolved);
}

/*
 *@@ lanQueryServers:
 *      returns an array of SERVER structures describing
 *      the servers which are available from this computer.
 *
 *      With PEER, this should return the peers, assuming
 *      that remote IBMLAN.INI's do not contain "srvhidden = yes".
 *
 *      If NO_ERROR is returned, the caller should free()
 *      the returned array
 */

APIRET lanQueryServers(PSERVER *paServers,      // out: array of SERVER structs
                       ULONG *pcServers)        // out: array item count (NOT array size)
{
    APIRET arc;
    if (!(arc = lanInit()))
    {
        ULONG   ulEntriesRead = 0,
                cTotalAvail = 0;
        PSERVER pBuf;
        ULONG cb = 4096;            // set this fixed, can't get it to work otherwise
        if (pBuf = (PSERVER)doshMalloc(cb,
                                       &arc))
        {
            if (!(arc = pNet32ServerEnum2(NULL,
                                          1,          // ulLevel
                                          (PUCHAR)pBuf,       // pbBuffer
                                          cb,          // cbBuffer,
                                          &ulEntriesRead, // *pcEntriesRead,
                                          &cTotalAvail,   // *pcTotalAvail,
                                          SV_TYPE_ALL,    // all servers
                                          NULL)))      // pszDomain == all domains
            {
                *pcServers = ulEntriesRead;
                *paServers = pBuf;
            }
            else
                free(pBuf);
        }
    }

    printf(__FUNCTION__ ": arc %d\n", arc);

    return arc;
}

#ifdef __LAN_TEST__

int main(void)
{
    ULONG ul;

    PSERVER paServers;
    ULONG cServers;
    lanQueryServers(&paServers, &cServers);

    for (ul = 0;
         ul < cServers;
         ul++)
    {
        printf("Server %d: \\\\%s (%s)\n",
               ul,
               paServers[ul].achServerName,
               paServers[ul].pszComment);
    }

    return 0;
}

#endif

