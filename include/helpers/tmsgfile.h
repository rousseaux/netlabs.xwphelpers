
/*
 *@@sourcefile tmsgfile.h:
 *      header file for tmsgfile.c. See notes there.
 *
 *      This file is entirely new with V0.9.0.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@include #include <os2.h>
 *@@include #include "tmsgfile.h"
 */

#if __cplusplus
extern "C" {
#endif

#ifndef TMSGFILE_HEADER_INCLUDED
#define TMSGFILE_HEADER_INCLUDED

APIRET tmfGetMessage(PCHAR *pTable,
                     ULONG cTable,
                     PBYTE pbBuffer,
                     ULONG cbBuffer,
                     PSZ pszMessageName,
                     PSZ pszFile,
                     PULONG pcbMsg);

APIRET tmfGetMessageExt(PCHAR* pTable,
                        ULONG cTable,
                        PBYTE pbBuffer,
                        ULONG cbBuffer,
                        PSZ pszMessageName,
                        PSZ pszFile,
                        PULONG pcbMsg);

#endif // TMSGFILE_HEADER_INCLUDED

#if __cplusplus
}
#endif

