
/*
 *@@sourcefile tmsgfile.c:
 *      contains a "text message file" helper function,
 *      which works similar to DosGetMessage. See
 *      tmfGetMessage for details.
 *
 *      tmfGetMessage operates on plain-text ".TMF" files.
 *      The file is "compiled" into the file's extended
 *      attributes for faster access after the first access.
 *      When the file's contents change, the EAs are recompiled
 *      automatically.
 *
 *      This has the following advantages over DosGetMessage:
 *
 *      1)  No external utility is necessary to change message
 *          files. Simply edit the text, and on the next call,
 *          the file is recompiled at run-time.
 *
 *      2)  To identify messages, any string can be used, not
 *          only numerical IDs.
 *
 *      This file is entirely new with V0.9.0.
 *
 *      Usage: All OS/2 programs.
 *
 *      Function prefixes:
 *      --  tmf*   text message file functions
 *
 *      This code uses only C standard library functions
 *      which are also part of the subsystem libraries.
 *      It does _not_ use the fopen etc. functions, but
 *      DosOpen etc. instead.
 *      You can therefore use this code with your software
 *      which uses the subsystem libraries only.
 *
 *      This code was kindly provided by Christian Langanke.
 *      Modifications April 1999 by Ulrich M”ller. Any
 *      substantial changes (other than comments and
 *      formatting) are marked with (*UM).
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@header "helpers\tmsgfile.h"
 *@@added V0.9.0 [umoeller]
 */

/*
 *      Copyright (C) 1999 Christian Langanke.
 *      Copyright (C) 1999-2000 Ulrich M”ller.
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

#define INCL_DOSFILEMGR
#define INCL_DOSMISC
#define INCL_DOSNLS
#define INCL_DOSERRORS
#include <os2.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "setup.h"                      // code generation and debugging options

#include "helpers\eah.h"
#include "helpers\tmsgfile.h"

/*
 *@@category: Helpers\Control program helpers\Text message files (TMF)
 */

/* ******************************************************************
 *
 *   Declarations
 *
 ********************************************************************/

// extended attribute used for timestamp; written to .TMF file
#define EA_TIMESTAMP "TMF.FILEINFO"
// extended attribute used for compiled text file; written to .TMF file
#define EA_MSGTABLE  "TMF.MSGTABLE"

#define NEWLINE "\n"

// start-of-msgid marker
#define MSG_NAME_START   "\r\n<--"
// end-of-msgid marker
#define MSG_NAME_END     "-->:"       // fixed: the space had to be removed, because
                                      // if a newline was the first character after ":",
                                      // we got complete garbage (crashes) (99-10-23) [umoeller]
// comment marker
#define MSG_COMMENT_LINE "\r\n;"

// internal prototypes
APIRET          CompileMsgTable(PSZ pszMessageFile, PBYTE * ppbTableData);
APIRET          GetTimeStamp(PFILESTATUS3 pfs3, PSZ pszBuffer, ULONG ulBufferlen);

/* ******************************************************************
 *
 *   Text Message File Code
 *
 ********************************************************************/

/*
 *@@ tmfGetMessage:
 *      just like DosGetMessage, except that this does not
 *      take a simple message number for the message in the
 *      given message file, but a PSZ message identifier
 *      (pszMessageName).
 *
 *      This function automatically compiles the given .TMF
 *      file into the file's extended attributes, if we find
 *      that this is necessary, by calling CompileMsgTable.
 *
 *      Note that for compatibily, this function performs the
 *      same brain-dead string processing as DosGetMessage, that
 *      is, the return string is _not_ null-terminated, and
 *      trailing newline characters are _not_ cut off.
 *
 *      If you don't like this, call tmfGetMessageExt, which
 *      calls this function in turn.
 *
 *      <B>Returns:</B>
 *      --  ERROR_INVALID_PARAMETER
 *      --  ERROR_BUFFER_OVERFLOW
 *      --  ERROR_MR_MID_NOT_FOUND
 *      --  ERROR_MR_INV_MSGF_FORMAT
 *      --  ERROR_BUFFER_OVERFLOW
 *
 *      plus the error codes of DosOpen, DosRead, DosClose.
 */

