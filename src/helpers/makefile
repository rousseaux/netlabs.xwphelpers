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
#               -- PROJECT_BASE_DIR: where to find setup.in; this should
#                   be the root directory of the project, e.g. "C:\cvs\warpin"
#                   or "C:\cvs\xworkplace"
#               -- HELPERS_OUTPUT_DIR: where to create output files (*.obj, helpers.lib)
#                   this should be a "bin" directory (e.g. "C:\cvs\warpin\bin"
#               -- CC_HELPERS: compiler command line for compiling C files.
#                   With VAC++, this should include the /Ge+ (compile to EXE)
#                   option to allow linking the library to both EXE and DLL
#                   files.
#                   If you're using the "dll" target, specify /Ge- instead.
#               -- MAINMAKERUNNING: if this is NOT defined, this makefile
#                   will recurse to the makefile in $(PROJECT_BASE_DIR).
#                   So to have this makefile run successfully, define this
#                   variable to something.
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

# Define the main dependency between the output HELPERS.LIB and
# all the object files.
# $? represents the names of all dependent files that are
# out-of-date with respect to the target file.
# The exclamation point ( ! ) preceding the LIB command causes NMAKE
# to execute the LIB command once for each dependent file in the list.

$(OUTPUTDIR)\helpers.lib: $(OBJS)
!ifdef EMX
    !emxomfar cr $* $?
!else
    !ilib /nol /nob $* -+$?;
!endif

# same thing for cp.lib
$(OUTPUTDIR)\cp.lib: $(CPOBJS)
!ifdef EMX
    !emxomfar cr $* $?
!else
    !ilib /nol /nob $* -+$?;
!endif

# same thing for plainc.lib
$(OUTPUTDIR)\plainc.lib: $(PLAINCOBJS)
!ifdef EMX
    !emxomfar cr $* $?
!else
    !ilib /nol /nob $* -+$?;
!endif

!include helpers_post.in


