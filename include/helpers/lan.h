
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

    #ifndef SNLEN
        #define CNLEN           15                  /* Computer name length     */
        #define UNCLEN          (CNLEN+2)           /* UNC computer name length */
        #define NNLEN           12                  /* 8.3 Net name length      */
        #define RMLEN           (UNCLEN+1+NNLEN)    /* Maximum remote name length */

        #define SNLEN           15                  /* Service name length      */
        #define STXTLEN         63                  /* Service text length      */
    #endif

    #pragma pack(1)

    /*
     *@@ SERVER:
     *
     */

    typedef struct _SERVER
    {
        UCHAR       achServerName[CNLEN + 1];
                                        // server name (without leading \\)
        UCHAR       cVersionMajor;      // major version # of net
        UCHAR       cVersionMinor;      // minor version # of net
        ULONG       ulServerType;       // server type
        PSZ         pszComment;         // server comment
    } SERVER, *PSERVER;

    /*
     *@@ SERVICEBUF1:
     *
     */

    typedef struct _SERVICEBUF1 // service_info_1
    {
        unsigned char   svci1_name[SNLEN+1];
        unsigned short  svci1_status;
        unsigned long   svci1_code;
        unsigned short  svci1_pid;
    } SERVICEBUF1, *PSERVICEBUF1;

    /*
     *@@ SERVICEBUF2:
     *
     */

    typedef struct _SERVICEBUF2 // service_info_2
    {
        unsigned char   svci2_name[SNLEN+1];   /* service name                  */
        unsigned short  svci2_status;          /* See status values below       */
        unsigned long   svci2_code;            /* install code of service       */
        unsigned short  svci2_pid;             /* pid of service program        */
        unsigned char   svci2_text[STXTLEN+1]; /* text area for use by services */
    } SERVICEBUF2, *PSERVICEBUF2;

    #pragma pack()

    #ifndef SV_TYPE_ALL
        #define SV_TYPE_ALL             0xFFFFFFFF   /* handy for NetServerEnum2 */
    #endif

    #ifndef SERVICE_INSTALL_STATE
        #define SERVICE_INSTALL_STATE           0x03
        #define SERVICE_UNINSTALLED             0x00
                    // service stopped (not running)
        #define SERVICE_INSTALL_PENDING         0x01
                    // service start pending
        #define SERVICE_UNINSTALL_PENDING       0x02
                    // service stop pending
        #define SERVICE_INSTALLED               0x03
                    // service started

        #define SERVICE_PAUSE_STATE             0x0C
        #define SERVICE_ACTIVE                  0x00
                    // service active (not paused)
        #define SERVICE_CONTINUE_PENDING        0x04
                    // service continue pending
        #define SERVICE_PAUSE_PENDING           0x08
                    // service pause pending
        #define SERVICE_PAUSED                  0x0C
                    // service paused

        #define SERVICE_NOT_UNINSTALLABLE       0x00
                    // service cannot be stopped
        #define SERVICE_UNINSTALLABLE           0x10
                    // service can be stopped

        #define SERVICE_NOT_PAUSABLE            0x00
                    // service cannot be paused
        #define SERVICE_PAUSABLE                0x20
                    // service can be paused

        /* Requester service only:
         * Bits 8,9,10 -- redirection paused/active */

        #define SERVICE_REDIR_PAUSED            0x700
        #define SERVICE_REDIR_DISK_PAUSED       0x100
                    // redirector for disks paused
        #define SERVICE_REDIR_PRINT_PAUSED      0x200
                    // redirector for spooled devices paused
        #define SERVICE_REDIR_COMM_PAUSED       0x400
                    // redirector for serial devices paused

        #define SERVICE_CTRL_INTERROGATE        0
        #define SERVICE_CTRL_PAUSE              1
        #define SERVICE_CTRL_CONTINUE           2
        #define SERVICE_CTRL_UNINSTALL          3
    #endif

    APIRET lanInit(VOID);

    APIRET lanQueryServers(PSERVER *paServers,
                           ULONG *pcServers);

    APIRET lanServiceGetInfo(PCSZ pcszServiceName,
                             PSERVICEBUF2 pBuf);

    APIRET lanServiceInstall(PCSZ pcszServiceName,
                             PSERVICEBUF2 pBuf2);

    APIRET lanServiceControl(PCSZ pcszServiceName,
                             ULONG opcode,
                             PSERVICEBUF2 pBuf2);
#endif

#if __cplusplus
}
#endif