APIRET tmfGetMessage(PCHAR* pTable,    // in: pointer table for string insertion
                     ULONG cTable,     // in: number of pointers in *pTable
                     PBYTE pbBuffer,   // out: returned string
                     ULONG cbBuffer,   // out: sizeof(*pbBuffer)
                     PSZ pszMessageName,  // in: message identifier
                     PSZ pszFile,      // in: message file (.TMF extension proposed)
                     PULONG pcbMsg)    // out: bytes written to *pbBuffer
{
    // fixed UM 99-10-22: now initializing all vars to 0,
    // because ulBytesRead might be returned from this func
    APIRET          rc = NO_ERROR;
    CHAR            szMessageFile[_MAX_PATH];
    PBYTE           pbTableData = NULL;
    PSZ             pszEntry = 0;
    PSZ             pszEndOfEntry = 0;
    ULONG           ulMessagePos = 0;
    ULONG           ulMessageLen = 0;

    HFILE           hfile = NULLHANDLE;
    ULONG           ulFilePtr = 0;
    ULONG           ulAction = 0;
    ULONG           ulBytesToRead = 0;
    ULONG           ulBytesRead = 0;

    do
    {
        // check parms
        if ((!pbBuffer) ||
            (!cbBuffer) ||
            (!pszMessageName) ||
            (!*pszMessageName) ||
            (!*pszFile))
        {
            rc = ERROR_INVALID_PARAMETER;
            break;
        }

        if (cbBuffer < 2)
        {
            rc = ERROR_BUFFER_OVERFLOW;
            break;
        }

        // search file
        if ((strchr(pszFile, ':')) ||
            (strchr(pszFile, '\\')) ||
            (strchr(pszFile, '/')))
            // drive and/or path given: no search in path
            strcpy(szMessageFile, pszFile);
        else
        {
            // only filename, search in current dir and DPATH
            rc = DosSearchPath(SEARCH_IGNORENETERRS
                               | SEARCH_ENVIRONMENT
                               | SEARCH_CUR_DIRECTORY,
                               "DPATH",
                               pszFile,
                               szMessageFile,
                               sizeof(szMessageFile));
            if (rc != NO_ERROR)
                break;
        }

        // _Pmpf(("tmfGetMessage: Found %s", szMessageFile));

        // compile table if neccessary
        rc = CompileMsgTable(szMessageFile, &pbTableData);
        if (rc != NO_ERROR)
            break;

        // _Pmpf(("tmfGetMessage: Now searching compiled file"));

        // search the name
        pszEntry = strstr(pbTableData, pszMessageName);
        if (!pszEntry)
        {
            rc = ERROR_MR_MID_NOT_FOUND;
            break;
        }
        else
            pszEntry += strlen(pszMessageName) + 1;

        // isolate entry
        pszEndOfEntry = strchr(pszEntry, '\n');
        if (pszEndOfEntry)
            *pszEndOfEntry = 0;

        // get numbers
        ulMessagePos = atol(pszEntry);
        if (ulMessagePos == 0)
            if (!pszEntry)
            {
                rc = ERROR_MR_INV_MSGF_FORMAT;
                break;
            }

        pszEntry = strchr(pszEntry, ' ');
        if (!pszEntry)
        {
            rc = ERROR_MR_INV_MSGF_FORMAT;
            break;
        }
        ulMessageLen = atol(pszEntry);

        // determine maximum read len
        ulBytesToRead = _min(ulMessageLen, cbBuffer);

        // open file and read message
        rc = DosOpen(pszFile,
                     &hfile,
                     &ulAction,
                     0, 0,
                     OPEN_ACTION_FAIL_IF_NEW
                     | OPEN_ACTION_OPEN_IF_EXISTS,
                     OPEN_FLAGS_FAIL_ON_ERROR
                     | OPEN_SHARE_DENYWRITE
                     | OPEN_ACCESS_READONLY,
                     NULL);
        if (rc != NO_ERROR)
            break;

        rc = DosSetFilePtr(hfile, ulMessagePos, FILE_BEGIN, &ulFilePtr);
        if ((rc != NO_ERROR) || (ulFilePtr != ulMessagePos))
            break;

        rc = DosRead(hfile,
                     pbBuffer,
                     ulBytesToRead,
                     &ulBytesRead);
        if (rc != NO_ERROR)
            break;

        // report "buffer too small" here
        if (ulBytesToRead < ulMessageLen)
            rc = ERROR_BUFFER_OVERFLOW;

    }
    while (FALSE);

    // replace string placeholders ("%1" etc.) (*UM)
    if (rc == NO_ERROR)     // added (99-10-24) [umoeller]
        if (cTable)
        {
            // create a temporary buffer for replacements
            PSZ             pszTemp = (PSZ)malloc(cbBuffer); // ### memory leak here!

            if (!pszTemp)
                rc = ERROR_NOT_ENOUGH_MEMORY;
            else
            {
                // two pointers for copying
                PSZ             pSource = pbBuffer,
                                pTarget = pszTemp;

                CHAR            szMsgNum[5] = "0";
                ULONG           ulTableIndex = 0;

                ULONG           ulCount = 0, ulTargetWritten = 0;

                // 1) copy the stuff we've read to the
                // new temporary buffer
                for (ulCount = 0;
                     ulCount < ulBytesRead;
                     ulCount++)
                    *pTarget++ = *pSource++;

                // 2) now go thru the source string
                // (which we've read above) and
                // look for "%" characters; we do
                // this while copying the stuff
                // back to pbBuffer

                pSource = pszTemp;
                pTarget = pbBuffer;

                for (ulCount = 0;
                     ulCount < ulBytesRead;
                     ulCount++)
                {
                    if (*pSource == '%')
                    {
                        // copy the index (should be > 0)
                        szMsgNum[0] = *(pSource + 1);
                        // szMsgNum[1] is still 0

                        // Pmpf(("      Found %s", szMsgNum));

                        ulTableIndex = atoi(szMsgNum);
                        // _Pmpf(("        --> %d", ulTableIndex));
                        if ((ulTableIndex != 0)
                            && (ulTableIndex <= cTable)
                            )
                        {
                            // valid index:
                            // insert stuff from table
                            PSZ            *ppTableThis = (PSZ*)(pTable + (ulTableIndex - 1));

                            // (pTable) if 1, (pTable+1) if 2, ...
                            PSZ             pTableSource = *ppTableThis;

                            // copy string from table
                            // and increase the target pointer
                            while ((*pTarget++ = *pTableSource++))
                                ulTargetWritten++;

                            // increase source pointer to point
                            // behind the "%x"
                            pSource += 2;
                            // decrease target again, because
                            // we've copied a null-byte
                            pTarget--;

                            ulCount++;
                            // next for
                            continue;
                        }
                        // else invalid index: just copy
                    }

                    // regular character: copy
                    *pTarget++ = *pSource++;
                    ulTargetWritten++;
                }                   // end for (ulCount = 0) ...

                ulBytesRead = ulTargetWritten;
            }
        }                           // end if (cTable)

    // report bytes written (*UM)
    if (pcbMsg)
        *pcbMsg = ulBytesRead;

    // cleanup
    if (hfile)
        DosClose(hfile);
    if (pbTableData)
        free(pbTableData);

    return (rc);
}

