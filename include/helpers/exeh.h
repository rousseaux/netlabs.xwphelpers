
/*
 *@@sourcefile exeh.h:
 *      header file for exeh.c. See remarks there.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@include #include <os2.h>
 *@@include #include "helpers\dosh.h"
 *@@include #include "helpers\exeh.h"
 */

/*      This file Copyright (C) 2000-2002 Ulrich M”ller,
 *                                        Martin Lafaix.
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

#ifndef EXEH_HEADER_INCLUDED
    #define EXEH_HEADER_INCLUDED

    /********************************************************************
     *
     *   Executable helpers
     *
     ********************************************************************/

    #pragma pack(1)

    /*
     *@@ DOSEXEHEADER:
     *      old DOS EXE header at offset 0
     *      in any EXE file.
     *
     *@@changed V0.9.7 (2000-12-20) [umoeller]: fixed NE offset
     *@@changed V0.9.9 (2001-04-06) [lafaix]: additional fields defined
     */

    typedef struct _DOSEXEHEADER
    {
         USHORT usDosExeID;             // 00: DOS exeid (0x5a4d)
         USHORT usFileLenMod512;        // 02: filelen mod 512
         USHORT usFileLenDiv512;        // 04: filelen div 512
         USHORT usSegFix;               // 06: count of segment adds to fix
         USHORT usHdrPargCnt;           // 08: size of header in paragrphs
         USHORT usMinAddPargCnt;        // 0a: minimum addtl paragraphs count
         USHORT usMaxAddPargCnt;        // 0c: max addtl paragraphs count
         USHORT usSSStartup;            // 0e: SS at startup
         USHORT usSPStartup;            // 10: SP at startup
         USHORT usHdrChecksum;          // 12: header checksum
         USHORT usIPStartup;            // 14: IP at startup
         USHORT usCodeSegOfs;           // 16: code segment offset from EXE start
         USHORT usRelocTableOfs;        // 18: reloc table ofs.header (Win: >= 0x40)
         USHORT usOverlayNo;            // 1a: overlay no.
         USHORT usLinkerVersion;        // 1c: linker version (if 0x18 > 0x28)
         USHORT usUnused1;              // 1e: unused
         USHORT usBehaviorBits;         // 20: exe.h says this field contains
                                        //     'behavior bits'
         USHORT usUnused2;              // 22: unused
         USHORT usOEMIdentifier;        // 24: OEM identifier
         USHORT usOEMInformation;       // 26: OEM information
         ULONG  ulUnused3;              // 28:
         ULONG  ulUnused4;              // 2c:
         ULONG  ulUnused5;              // 30:
         ULONG  ulUnused6;              // 34:
         ULONG  ulUnused7;              // 38:
         ULONG  ulNewHeaderOfs;         // 3c: new header ofs (if 0x18 > 0x40)
                    // fixed this from USHORT, thanks Martin Lafaix
                    // V0.9.7 (2000-12-20) [umoeller]
    } DOSEXEHEADER, *PDOSEXEHEADER;

    // NE and LX OS types
    #define NEOS_UNKNOWN        0
    #define NEOS_OS2            1   // Win16 SDK says: "reserved"...
    #define NEOS_WIN16          2
    #define NEOS_DOS4           3   // Win16 SDK says: "reserved"...
    #define NEOS_WIN386         4   // Win16 SDK says: "reserved"...

    /*
     *@@ NEHEADER:
     *      linear executable (LX) header format,
     *      which comes after the DOS header in the
     *      EXE file (at DOSEXEHEADER.ulNewHeaderOfs).
     *
     *@@changed V0.9.9 (2001-04-06) [lafaix]: fixed typo in usMoveableEntries
     */

    typedef struct _NEHEADER
    {
        CHAR      achNE[2];             // 00: "NE" magic                       ne_magic
        BYTE      bLinkerVersion;       // 02: linker version                   ne_ver
        BYTE      bLinkerRevision;      // 03: linker revision                  ne_rev
        USHORT    usEntryTblOfs;        // 04: ofs from this to entrytable      ne_enttab
        USHORT    usEntryTblLen;        // 06: length of entrytable             ne_cbenttab
        ULONG     ulChecksum;           // 08: MS: reserved, OS/2: checksum     ne_crc
        USHORT    usFlags;              // 0c: flags                            ne_flags
                           /*
                              #define NENOTP          0x8000          // Not a process == library
                              #define NENOTMPSAFE     0x4000          // Process is not multi-processor safe
                                                                      // (Win3.1 SDK: "reserved")
                              #define NEIERR          0x2000          // Errors in image
                              #define NEBOUND         0x0800          // Bound Family/API
                                                                      // (Win3.1 SDK: "first segment contains code
                                                                      // that loads the application")
                              #define NEAPPTYP        0x0700          // Application type mask
                                                                      // (Win3.1 SDK: "reserved")
                              #define NENOTWINCOMPAT  0x0100          // Not compatible with P.M. Windowing
                                                                      // (Win3.1 SDK: "reserved")
                              #define NEWINCOMPAT     0x0200          // Compatible with P.M. Windowing
                                                                      // (Win3.1 SDK: "reserved")
                              #define NEWINAPI        0x0300          // Uses P.M. Windowing API
                                                                      // (Win3.1 SDK: "reserved")
                              #define NEFLTP          0x0080          // Floating-point instructions
                              #define NEI386          0x0040          // 386 instructions
                              #define NEI286          0x0020          // 286 instructions
                              #define NEI086          0x0010          // 8086 instructions
                              #define NEPROT          0x0008          // Runs in protected mode only
                                                                      // (Win3.1 SDK: "reserved")
                              #define NEPPLI          0x0004          // Per-Process Library Initialization
                                                                      // (Win3.1 SDK: "reserved")
                              #define NEINST          0x0002          // Instance data
                              #define NESOLO          0x0001          // Solo data (single data)
                           */
        USHORT    usAutoDataSegNo;      // 0e: auto-data seg no.                ne_autodata
                                        // (Win3.1 SDK: "0 if both NEINST and NESOLO are cleared")
        USHORT    usInitlHeapSize;      // 10: initl. heap size                 ne_heap
                                        // (Win3.1 SDK: "0 if no local allocation")
        USHORT    usInitlStackSize;     // 12: initl. stack size                ne_stack
                                        // (Win3.1 SDK: "0 if SS != DS")
        ULONG     ulCSIP;               // 14: CS:IP                            ne_csip
        ULONG     ulSSSP;               // 18: SS:SP                            ne_sssp
        USHORT    usSegTblEntries;      // 1c: segment tbl entry count          ne_cseg
        USHORT    usModuleTblEntries;   // 1e: module ref. table entry count    ne_cmod
        USHORT    usNonResdTblLen;      // 20: non-resd. name tbl length        ne_cbnrestab
        USHORT    usSegTblOfs;          // 22: segment tbl ofs                  ne_segtab
                                        // (from start of NEHEADER)
        USHORT    usResTblOfs;          // 24: resource tbl ofs                 ne_rsrctab
                                        // (from start of NEHEADER)
        USHORT    usResdNameTblOfs;     // 26: resd. name tbl ofs               ne_restab
                                        // (from start of NEHEADER)
        USHORT    usModRefTblOfs;       // 28: module ref. table ofs            ne_modtab
                                        // (from start of NEHEADER)
        USHORT    usImportTblOfs;       // 2a: import name tbl ofs              ne_imptab
                                        // (from start of NEHEADER)
        ULONG     ulNonResdTblOfs;      // 2c: non-resd. name tbl ofs           ne_nrestab
                                        // (from start of EXE!)
        USHORT    usMoveableEntries;    // 30: moveable entry points count      ne_cmovent
        USHORT    usLogicalSectShift;   // 32: logcl. sector shift              ne_align
                                        // (Win3.1 SDK: "typically 4, but default is 9")
        USHORT    usResSegmCount;       // 34: resource segm. count             ne_cres
        BYTE      bTargetOS;            // 36: target OS (NEOS_* flags)         ne_exetyp
        BYTE      bFlags2;              // 37: addtl. flags                     ne_flagsothers
                                        // Win3.1 SDK:
                                        //      bit 1 --> Win2.x, but runs in Win3.x protected mode
                                        //      bit 2 --> Win2.x that supports prop. fonts
                                        //      bit 3 --> exec contains fastload area
                                /*
                                #define NELONGNAMES     0x01
                                #define NEWINISPROT     0x02
                                #define NEWINGETPROPFON 0x04
                                #define NEWLOAPPL       0x80
                                */
        // the following are not listed in newexe.h, but are documented for Win3.x
        USHORT    usFastLoadOfs;        // 38: fast-load area ofs
        USHORT    usFastLoadLen;        // 3a: fast-load area length
        USHORT    usReserved;           // 3c: MS: 'reserved'
        USHORT    usReqWinVersion;      // 3e: Win-only: min. Win version
    } NEHEADER, *PNEHEADER;

    /*
     *@@ LXHEADER:
     *      linear executable (LX) header format,
     *      which comes after the DOS header in the
     *      EXE file (at DOSEXEHEADER.ulNewHeaderOfs).
     *
     *@@changed V0.9.9 (2001-04-06) [lafaix]: fixed auto data object and ulinstanceDemdCnt
     */

    typedef struct _LXHEADER
    {
        CHAR        achLX[2];           // 00: e32_magic  "LX" or "LE" magic
            // this is "LX" for 32-bit OS/2 programs, but
            // "LE" for MS-DOS progs which use this format
            // (e.g. WINDOWS\SMARTDRV.EXE). IBM says the
            // LE format was never released and superceded
            // by LX... I am unsure what the differences are.
        BYTE      fByteBigEndian;       // 02: e32_border byte ordering (1 = big endian)
        BYTE      fWordBigEndian;       // 03: e32_worder word ordering (1 = big endian)
        ULONG     ulFormatLevel;        // 04: e32_level exe format level (0)
        USHORT    usCPU;                // 08: e32_cpu CPU type
        USHORT    usTargetOS;           // 0a: e32_os OS type (NEOS_* flags)
        ULONG     ulModuleVersion;      // 0c: e32_ver module version
        ULONG     ulFlags;              // 10: e32_mflags module flags
                          /* #define E32NOTP          0x8000L        // Library Module - used as NENOTP
                             #define E32NOLOAD        0x2000L        // Module not Loadable
                             #define E32PMAPI         0x0300L        // Uses PM Windowing API
                             #define E32PMW           0x0200L        // Compatible with PM Windowing
                             #define E32NOPMW         0x0100L        // Incompatible with PM Windowing
                             #define E32NOEXTFIX      0x0020L        // NO External Fixups in .EXE
                             #define E32NOINTFIX      0x0010L        // NO Internal Fixups in .EXE
                             #define E32SYSDLL        0x0008L        // System DLL, Internal Fixups discarded
                             #define E32LIBINIT       0x0004L        // Per-Process Library Initialization
                             #define E32LIBTERM       0x40000000L    // Per-Process Library Termination
                             #define E32APPMASK       0x0300L        // Application Type Mask
                          */
        ULONG     ulPageCount;          // 14: e32_mpages no. of pages in module
        ULONG     ulEIPRelObj;          // 18: e32_startobj obj # for IP
                                        // object to which EIP is relative
        ULONG     ulEIPEntryAddr;       // 1c: e32_eip EIP entry addr
        ULONG     ulESPObj;             // 20: e32_stackobj ESP object
        ULONG     ulESP;                // 24: e32_esp ESP
        ULONG     ulPageSize;           // 28: e32_pagesize page size (4K)
        ULONG     ulPageLeftShift;      // 2c: e32_pageshift page left-shift
        ULONG     ulFixupTblLen;        // 30: e32_fixupsize fixup section total size
        ULONG     ulFixupTblChecksum;   // 34: e32_fixupsum fixup section checksum
        ULONG     ulLoaderLen;          // 38: e32_ldrsize size req. for loader section
        ULONG     ulLoaderChecksum;     // 3c: e32_ldrsum loader section checksum
        ULONG     ulObjTblOfs;          // 40: e32_objtab object table offset
        ULONG     ulObjCount;           // 44: e32_objcnt object count
        ULONG     ulObjPageTblOfs;      // 48: e32_objmap object page table ofs
        ULONG     ulObjIterPagesOfs;    // 4c: e32_itermap object iter pages ofs
        ULONG     ulResTblOfs;          // 50: e32_rsrctab resource table ofs
        ULONG     ulResTblCnt;          // 54: e32_rsrccnt resource entry count
        ULONG     ulResdNameTblOfs;     // 58: e32_restab resident name tbl ofs
        ULONG     ulEntryTblOfs;        // 5c: e32_enttab entry tbl ofs
        ULONG     ulModDirectivesOfs;   // 60: e32_dirtab module directives ofs
        ULONG     ulModDirectivesCnt;   // 64: e32_dircnt module directives count
        ULONG     ulFixupPagePageTblOfs;// 68: e32_fpagetab fixup page tbl ofs
        ULONG     ulFixupRecTblOfs;     // 6c: e32_frectab fixup record tbl ofs
        ULONG     ulImportModTblOfs;    // 70: e32_impmod import modl tbl ofs
        ULONG     ulImportModTblCnt;    // 74: e32_impmodcnt import modl tbl count
        ULONG     ulImportProcTblOfs;   // 78: e32_impproc import proc tbl ofs
        ULONG     ulPerPageCSOfs;       // 7c: e32_pagesum per page checksum ofs
        ULONG     ulDataPagesOfs;       // 80: e32_datapage data pages ofs
        ULONG     ulPreloadPagesCnt;    // 84: e32_preload preload pages count
        ULONG     ulNonResdNameTblOfs;  // 88: e32_nrestab non-resdnt name tbl ofs
        ULONG     ulNonResdNameTblLen;  // 8c: e32_cbnrestab non-resdnt name tbl length
        ULONG     ulNonResdNameTblCS;   // 90: e32_nressum non-res name tbl checksum
        ULONG     ulAutoDataSegObj;     // 94: e32_autodata auto dataseg object
        ULONG     ulDebugOfs;           // 98: e32_debuginfo debug info ofs
        ULONG     ulDebugLen;           // 9c: e32_debuglen debug info length
        ULONG     ulInstancePrelCnt;    // a0: e32_instpreload instance preload count
        ULONG     ulInstanceDemdCnt;    // a0: e32_instdemand instance demand count
        ULONG     ulHeapSize16;         // a8: e32_heapsize heap size (16-bit)
        ULONG     ulStackSize;          // ac: e32_stacksize stack size
        BYTE      e32_res3[20];
                                        // Pad structure to 196 bytes
    } LXHEADER, *PLXHEADER;

    /*
     *@@ PEHEADER:
     *      portable executable (PE) header format,
     *      which comes after the DOS header in the
     *      EXE file (at DOSEXEHEADER.ulNewHeaderOfs).
     *
     *@@added V0.9.10 (2001-04-08) [lafaix]
     */

    typedef struct _PEHEADER
    {
        // standard header
        ULONG     ulSignature;          // 00: 'P', 'E', 0, 0
        USHORT    usCPU;                // 04: CPU type
        USHORT    usObjCount;           // 06: number of object entries
        ULONG     ulTimeDateStamp;      // 08: store the time and date the
                                        //     file was created or modified
                                        //     by the linker
        ULONG     ulReserved1;          // 0c: reserved
        ULONG     ulReserved2;          // 10: reserved
        USHORT    usHeaderSize;         // 14: number of remaining bytes after
                                        //     usImageFlags
        USHORT    usImageFlags;         // 16: flags bits for the image

        // optional header (always present in valid Win32 files)
        USHORT    usReserved3;          // 18: reserved
        USHORT    usLinkerMajor;        // 1a: linker major version number
        USHORT    usLinkerMinor;        // 1c: linker minor version number
        USHORT    usReserved4;          // 1e: reserved
        ULONG     ulReserved5;          // 20: reserved
        ULONG     ulReserved6;          // 24: reserved
        ULONG     ulEntryPointRVA;      // 28: entry point relative virtual address
        ULONG     ulReserved7;          // 2c: reserved
        ULONG     ulReserved8;          // 30: reserved
        ULONG     ulImageBase;          // 34:
        ULONG     ulObjectAlign;        // 38:
        ULONG     ulFileAlign;          // 3c:
        USHORT    usOSMajor;            // 40:
        USHORT    usOSMinor;            // 42:
        USHORT    usUserMajor;          // 44:
        USHORT    usUserMinor;          // 46:
        USHORT    usSubSystemMajor;     // 48:
        USHORT    usSubSystemMinor;     // 4a:
        ULONG     ulReserved9;          // 4c: reserved
        ULONG     ulImageSize;          // 50:
        ULONG     ulHeaderSize;         // 54:
        ULONG     ulFileChecksum;       // 58:
        USHORT    usSubSystem;          // 5c:
        USHORT    usDLLFlags;           // 5e:
        ULONG     ulStackReserveSize;   // 60:
        ULONG     ulStackCommitSize;    // 64:
        ULONG     ulHeapReserveSize;    // 68:
        ULONG     ulHeapCommitSize;     // 6c:
        ULONG     ulReserved10;         // 70: reserved
        ULONG     ulInterestingRVACount;// 74:
        ULONG     aulRVASize[1];        // 78: array of RVA/Size entries
    } PEHEADER, *PPEHEADER;

    // additional LX structures

    /*
     *@@ RESOURCETABLEENTRY:
     *     LX resource table entry.
     *
     *@@added V0.9.16 (2001-12-08) [umoeller]
     */

    typedef struct _RESOURCETABLEENTRY     // rsrc32
    {
        USHORT  type;   // Resource type
        USHORT  name;   // Resource name
        ULONG   cb;     // Resource size
        USHORT  obj;    // Object number
        ULONG   offset; // Offset within object
    } RESOURCETABLEENTRY;

    /*
     *@@ OBJECTTABLEENTRY:
     *     LX object table entry.
     *
     *@@added V0.9.16 (2001-12-08) [umoeller]
     */

    typedef struct _OBJECTTABLEENTRY       // o32_obj
    {
        ULONG   o32_size;     // Object virtual size
        ULONG   o32_base;     // Object base virtual address
        ULONG   o32_flags;    // Attribute flags
        ULONG   o32_pagemap;  // Object page map index
        ULONG   o32_mapsize;  // Number of entries in object page map
        ULONG   o32_reserved; // Reserved
    } OBJECTTABLEENTRY;

    /*
     *@@ OBJECTPAGETABLEENTRY:
     *     LX object _page_ table entry, sometimes
     *     referred to as map entry.
     *
     *@@added V0.9.16 (2001-12-08) [umoeller]
     */

    typedef struct _OBJECTPAGETABLEENTRY   // o32_map
    {
        ULONG   o32_pagedataoffset;     // file offset of page
        USHORT  o32_pagesize;           // # of real bytes of page data
        USHORT  o32_pageflags;          // Per-Page attributes
    } OBJECTPAGETABLEENTRY;

    /*
     *@@ LXITER:
     *      iteration Record format for 'EXEPACK'ed pages.
     *
     *@@added V0.9.16 (2001-12-08) [umoeller]
     */

    typedef struct _LXITER
    {
        USHORT          LX_nIter;            // number of iterations
        USHORT          LX_nBytes;           // number of bytes
        unsigned char   LX_Iterdata;         // iterated data byte(s)
    } LXITER, *PLXITER;

    /*
     *@@ OS2NERESTBLENTRY:
     *      OS/2 NE resource table entry.
     *
     *@@added V0.9.16 (2001-12-08) [umoeller]
     */

    typedef struct _OS2NERESTBLENTRY
    {
        USHORT      usType;
        USHORT      usID;
    } OS2NERESTBLENTRY, *POS2NERESTBLENTRY;

    /*
     *@@ OS2NESEGMENT:
     *      OS/2 NE segment definition.
     *
     *@@added V0.9.16 (2001-12-08) [umoeller]
     */

    typedef struct _OS2NESEGMENT       // New .EXE segment table entry
    {
        USHORT      ns_sector;      // File sector of start of segment
        USHORT      ns_cbseg;       // Number of bytes in file
        USHORT      ns_flags;       // Attribute flags
        USHORT      ns_minalloc;    // Minimum allocation in bytes
    } OS2NESEGMENT, *POS2NESEGMENT;

    #pragma pack()

    // object/segment flags (in NE and LX)
    #define OBJWRITE         0x0002L    // Writeable Object
    #define OBJDISCARD       0x0010L    // Object is Discardable
    #define OBJSHARED        0x0020L    // Object is Shared
    #define OBJPRELOAD       0x0040L    // Object has preload pages

    // resource flags
    #define RNMOVE           0x0010     // Moveable resource
    #define RNPURE           0x0020     // Pure (read-only) resource
    #define RNPRELOAD        0x0040     // Preloaded resource
    #define RNDISCARD        0xF000     // Discard priority level for resource

    // EXE format
    #define EXEFORMAT_OLDDOS        1
    #define EXEFORMAT_NE            2
    #define EXEFORMAT_PE            3
    #define EXEFORMAT_LX            4
    #define EXEFORMAT_TEXT_BATCH    5
    #define EXEFORMAT_TEXT_CMD      6       // REXX or plain OS/2 batch
    #define EXEFORMAT_COM           7       // added V0.9.16 (2002-01-04) [umoeller]

    // target OS (in NE and LX)
    #define EXEOS_DOS3              1
    #define EXEOS_DOS4              2       // there is a flag for this in NE
    #define EXEOS_OS2               3
    #define EXEOS_WIN16             4
    #define EXEOS_WIN386            5       // according to IBM, there are flags
                                            // for this both in NE and LX
    #define EXEOS_WIN32             6

