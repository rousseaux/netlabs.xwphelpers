# $Id$

#
# makefile:
#       makefile for src/helpers directory.
#       For use with IBM NMAKE, which comes with the IBM compilers,
#       the Developer's Toolkit, and the DDK.
#
#       This makefile is even more complicated than the other makefiles
#       because the helpers code has been designed to be independent
#       of any single project. For example, the helpers code is used
#       by XWorkplace also. As a result, I had to design a way so that
#       this makefile can produce output in a variable directory, with
#       variable compiler flags, and so on. This is done via environment
#       variables (see below).
#
#       Even worse, with V0.9.5, I chose to create a shared runtime DLL
#       for the various WarpIN executables. For that, I chose to use
#       a separate makefile (makefile_dll). In order to share make
#       definitions, these have been put into separate .in files in
#       this directory, which are included via nmake !include.
#
#       Called from:    main makefile
#
#       Required environment variables:
#
#               -- PROJECT_BASE_DIR: where to find setup.in; this should
#                   be the root directory of the project, e.g. "C:\cvs\warpin"
#                   or "C:\cvs\xworkplace"
#
#               -- HELPERS_OUTPUT_DIR: where to create output files (*.obj, helpers.lib)
#                   this should be a "bin" directory (e.g. "C:\cvs\warpin\bin"
#
#               -- CC_HELPERS: compiler command line for compiling C files.
#                   With VAC++, this should include the /Ge+ (compile to EXE)
#                   option to allow linking the library to both EXE and DLL
#                   files.
#                   If you're using the "dll" target, specify /Ge- instead.
#
#               -- MAINMAKERUNNING: if this is NOT defined, this makefile
#                   will recurse to the makefile in $(PROJECT_BASE_DIR).
#                   So to have this makefile run successfully, define this
#                   variable to something.
#
#                   This variable was introduced to be able to start a build
#                   process from src\helpers as well. However, when your own
#                   project makefile calls src\helpers\makefile, you must set
#                   this to something.
#
#       Input:          ./*.c
#
#       Targets:        specify the target(s) to be made, which can be:
#
#                       --  "all" (default): create helpers.lib in addition
#                           to all the output .obj files in $(HELPERS_OUTPUT_DIR).
#
#                           This contains all helpers (plain C, control program,
#                           and PM).
#
#                           This makes linking a bit easier since you don't have to
#                           keep in mind the millions of object files. Still, you
#                           should be sure to include the proper headers in your
#                           code.
#
#                           Alternatively, you can call this makefile with certain
#                           targets explicitly specified. However, you must then
#                           make sure that the resulted object files are linked
#                           properly, because some of the more advanced helpers
#                           require other helpers.
#
#                       --  "plainc": create "plainc.lib", which contains
#                           platform-independent helpers code only (no control
#                           program helpers, no PM helpers).
#
#                           This is included if you specify "all". Use this if
#                           you want a subset of the helpers only.
#
#                       --  "cp": create "cp.lib", which contains "plainc" plus
#                           control program helpers.
#
#                           This is included if you specify "all". Use this if
#                           you want a subset of the helpers only.
#
#       Edit "setup.in" to set up the make process.
#

# Say hello to yourself.
!if [@echo +++++ Entering $(MAKEDIR)\makefile]
!endif

# helpers_pre.in: sets up more environment variables
# and defines $(OBJ), which contains all object targets
!include helpers_pre.in

# The main target:
# If we're called from the main makefile, MAINMAKERUNNING is defined,
# and we'll set $(OBJS) as our targets (which will go on).
# Otherwise, we call the main makefile, which will again call ourselves later.

all:   \
!ifndef MAINMAKERUNNING
    callmainmake
    @echo ----- Leaving $(MAKEDIR)
!else
    $(OUTPUTDIR)\helpers.lib
#$(OBJS)
    @echo ----- Leaving $(MAKEDIR)
!endif

