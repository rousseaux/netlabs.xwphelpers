
/*
 *@@sourcefile lan.h:
 *      header file for lan.c. See notes there.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@include #include <os2.h>
 *@@include #include <netcons.h>
 *@@include #include "helpers\lan.h"
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

#if __cplusplus
extern "C" {
#endif

#ifndef LANH_HEADER_INCLUDED
    #define LANH_HEADER_INCLUDED

    APIRET lanInit(VOID);

    #pragma pack(1)
    typedef struct _SERVER
    {
        UCHAR       achServerName[CNLEN + 1];
                                        // server name (without leading \\)
        UCHAR       cVersionMajor;      // major version # of net
        UCHAR       cVersionMinor;      // minor version # of net
        ULONG       ulServerType;       // server type
        PSZ         pszComment;         // server comment
    } SERVER, *PSERVER;
    #pragma pack()

    APIRET lanQueryServers(PSERVER *paServers,
                           ULONG *pcServers);

#endif

#if __cplusplus
}
#endif

