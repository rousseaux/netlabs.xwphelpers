
/*
 *@@sourcefile apcih.c:
 *      contains helpers for accessing ACPI.
 *
 *      Usage: All OS/2 programs.
 *
 *      Function prefixes:
 *      --  acpih*   ACPI helper functions
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@header "helpers\acpih.h"
 *@@added V1.0.5 (2006-06-26) [pr]
 */

/*
 *      Copyright (C) 2006 Paul Ratcliffe.
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

#define INCL_DOSMODULEMGR
#define INCL_DOSERRORS
#include <os2.h>

#include "setup.h"                      // code generation and debugging options

#include "helpers\acpih.h"
#include "helpers\standards.h"

/* ******************************************************************
 *
 *   Globals
 *
 ********************************************************************/

HMODULE       hmodACPI = NULLHANDLE;

ACPISTARTAPI  *pAcpiStartApi = NULL;
ACPIENDAPI    *pAcpiEndApi = NULL;
ACPIGOTOSLEEP *pAcpiGoToSleep = NULL;

/*
 *@@category: Helpers\Control program helpers\ACPI
 *      See acpih.c.
 */

/*
 *@@ acpihOpen:
 *      resolves the ACPI entrypoints and loads the ACPI DLL.
 */

APIRET acpihOpen(ACPI_API_HANDLE *phACPI)
{
    APIRET arc = NO_ERROR;

    if (!hmodACPI)
        if (!(arc = DosLoadModule(NULL,
                                  0,
                                  "ACPI32",
                                  &hmodACPI)))
        {
            arc = DosQueryProcAddr(hmodACPI,
                                   ORD_ACPISTARTAPI,
                                   NULL,
                                   (PFN *) &pAcpiStartApi);
            if (!arc)
                arc = DosQueryProcAddr(hmodACPI,
                                   ORD_ACPIENDAPI,
                                   NULL,
                                   (PFN *) &pAcpiEndApi);

            if (!arc)
                arc = DosQueryProcAddr(hmodACPI,
                                   ORD_ACPIGOTOSLEEP,
                                   NULL,
                                   (PFN *) &pAcpiGoToSleep);
            if (arc)
            {
                DosFreeModule(hmodACPI);
                hmodACPI = NULLHANDLE;
                pAcpiStartApi = NULL;
                pAcpiEndApi = NULL;
                pAcpiGoToSleep = NULL;
                return(arc);
            }
        }

    if (arc)
        return(arc);
    else
        return(pAcpiStartApi(phACPI));
}

/*
 *@@ acpihClose:
 *      unloads the ACPI DLL.
 */

VOID acpihClose(ACPI_API_HANDLE *phACPI)
{
    if (pAcpiEndApi)
        pAcpiEndApi(phACPI);
}

/*
 *@@ acpihGoToSleep:
 *      changes the Power State.
 */

APIRET acpihGoToSleep(ACPI_API_HANDLE *phACPI, UCHAR ucState)
{
    if (pAcpiGoToSleep)
        return(pAcpiGoToSleep(phACPI, ucState));
    else
        return(ERROR_PROTECTION_VIOLATION);
}

