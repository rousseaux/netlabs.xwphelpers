XWP Helpers 0.9.6 README
(W) Ulrich M”ller, October 26, 2000
Last updated October 26, 2000, Ulrich M”ller


0. CONTENTS OF THIS FILE
========================

    1. LICENSE, COPYRIGHT, DISCLAIMER
    2. INTRODUCTION
    3. CREATING CODE DOCUMENTATION
    4. COMPILING
    5. INCLUDING HEADER FILES


1. LICENSE, COPYRIGHT, DISCLAIMER
=================================

    Copyright (C) 1997-2000 Ulrich M”ller,
                            Christian Langanke,
                            and others (see the individual source files).

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as contained in
    the file COPYING in this distribution.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.


2. INTRODUCTION
===============

    Welcome to the XWorkplace Helpers.

    This CVS archive is intended to support OS/2 developers with any
    code they might need writing OS/2 programs.

    The XWPHelpers are presently used in XWorkplace and WarpIN. They
    started out from various code snippets I created for XFolder,
    the predecessor of XWorkplace. I then isolated the code which could
    be used independently and put that code into separate directories
    in the WarpIN CVS repository (also at Netlabs).

    At Warpstock Europe 2000 in Karlsruhe, I talked to a number of
    developers and then decided that this should become an independent
    Netlabs CVS archive so that other people can more easily contribute.

    Even though the helpers are called "XWorkplace helpers", they
    have nothing to do with WPS and SOM programming. They can help
    any OS/2 programmer.

    The XWPHelpers offer you frequently used code for writing all
    sorts of OS/2 programs, including:

    --  standard C code which is independent of the OS/2 platform;

    --  OS/2-specific code which can be used in any OS/2 program
        (text mode or PM);

    --  PM-specific code which assists you in writing PM programs.

    The XWPHelpers can be compiled with EMX/GCC or IBM VisualAge
    C++ 3.08. They can be used with C or C++ programs.


    Getting Sources from Netlabs CVS
    --------------------------------

    First set the CVS enviroment:
        CVSROOT=:pserver:guest@www.netlabs.org:d:/netlabs.src/xwphelpers
        USER=guest

    Then, to check out the most current XWPHelpers sources, create
    a subdirectory in your CVS root dir called "xwphelpers".

    Do a "cvs login" with "readonly" as your password and do a
    "cvs checkout ." from the "xwphelpers" subdirectory. Don't forget
    the dot.

    Alternatively, use the Netlabs Open Source Archive Client (NOSAC).
    See http://www.netlabs.org/nosa for details.

    In any case, I strongly recommend to create a file in $(HOME)
    called ".cvsrc" and add "cvs -z9" in there to enable maximum
    compression during transfers. This greatly speeds up things.


3. CREATING CODE DOCUMENTATION
==============================

    The XWPHelpers do not come with pre-made documentation. However,
    you can automatically have documentation generated from the sources
    using my "xdoc" utility, which resides in the main directory of
    the helpers. (The source code for xdoc is in the WarpIN CVS
    repository because it shares some C++ code with WarpIN.)

    To have the code generated, call "createdoc.cmd" in the main
    directory. This will call xdoc in turn with the proper parameters
    and create a new "HTML" directory, from where you should start
    with the "index.html" file.


4. COMPILING
============

    Compiling is a bit tricky because the code and the makefiles
    were designed to be independent of any single project. As a
    result, I had to used environment variables in order to pass
    parameters to the makefiles.

    The most important environment variable is PROJECT_BASE_DIR.
    This should point to the root directory of your own project.
    In this directory, src\helpers\makefile expects a file called
    "setup.in" which sets up more environment variables. You can
    take the one from the XWPHelpers makefile as a template.

    See the top of src\helpers\makefile for additional variables.

    Of course, nothing stops you from writing your own makefile
    if you find all this to complicated. However, if you choose
    to use my makefile from within your own project, you can
    then simply change to the src\helpers directory and start a
    second nmake from your own makefile like this:

        @cd xxx\src\helpers
        nmake -nologo "PROJECT_BASE_DIR=C:\myproject" "MAINMAKERUNNING=YES"
        @cd olddir


5. INCLUDING HEADER FILES
=========================

    The "include policy" of the helpers is that the "include"
    directory in the helpers source tree should be part of your
    include path. This way you can include helper headers in
    your own project code using

        #include "helpers\header.h"

    so that the helpers headers won't interfere with your own
    headers.

    Note that all the helpers C code includes their own include
    files this way. As a result, the XWPHelpers "include"
    directory must be in your include path, or this won't
    compile.

    Besides, the helpers C code expects a file called "setup.h"
    in your include path somewhere. This is included by all
    the C files so you can (re)define certain macros there.