/*
 *@@ tmfGetMessageExt:
 *      just like tmfGetMessage, but this func is smart
 *      enough to return a zero-terminated string and
 *      strip trailing newlines.
 *
 *      See tmfGetMessage for details.
 */

APIRET tmfGetMessageExt(PCHAR* pTable,    // in: pointer table for string insertion
                        ULONG cTable,     // in: number of pointers in *pTable
                        PBYTE pbBuffer,   // out: returned string
                        ULONG cbBuffer,   // out: sizeof(*pbBuffer)
                        PSZ pszMessageName,  // in: message identifier
                        PSZ pszFile,      // in: message file (.TMF extension proposed)
                        PULONG pcbMsg)    // out: bytes written to *pbBuffer
{
    APIRET  arc = NO_ERROR;
    ULONG   cbReturned = 0;
    arc = tmfGetMessage(pTable, cTable, pbBuffer, cbBuffer, pszMessageName, pszFile,
                        &cbReturned);

    if (arc == NO_ERROR)
    {
        PSZ p = pbBuffer;

        // terminate string
        if (cbReturned < cbBuffer)
            *(pbBuffer+cbReturned) = 0;
        else
            *(pbBuffer+cbBuffer-1) = 0;

        // remove leading spaces (99-10-24) [umoeller]
        while (*p == ' ')
            p++;
        if (p != pbBuffer)
        {
            // we have leading spaces:
            strcpy(pbBuffer, p);
            // decrease byte count
            cbReturned -= (p - pbBuffer);
        }

        // remove trailing newlines
        while (TRUE)
        {
            p = pbBuffer + strlen(pbBuffer) - 1;
            if (    (*p == '\n')
                 || (*p == '\r')
               )
            {
                *p = '\0';
            } else
                break; // while (TRUE)
        }
    }

    if (pcbMsg)
        *pcbMsg = cbReturned;

    return (arc);
}