plainc:   \
!ifndef MAINMAKERUNNING
    callmainmake
    @echo ----- Leaving $(MAKEDIR)
!else
    $(OUTPUTDIR)\plainc.lib
#$(OBJS)
    @echo ----- Leaving $(MAKEDIR)
!endif

cp:   \
!ifndef MAINMAKERUNNING
    callmainmake
    @echo ----- Leaving $(MAKEDIR)
!else
    $(OUTPUTDIR)\cp.lib
#$(OBJS)
    @echo ----- Leaving $(MAKEDIR)
!endif


callmainmake:
    @echo $(MAKEDIR)\makefile: Recursing to main makefile.
    @cd $(PROJECT_BASE_DIR)
    @nmake
    @echo $(MAKEDIR)\makefile: Returned from main makefile. Done.

# The "dep" target: run fastdep on the sources.
# "nmake dep" gets called from src\makefile if nmake dep
# is running on the main makefile.
dep:
    $(RUN_FASTDEP) *.c
    @echo ----- Leaving $(MAKEDIR)

# Define the main dependency between the output HELPERS.LIB and
# all the object files.
# $? represents the names of all dependent files that are
# out-of-date with respect to the target file.
# The exclamation point ( ! ) preceding the LIB command causes NMAKE
# to execute the LIB command once for each dependent file in the list.

$(OUTPUTDIR)\helpers.lib: $(OBJS) makefile
!ifdef EMX
    !emxomfar cr $* $?
!else
    - del $@
    ilib /nol /nob $@ @<<$(TEMP)\ilib.lnk
+$(OBJS: =&^
);
<<KEEP
!endif

# same thing for cp.lib
$(OUTPUTDIR)\cp.lib: $(CPOBJS) makefile
!ifdef EMX
    !emxomfar cr $* $?
!else
    - del $@
    ilib /nol /nob $@ @<<
+$(CPOBJS: =&^
);
<<KEEP
!endif

# same thing for plainc.lib
$(OUTPUTDIR)\plainc.lib: $(PLAINCOBJS) makefile
!ifdef EMX
    !emxomfar cr $* $?
!else
    - del $@
    ilib /nol /nob $@ @<<
+$(PLAINCOBJS: =&^
);
<<KEEP
!endif

!include helpers_post.in

###################################################
#
# "test" target: for test cases
#
###################################################

TESTCASE_DIR = testcase

DEBUGDIALOG =
!ifdef DBGDLG
DEBUGDIALOG = /DDEBUG_DIALOG_WINDOWS=1
!endif

TESTCASE_CC = icc /c /ti+ /w2 /ss /se /D__DEBUG__=1 $(DEBUGDIALOG) /i$(HELPERS_BASE)\include /Fo$(TESTCASE_DIR)\$(@B).obj $(@B).c

.c.{$(TESTCASE_DIR)}.obj:
    @echo $(MAKEDIR)\makefile: Compiling $(@B).c
    @echo INCLUDE is $(INCLUDE)
    $(TESTCASE_CC)

# testcase executables
TESTCASE_TARGETS = \
    dosh.exe \
    dialog.exe \
    exeh.exe \
    fdlg.exe \
    xmap.exe \
    vcard.exe

# dialog.exe
DIALOG_TEST_OBJS = \
    $(TESTCASE_DIR)\dialog.obj \
    $(TESTCASE_DIR)\_test_dialog.obj \
    $(TESTCASE_DIR)\winh.obj \
    $(TESTCASE_DIR)\xstring.obj \
    $(TESTCASE_DIR)\linklist.obj \
    $(TESTCASE_DIR)\cctl_checkcnr.obj \
    $(TESTCASE_DIR)\cnrh.obj \
    $(TESTCASE_DIR)\comctl.obj \
    $(TESTCASE_DIR)\stringh.obj \
    $(TESTCASE_DIR)\dosh.obj \
    $(TESTCASE_DIR)\except.obj \
    $(TESTCASE_DIR)\debug.obj \
    $(TESTCASE_DIR)\textview.obj \
    $(TESTCASE_DIR)\textv_html.obj \
    $(TESTCASE_DIR)\tmsgfile.obj \
    $(TESTCASE_DIR)\datetime.obj \
    $(TESTCASE_DIR)\tree.obj \
    $(TESTCASE_DIR)\gpih.obj

