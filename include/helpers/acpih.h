
/*
 *@@sourcefile acpih.h:
 *      header file for acpih.c.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@include #include <os2.h>
 *@@include #include "helpers\acpih.h"
 */

/*      Copyright (C) 2006 Paul Ratcliffe.
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

#if __cplusplus
extern "C" {
#endif

#ifndef ACPIH_HEADER_INCLUDED
    #define ACPIH_HEADER_INCLUDED

    /*
     * Power state values
     */

    #define ACPI_STATE_UNKNOWN              (UCHAR) 0xFF

    #define ACPI_STATE_S0                   (UCHAR) 0
    #define ACPI_STATE_S1                   (UCHAR) 1
    #define ACPI_STATE_S2                   (UCHAR) 2
    #define ACPI_STATE_S3                   (UCHAR) 3
    #define ACPI_STATE_S4                   (UCHAR) 4
    #define ACPI_STATE_S5                   (UCHAR) 5
    #define ACPI_S_STATES_MAX               ACPI_STATE_S5
    #define ACPI_S_STATE_COUNT              6

    #pragma pack(1)

    typedef struct _VersionAcpi_
    {
        ULONG  Major;
        ULONG  Minor;
    } ACPI_VERSION;

    typedef struct _AcpiApiHandle_
    {
        HFILE           AcpiDrv;                 // Handle to ACPICA driver
        ACPI_VERSION    PSD;                     // Version PSD
        ACPI_VERSION    Driver;                  // Version ACPICA driver
        ACPI_VERSION    DLL;                     // Version acpi32.dll
        ULONG           StartAddrPSD;            // Start address PSD (for testcase)
        ULONG           AddrCommApp;             // Address DosCommApp from PSD (which not write IBM)
        ULONG           StartAddrDriver;         // Start address ACPICA (for testcase)
        ULONG           AddrFindPSD;             // Address function for find PSD (find CommApp)
        ULONG           IRQNumber;               // Number use IRQ
        void            *Internal;               // For internal DLL use
    } ACPI_API_HANDLE, *PACPI_API_HANDLE;

    /* ******************************************************************
     *
     *   ACPI helper APIs
     *
     ********************************************************************/

    #pragma pack()

    APIRET APIENTRY AcpiStartApi(ACPI_API_HANDLE *);
    typedef APIRET APIENTRY ACPISTARTAPI(ACPI_API_HANDLE *);
    typedef ACPISTARTAPI *PACPISTARTAPI;

    APIRET APIENTRY AcpiEndApi(ACPI_API_HANDLE *);
    typedef APIRET APIENTRY ACPIENDAPI(ACPI_API_HANDLE *);
    typedef ACPIENDAPI *PACPIENDAPI;

    APIRET APIENTRY AcpiGoToSleep(ACPI_API_HANDLE *, UCHAR);
    typedef APIRET APIENTRY ACPIGOTOSLEEP(ACPI_API_HANDLE *, UCHAR);
    typedef ACPIGOTOSLEEP *PACPIGOTOSLEEP;

    APIRET APIENTRY acpihOpen(ACPI_API_HANDLE *phACPI);

    VOID APIENTRY acpihClose(ACPI_API_HANDLE *phACPI);

    APIRET APIENTRY acpihGoToSleep(ACPI_API_HANDLE *phACPI, UCHAR ucState);

    #define ORD_ACPISTARTAPI    16
    #define ORD_ACPIENDAPI      17
    #define ORD_ACPIGOTOSLEEP   19

#endif

#if __cplusplus
}
#endif

