/* $Id$ */


/*
 *@@sourcefile syssound.h:
 *      header file for syssound.c. See remarks there.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@include #define INCL_WINSHELLDATA
 *@@include #include <os2.h>
 *@@include #include "syssound.h"
 */

/*      Copyright (C) 1999-2000 Ulrich M�ller.
 *      This file is part of the XWorkplace source package.
 *      XWorkplace is free software; you can redistribute it and/or modify
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

#ifndef SYSSOUND_HEADER_INCLUDED
    #define SYSSOUND_HEADER_INCLUDED

    /********************************************************************
     *                                                                  *
     *   Declarations                                                   *
     *                                                                  *
     ********************************************************************/

    /* keys for accessing sounds in MMPM.INI */
    #define MMINIKEY_SYSSOUNDS      "MMPM2_AlarmSounds"
    #define MMINIKEY_SOUNDSETTINGS  "MMPM2_AlarmSoundsData"

    // system sound indices in MMPM.INI
    #define MMSOUND_WARNING         0
    #define MMSOUND_INFORMATION     1
    #define MMSOUND_LOCKUP          10
    #define MMSOUND_ALARMCLOCK      11
    #define MMSOUND_PRINTERROR      12
    #define MMSOUND_ERROR           2
    #define MMSOUND_ANIMATEOPEN     3
    #define MMSOUND_ANIMATECLOSE    4
    #define MMSOUND_DRAG            5
    #define MMSOUND_DROP            6
    #define MMSOUND_SYSTEMSTARTUP   7
    #define MMSOUND_SHUTDOWN        8
    #define MMSOUND_SHREDDER        9

    // sound schemes INI keys in OS2SYS.INI
    #define MMINIKEY_SOUNDSCHEMES   "PM_SOUND_SCHEMES_LIST"

    /********************************************************************
     *                                                                  *
     *   Function prototypes                                            *
     *                                                                  *
     ********************************************************************/

    ULONG sndParseSoundData(PSZ pszSoundData,
                            PSZ pszDescr,
                            PSZ pszFile,
                            PULONG pulVolume);

    HINI sndOpenMmpmIni(HAB hab);

    BOOL sndQuerySystemSound(HAB hab,
                             USHORT usIndex,
                             PSZ pszDescr,
                             PSZ pszFile,
                             PULONG pulVolume);

    BOOL sndWriteSoundData(HINI hiniMMPM,
                           USHORT usIndex,
                           PSZ pszDescr,
                           PSZ pszFile,
                           ULONG ulVolume);

    BOOL sndSetSystemSound(HAB hab,
                           USHORT usIndex,
                           PSZ pszDescr,
                           PSZ pszFile,
                           ULONG ulVolume);

    BOOL sndDoesSchemeExist(PSZ pszScheme);

    APIRET sndCreateSoundScheme(HINI hiniMMPM,
                                PSZ pszNewScheme);

    APIRET sndLoadSoundScheme(HINI hiniMMPM,
                              PSZ pszScheme);

    APIRET sndDestroySoundScheme(PSZ pszScheme);

#endif

#if __cplusplus
}
#endif