#ifndef __STRIP_DOWN_EXECUTABLE__
// for mini stubs in warpin, which has its own
// implementation of this

    /*
     *@@ EXECUTABLE:
     *      structure used with all the exeh*
     *      functions.
     */

    typedef struct _EXECUTABLE
    {
        // executable opened by doshOpen;
        // still NULLHANDLE for .COM, .BAT, .CMD files
        // (V0.9.16)
        PXFILE          pFile;

        // Most of the following fields are set by
        // exehOpen if NO_ERROR is returned:

        // old DOS EXE header;
        // warning, this is NULL if
        // --   the executable has a new header only
        //      (NOSTUB, V0.9.12);
        // --   for .COM, .BAT, .CMD files (V0.9.16)
        PDOSEXEHEADER   pDosExeHeader;
        ULONG           cbDosExeHeader;

        // New Executable (NE) header, if ulExeFormat == EXEFORMAT_NE
        PNEHEADER       pNEHeader;
        ULONG           cbNEHeader;

        // Linear Executable (LX) header, if ulExeFormat == EXEFORMAT_LX
        PLXHEADER       pLXHeader;
        ULONG           cbLXHeader;

        // Portable Executable (PE) header, if ulExeFormat == EXEFORMAT_PE
        PPEHEADER       pPEHeader;
        ULONG           cbPEHeader;

        // module analysis (always set):
        ULONG           ulExeFormat;
                // one of:
                // EXEFORMAT_OLDDOS        1
                // EXEFORMAT_NE            2
                // EXEFORMAT_PE            3
                // EXEFORMAT_LX            4
                // EXEFORMAT_TEXT_BATCH    5    // for all .BAT files
                // EXEFORMAT_TEXT_CMD      6    // for all .CMD files
                // EXEFORMAT_COM           7    // for all .COM files

        BOOL            fLibrary,           // TRUE if this is a DLL
                        f32Bits;            // TRUE if this a 32-bits module

        ULONG           ulOS;
                // target operating system as flagged in one of
                // the EXE headers; one of:
                // EXEOS_DOS3              1    // always for .BAT, possibly for .COM
                // EXEOS_DOS4              2    // there is a flag for this in NE
                // EXEOS_OS2               3    // always for .CMD, possibly for .COM
                // EXEOS_WIN16             4
                // EXEOS_WIN386            5    // according to IBM, there are flags
                                                // for this both in NE and LX
                // EXEOS_WIN32             6

        // The following fields are only set after
        // an extra call to exehQueryBldLevel:

        PSZ             pszDescription;
                // whole string (first non-res name tbl entry)
        PSZ             pszVendor;
                // vendor substring (if IBM BLDLEVEL format)
        PSZ             pszVersion;
                // version substring (if IBM BLDLEVEL format)

        PSZ             pszInfo;
                // module info substring (if IBM BLDLEVEL format)

        // if pszInfo is extended DESCRIPTION field, the following
        // are set as well:
        PSZ             pszBuildDateTime,
                        pszBuildMachine,
                        pszASD,
                        pszLanguage,
                        pszCountry,
                        pszRevision,
                        pszUnknown,
                        pszFixpak;

        // the following fields are set after exehLoadLXMaps
        BOOL                    fLXMapsLoaded;  // TRUE after good exehLoadLXMaps
        RESOURCETABLEENTRY      *pRsTbl;        // pLXHeader->ulResTblCnt
        OBJECTTABLEENTRY        *pObjTbl;       // pLXHeader->ulObjCount
        OBJECTPAGETABLEENTRY    *pObjPageTbl;   // pLXHeader->ulPageCount

        // the following fields are set after exehLoadOS2NEMaps
        BOOL                    fOS2NEMapsLoaded;   // TRUE after good exehLoadOS2NEMaps
        POS2NERESTBLENTRY       paOS2NEResTblEntry;
        POS2NESEGMENT           paOS2NESegments;
    } EXECUTABLE, *PEXECUTABLE;

    APIRET exehOpen(const char* pcszExecutable,
                    PEXECUTABLE* ppExec);

    APIRET exehQueryBldLevel(PEXECUTABLE pExec);

    /*
     *@@ FSYSMODULE:
     *
     *@@added V0.9.9 (2001-03-11) [lafaix]
     */

    typedef struct _FSYSMODULE
    {
        CHAR achModuleName[256];
    } FSYSMODULE, *PFSYSMODULE;

    APIRET exehQueryImportedModules(PEXECUTABLE pExec,
                                    PFSYSMODULE *ppaModules,
                                    PULONG pcModules);

    APIRET exehFreeImportedModules(PFSYSMODULE paModules);

    /*
     *@@ FSYSFUNCTION:
     *
     *@@added V0.9.9 (2001-03-11) [lafaix]
     */

    typedef struct _FSYSFUNCTION
    {
        ULONG ulOrdinal;
        ULONG ulType;
        CHAR achFunctionName[256];
    } FSYSFUNCTION, *PFSYSFUNCTION;

    APIRET exehQueryExportedFunctions(PEXECUTABLE pExec,
                                      PFSYSFUNCTION *ppaFunctions,
                                      PULONG pcFunctions);

    APIRET exehFreeExportedFunctions(PFSYSFUNCTION paFunctions);

    #define WINRT_CURSOR                1
    #define WINRT_BITMAP                2
    #define WINRT_ICON                  3
    #define WINRT_MENU                  4
    #define WINRT_DIALOG                5
    #define WINRT_STRING                6
    #define WINRT_FONTDIR               7
    #define WINRT_FONT                  8
    #define WINRT_ACCELERATOR           9
    #define WINRT_RCDATA               10

    /*
     *@@ FSYSRESOURCE:
     *
     *@@added V0.9.7 (2000-12-18) [lafaix]
     */

    typedef struct _FSYSRESOURCE
    {
        ULONG ulID;                     // resource ID
        ULONG ulType;                   // resource type
        ULONG ulSize;                   // resource size in bytes
        ULONG ulFlag;                   // resource flags

    } FSYSRESOURCE, *PFSYSRESOURCE;

    APIRET exehQueryResources(PEXECUTABLE pExec,
                              PFSYSRESOURCE *ppaResources,
                              PULONG pcResources);

    APIRET exehFreeResources(PFSYSRESOURCE paResources);

    APIRET exehLoadLXMaps(PEXECUTABLE pExec);

    VOID exehFreeLXMaps(PEXECUTABLE pExec);

    APIRET exehLoadLXResource(PEXECUTABLE pExec,
                              ULONG ulType,
                              ULONG idResource,
                              PBYTE *ppbResData,
                              PULONG pcbResData);

    APIRET exehLoadOS2NEMaps(PEXECUTABLE pExec);

    VOID exehFreeNEMaps(PEXECUTABLE pExec);

    APIRET exehLoadOS2NEResource(PEXECUTABLE pExec,
                                 ULONG ulType,
                                 ULONG idResource,
                                 PBYTE *ppbResData,
                                 PULONG pcbResData);

    APIRET exehClose(PEXECUTABLE *ppExec);

#endif

#endif

#if __cplusplus
}
#endif