$(TESTCASE_DIR)\dialog.obj: ..\..\include\helpers\dialog.h
$(TESTCASE_DIR)\_test_dialog.obj: ..\..\include\helpers\dialog.h

dialog.exe: $(DIALOG_TEST_OBJS)
    ilink /debug /optfunc /pmtype:pm $(DIALOG_TEST_OBJS) pmprintf.lib /o:$@

# dosh.exe
DOSH_TEST_OBJS = \
    $(TESTCASE_DIR)\dosh.obj \
    $(TESTCASE_DIR)\_test_dosh.obj

dosh.exe: $(DOSH_TEST_OBJS)
    ilink /debug /optfunc /pmtype:vio $(DOSH_TEST_OBJS) pmprintf.lib /o:$@

# exeh.exe
EXEH_TEST_OBJS = \
    $(TESTCASE_DIR)\dosh.obj \
    $(TESTCASE_DIR)\exeh.obj \
    $(TESTCASE_DIR)\stringh.obj \
    $(TESTCASE_DIR)\xstring.obj \
    $(TESTCASE_DIR)\_test_exeh.obj

exeh.exe: $(EXEH_TEST_OBJS)
    ilink /debug /optfunc /pmtype:vio $(EXEH_TEST_OBJS) pmprintf.lib /o:$@

# fdlg.exe
FDLG_TEST_OBJS = \
    $(TESTCASE_DIR)\comctl.obj \
    $(TESTCASE_DIR)\cctl_toolbar.obj \
    $(TESTCASE_DIR)\cctl_tooltip.obj \
    $(TESTCASE_DIR)\cctl_xframe.obj \
    $(TESTCASE_DIR)\dosh.obj \
    $(TESTCASE_DIR)\gpih.obj \
    $(TESTCASE_DIR)\linklist.obj \
    $(TESTCASE_DIR)\winh.obj \
    $(TESTCASE_DIR)\stringh.obj \
    $(TESTCASE_DIR)\xstring.obj \
    $(TESTCASE_DIR)\_call_filedlg.obj

fdlg.exe: $(FDLG_TEST_OBJS)
    ilink /debug /pmtype:pm $(FDLG_TEST_OBJS) pmprintf.lib /o:$@

# xmap.exe
MAP_TEST_OBJS = \
    $(TESTCASE_DIR)\map_vac.obj \
    $(TESTCASE_DIR)\linklist.obj \
    $(TESTCASE_DIR)\stringh.obj \
    $(TESTCASE_DIR)\xstring.obj \
    $(TESTCASE_DIR)\_test_map.obj

xmap.exe: $(MAP_TEST_OBJS)
    ilink /debug /optfunc /pmtype:vio $(MAP_TEST_OBJS) pmprintf.lib /o:$@

# vcard.exe
VCARD_TEST_OBJS = \
    $(TESTCASE_DIR)\vcard.obj \
    $(TESTCASE_DIR)\_test_vcard.obj \
    $(TESTCASE_DIR)\xstring.obj \
    $(TESTCASE_DIR)\stringh.obj \
    $(TESTCASE_DIR)\linklist.obj \
    $(TESTCASE_DIR)\dosh.obj \
    $(TESTCASE_DIR)\prfh.obj \
    $(TESTCASE_DIR)\nls.obj \
    $(TESTCASE_DIR)\except.obj \
    $(TESTCASE_DIR)\debug.obj \
    $(TESTCASE_DIR)\tree.obj

vcard.exe: $(VCARD_TEST_OBJS)
    ilink /debug /pmtype:vio $(VCARD_TEST_OBJS) /o:$@

test: $(TESTCASE_TARGETS)


