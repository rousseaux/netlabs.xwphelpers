
#
# makefile:
#
#       simple makefile which basically only goes into the
#       src\helpers subdirectory.
#

PROJECT_BASE_DIR = $(MAKEDIR)

INCLUDE = $(PROJECT_BASE_DIR)\include\helpers;$(INCLUDE)

all:
    @cd src\helpers
    nmake -nologo
    @cd ..\..

dep:
    cd src\helpers
    nmake -nologo dep "SUBTARGET=dep" "RUNDEPONLY=1"
    cd ..\..