/*
 *@@ CompileMsgTable:
 *      this gets called from tmfGetMessage.
 *      Here we check for whether the given
 *      message file has been compiled to the
 *      EAs yet or if the compilation is
 *      outdated.
 *
 *      If we need recompilation, we do this here.
 *      Memory for the compiled message table is
 *      allocated here using malloc(), and the
 *      buffer is stored in ppbTableData. You
 *      must free() this buffer after using this
 *      function.
 *
 *      <B>Returns:</B>
 *      --  ERROR_INVALID_PARAMETER
 *      --  ERROR_BUFFER_OVERFLOW
 *      --  ERROR_NOT_ENOUGH_MEMORY
 *      --  ERROR_READ_FAULT
 *      --  ERROR_INVALID_DATA
 *
 *@@changed V0.9.1 (99-12-12) [umoeller]: fixed heap overwrites which caused irregular crashes
 *@@changed V0.9.1 (99-12-12) [umoeller]: fixed realloc failure
 *@@changed V0.9.1 (99-12-12) [umoeller]: file last-write date is now reset when EAs are written
 *@@changed V0.9.1 (2000-02-01) [umoeller]: now skipping leading spaces in msg
 */

APIRET CompileMsgTable(PSZ pszMessageFile,     // in: file to compile
                       PBYTE *ppbTableDataReturn)  // out: compiled message table
{
    APIRET          rc = NO_ERROR;
    // CHAR            szMessageFile[_MAX_PATH];

    FILESTATUS3     fs3MessageFile;

    // fixed UM 99-10-22: initializing all vars to 0
    ULONG           ulStampLength = 0;
    PBYTE           pbFileData = NULL;
    ULONG           ulFileDataLength = 0;

    CHAR            szFileStampOld[18] = "";     // yyyymmddhhmmssms.

    CHAR            szFileStampCurrent[18] = "";

    PBYTE           pbTableData = NULL;
    ULONG           cbTableDataAllocated = 0;
    ULONG           cbTableDataUsed = 0;

    HFILE           hfileMessageFile = NULLHANDLE;
    ULONG           ulAction = 0;
    ULONG           ulBytesRead = 0;

    COUNTRYCODE     cc = {0, 0};

    ULONG           cbOpenTag = strlen(MSG_NAME_START);
    ULONG           cbClosingTag = strlen(MSG_NAME_END);

    do
    {
        PSZ             pCurrentNameStart = 0;
        PSZ             pCurrentNameEnd = 0;
        ULONG           ulCurrentMessagePos = 0;
        ULONG           ulCurrentMessageLen = 0;
        // BOOL            fError = FALSE;

        // check parms
        if ((!pszMessageFile) ||
            (!ppbTableDataReturn))
        {
            rc = ERROR_INVALID_PARAMETER;
            break;
        }

        // get length and timestamp of file
        rc = DosQueryPathInfo(pszMessageFile,
                              FIL_STANDARD,
                              &fs3MessageFile,
                              sizeof(fs3MessageFile));
        if (rc != NO_ERROR)
            break;
        ulFileDataLength = fs3MessageFile.cbFile;

        // determine current timestamp
        GetTimeStamp(&fs3MessageFile, szFileStampCurrent, sizeof(szFileStampCurrent));

        _Pmpf(("    Timestamp for %s: '%s'", pszMessageFile, szFileStampCurrent));

        // determine saved timestamp
        ulStampLength = sizeof(szFileStampOld);
        rc = eahReadStringEA(pszMessageFile,
                          EA_TIMESTAMP,
                          szFileStampOld,
                          &ulStampLength);

        _Pmpf(("    Saved timestamp for %s: '%s'", pszMessageFile, szFileStampOld));

        // compare timestamps
        if ((rc == NO_ERROR)
            && (ulStampLength == (strlen(szFileStampCurrent) + 1))
            && (!strcmp(szFileStampCurrent, szFileStampOld))
            )
        {

            // read table out of EAs
            do
            {
                // get ea length of table
                rc = eahReadStringEA(pszMessageFile,
                                  EA_MSGTABLE,
                                  NULL, &cbTableDataAllocated);
                if (rc != ERROR_BUFFER_OVERFLOW)
                    break;

                // get memory
                if ((pbTableData = (PBYTE)malloc(cbTableDataAllocated)) == NULL)
                {
                    rc = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }

                // read table
                rc = eahReadStringEA(pszMessageFile,
                                  EA_MSGTABLE,
                                  pbTableData, &cbTableDataAllocated);

            }
            while (FALSE);

            // if no error occurred, we are finished
            if (rc == NO_ERROR)
            {
                _Pmpf(("      --> using precompiled table"));
                break;
            }
        } // end if

        _Pmpf(("      --> recompiling table"));

        // recompilation needed:
        // get memory for file data
        if ((pbFileData = (PBYTE)malloc(ulFileDataLength + 1)) == NULL)
        {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        *(pbFileData + ulFileDataLength) = 0;

        // get memory for table data
        cbTableDataAllocated = ulFileDataLength / 2;
        if ((pbTableData = (PBYTE)malloc(cbTableDataAllocated)) == NULL)
        {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        *pbTableData = 0;
                // fixed V0.9.1 (99-12-12);
                // this was
                //      *(pbTableData + cbTableDataAllocated) = 0;
                // since we're using "strcat" below to write into
                // the table data, this wasn't such a good idea

        // open file and read it
        rc = DosOpen(pszMessageFile,
                     &hfileMessageFile,
                     &ulAction,
                     0, 0,
                     OPEN_ACTION_FAIL_IF_NEW
                     | OPEN_ACTION_OPEN_IF_EXISTS,
                     OPEN_FLAGS_FAIL_ON_ERROR
                     | OPEN_SHARE_DENYWRITE
                     | OPEN_ACCESS_READWRITE,   // needed for EA attachement
                      NULL);
        if (rc != NO_ERROR)
            break;

        rc = DosRead(hfileMessageFile,
                     pbFileData,
                     ulFileDataLength,
                     &ulBytesRead);
        if (rc != NO_ERROR)
            break;
        if (ulBytesRead != ulFileDataLength)
        {
            rc = ERROR_READ_FAULT;
            break;
        }

        // search first message name
        // this will automatically skip comments
        // at the beginning
        pCurrentNameStart = strstr(pbFileData, MSG_NAME_START);

        if (!pCurrentNameStart)
        {
            rc = ERROR_INVALID_DATA;
            break;
        }
        else
            pCurrentNameStart += cbOpenTag;
                // this points to message ID string after "<--" now

        // is first name complete ?
        pCurrentNameEnd = strstr(pCurrentNameStart, MSG_NAME_END);
        if (!pCurrentNameEnd)
        {
            rc = ERROR_INVALID_DATA;
            break;
        }

        // scan through all names
        while (     (pCurrentNameStart)
                 && (*pCurrentNameStart)
              )
        {
            PSZ     pNextNameStart = 0,
                    pNextCommentStart = 0,
                    pEndOfMsg = 0;
            CHAR    szEntry[500] = "";

            // search end of name, if not exist, skip end of file
            pCurrentNameEnd = strstr(pCurrentNameStart, MSG_NAME_END);
                // this points to the closing "-->" now
            if (!pCurrentNameEnd)
                break;

            // search next name (ID), if none, use end of string
            pNextNameStart = strstr(pCurrentNameEnd, MSG_NAME_START);
                    // points to next "<--" now
            if (!pNextNameStart)
                // not found:
                // use terminating null byte
                pNextNameStart = pCurrentNameStart + strlen(pCurrentNameStart);
            else
                // found:
                // point to next message ID string
                pNextNameStart += cbOpenTag;

            pEndOfMsg = pNextNameStart - cbOpenTag;

            // search next comment (99-11-28) [umoeller]
            pNextCommentStart = strstr(pCurrentNameEnd, MSG_COMMENT_LINE);
            if (pNextCommentStart)
                // another comment found:
                if (pNextCommentStart < pNextNameStart)
                    // if it's even before the next key, use that
                    // for the end of the current string; otherwise
                    // the comment would appear in the message too
                    pEndOfMsg = pNextCommentStart;

            // now we have:
            // -- pCurrentNameStart: points to message ID after "<--"
            // -- pCurrentNameEnd: points to "-->"
            // -- pNextNameStart: points to _next_ message ID after "<--"
            // -- pNextCommentStart: points to _next_ comment
            // -- pEndOfMsg: either pNextNameStart-cbOpenTag
            //               or pNextCommentStart,
            //               whichever comes first

            // calculate table entry data
            *pCurrentNameEnd = 0;
            ulCurrentMessagePos = (pCurrentNameEnd - pbFileData) + cbClosingTag;
            // this points to the first character after the <-- --> tag now;
            // if this points to a space character, take next char
            // (V0.9.1 (2000-02-13) [umoeller])
            while (     (*(pbFileData + ulCurrentMessagePos))
                     && (*(pbFileData + ulCurrentMessagePos) == ' ')
                  )
                ulCurrentMessagePos++;

            ulCurrentMessageLen = (pEndOfMsg - pbFileData) - ulCurrentMessagePos;
                    // pszNextNameStart - pbFileData - ulCurrentMessagePos - 1;

            // determine entry
            sprintf(szEntry, "%s %lu %lu" NEWLINE,
                    pCurrentNameStart,
                    ulCurrentMessagePos,
                    ulCurrentMessageLen);

            _Pmpf(("Found %s at %d, length %d",
                    pCurrentNameStart,
                    ulCurrentMessagePos,
                    ulCurrentMessageLen));

            // need more space ?
            if ((cbTableDataUsed + strlen(szEntry) + 1) > cbTableDataAllocated)
            {
                PBYTE           pbTmp;
                _Pmpf(("  Re-allocating!!"));

                cbTableDataAllocated += ulFileDataLength / 2;
                pbTmp = (PBYTE)realloc(pbTableData, cbTableDataAllocated);
                if (!pbTmp)
                {
                    rc = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
                else
                    pbTableData = pbTmp;
            }

            // add entry
            strcat(pbTableData, szEntry);
            cbTableDataUsed += strlen(szEntry);
                    // added V0.9.1 (99-12-12);
                    // this was 0 all the time and never raised

            // adress next entry
            pCurrentNameStart = pNextNameStart;

        }  // while (pszCurrentNameStart)

        // write new timestamp and table
        // ### handle 64 kb limit here !!!
        if (rc == NO_ERROR)
        {
            FILESTATUS3 fs3Tmp;

            rc = eahWriteStringEA(hfileMessageFile, EA_TIMESTAMP, szFileStampCurrent);
            rc = eahWriteStringEA(hfileMessageFile, EA_MSGTABLE, pbTableData);

            if (rc == NO_ERROR)
            {
                // fixed V0.9.1 (99-12-12): we need to reset the file date to
                // the old date,
                // because writing EAs updates the "last write date" also;
                // without this, we'd recompile every single time
                rc = DosQueryFileInfo(hfileMessageFile,
                                      FIL_STANDARD,
                                      &fs3Tmp,
                                      sizeof(fs3Tmp));
                if (rc == NO_ERROR)
                {
                    // restore original "last write" date
                    _Pmpf(("Resetting file date"));
                    fs3Tmp.fdateLastWrite = fs3MessageFile.fdateLastWrite;
                    fs3Tmp.ftimeLastWrite = fs3MessageFile.ftimeLastWrite;
                    DosSetFileInfo(hfileMessageFile,
                                   FIL_STANDARD,
                                   &fs3Tmp,
                                   sizeof(fs3Tmp));
                }
            }
        }
    } while (FALSE);


    if (rc == NO_ERROR)
    {
        // hand over result
        *ppbTableDataReturn = pbTableData;

        // make text uppercase
        rc = DosMapCase(cbTableDataAllocated, &cc, pbTableData);
    }
    else
    {
        // error occured:
        if (pbTableData)
            free(pbTableData);
    }

    // cleanup
    if (pbFileData)
        free(pbFileData);
    if (hfileMessageFile)
        DosClose(hfileMessageFile);

    return rc;
}

/*
 * GetTimeStamp:
 *      this returns the "last write date" timestamp
 *      of the file specified in pfs3.
 *
 *      <B>Usage:</B> called from CompileMsgTable
 *      to check whether the compilation is
 *      outdated.
 *
 *      Returns:
 *      --  NO_ERROR
 *      --  ERROR_INVALID_PARAMETER
 *      --  ERROR_BUFFER_OVERFLOW: pszBuffer too small for timestamp
 */

APIRET GetTimeStamp(PFILESTATUS3 pfs3,     // in: file to check
                    PSZ pszBuffer,    // out: timestamp
                    ULONG ulBufferlen)    // in: sizeof(*pszBuffer)
{

    APIRET          rc = NO_ERROR;
    CHAR            szTimeStamp[15];
    static PSZ      pszFormatTimestamp = "%4u%02u%02u%02u%02u%02u%";

    do
    {
        // check parms
        if ((!pfs3) ||
            (!pszBuffer))
        {
            rc = ERROR_INVALID_PARAMETER;
            break;
        }

        // create stamp
        sprintf(szTimeStamp,
                pszFormatTimestamp,
                pfs3->fdateLastWrite.year + 1980,
                pfs3->fdateLastWrite.month,
                pfs3->fdateLastWrite.day,
                pfs3->ftimeLastWrite.hours,
                pfs3->ftimeLastWrite.minutes,
                pfs3->ftimeLastWrite.twosecs * 2);

        // check bufferlen
        if (strlen(szTimeStamp) + 1 > ulBufferlen)
        {
            rc = ERROR_BUFFER_OVERFLOW;
            break;
        }

        // hand over result
        strcpy(pszBuffer, szTimeStamp);
    }
    while (FALSE);

    return rc;
}

