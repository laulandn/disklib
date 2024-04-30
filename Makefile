
LIBNAME=$(shell basename $(shell pwd))
LDNAME=lib$(LIBNAME)_static.a

####################################################################
# Makefile
####################################################################

PATH_TO_BASE=.

include $(PATH_TO_BASE)/../Makefile.target
include $(PATH_TO_BASE)/../Makefile.common

#EXTRA_FLAGS=-DNIK_SDL=1 -DNIK_SDL_DRAWING=1 -DNICK_USE_DECODE3D=1

EXTRA_FLAGS=-I$(PATH_TO_BASE)
EXTRA_LDFLAGS=-L$(PATH_TO_BASE)


####################################################################
# Our objects 
####################################################################

DIRS=$(wildcard */)
SRCSCPP=$(wildcard *.cpp)
SRCSC=$(wildcard *.c)
OBJSCPP=$(patsubst %.cpp, %.o,$(SRCSCPP))
OBJSC=$(patsubst %.c, %.o,$(SRCSC))
OBJS=$(OBJSCPP) $(OBJSC)


# NOTE: Need add .o's if new .cpp files are created!
# NOTE: We use this instead of just OBJS because we don't want disklib_test.o put in the library

LDOBJS=DAcornFS.o DApple2FS.o DAppleDos3FS.o DAppleProDosFS.o DAtariFS.o DCbmFS.o DCpmFS.o DDirEntry.o DDisk.o DDiskImage.o DDiskImageFile.o DDiskImageMem.o DFatFS.o DFinderDatEntry.o DLocalDrive.o DMacFS.o DPartitionTable.o DPartitionTableApple.o DPartitionTableMbr.o DSmartDisk.o DTrs80FS.o Dabasics.o


####################################################################
# The libraries and programs 
####################################################################

SIMPLE_PROGRAMS1=disklib_test
SIMPLE_PROGRAMS2=
SIMPLE_PROGRAMS=$(SIMPLE_PROGRAMS1) $(SIMPLE_PROGRAMS2)
PROGRAMS=$(SIMPLE_PROGRAMS)


# Need for alib and guilib (if using it), for now both are assumed!
PATH_TO_CPP=../cpp
#
CPPLIB_NAME=cpp_static
CPPLIB_L=-L$(PATH_TO_CPP) -l$(CPPLIB_NAME)
CPPLIB_LIB=lib$(CPPLIB_NAME).$(AFILE)
CPPLIB_PATH=$(PATH_TO_CPP)/$(CPPLIB_LIB)
#
CPP_PATHS=$(CPPLIB_PATH)
CPP_LS=$(CPPLIB_L)


# NOTE: mini library meant to replace guilib!
#PATH_TO_NIKLIB=../niklib
#
#NIKLIB_NAME=niklib_static
#NIKLIB_L=-L$(PATH_TO_NIKLIB) -l$(NIKLIB_NAME)
#NIKLIB_LIB=lib$(NIKLIB_NAME).$(AFILE)
#NIKLIB_PATH=$(PATH_TO_NIKLIB)/$(NIKLIB_LIB)


# only needed if NICK_USE_GUILIB
ALIB_NAME=alib_static
ALIB_L=-L$(PATH_TO_BASE)/../cpp/alib -l$(ALIB_NAME)
ALIB_LIB=lib$(ALIB_NAME).$(AFILE)
ALIB_PATH=$(PATH_TO_BASE)/../cpp/alib/$(ALIB_LIB)

# only needed if NICK_USE_GUILIB
#GUILIB_NAME=guilib_static
#GUILIB_L=-L$(PATH_TO_BASE)/../cpp/guilib -l$(GUILIB_NAME)
#GUILIB_LIB=lib$(GUILIB_NAME).$(AFILE)
#GUILIB_PATH=$(PATH_TO_BASE)/../cpp/guilib/$(GUILIB_LIB)

# (should be) only needed if NICK_USE_DECODEIMG
#DECODEIMG_NAME=decodeimg_static
#DECODEIMG_L=-L$(PATH_TO_BASE)/../cpp/decodeimg -l$(DECODEIMG_NAME)
#DECODEIMG_LIB=lib$(DECODEIMG_NAME).$(AFILE)
#DECODEIMG_PATH=$(PATH_TO_BASE)/../cpp/decodeimg/$(DECODEIMG_LIB)

# (should be) only needed if NICK_USE_DECODEIMG
#DECODERLIB_NAME=decoderlib_static
#DECODERLIB_L=-L$(PATH_TO_BASE)/../cpp/decoderlib -l$(DECODERLIB_NAME)
#DECODERLIB_LIB=lib$(DECODERLIB_NAME).$(AFILE)
#DECODERLIB_PATH=$(PATH_TO_BASE)/../cpp/decoderlib/$(DECODERLIB_LIB)


ifeq ($(MSVC),yes)
endif


####################################################################
# The targets 
####################################################################

all:	$(OBJS)
	$(foreach DIR,$(DIRS),$(MAKE) -i -C $(DIR) all;)
	$(MAKE) $(LDNAME)
	$(MAKE) -i $(PROGRAMS)


all_libs:


$(LDNAME):	$(LDOBJS)
	$(AR_CMD) $(LDOBJS)
	$(RANLIB_CMD) $(LDNAME) 


clean:
	rm -rf *.$(OFILE) *.$(AFILE) *.s *.bak *.log map.f build *Data *.pdb $(PROGRAMS) *.exe *.exe.so *68k *.SYM .DS_Store *.ilk .gdb_history .nautilus-metafile.xml resource.frk RESOURCE.FRK finder.dat FINDER.DAT ._* */._* */*/._* */*/*/._* *~ */*~ */*/*~ */*/*/*~ *.class *.javap *.java.* *.h.? tmp/*
# Libraries...
	$(foreach DIR,$(DIRS),$(MAKE) -i -C $(DIR) clean;)


fix: fix_perm fix_type
# Libraries...
	#$(foreach DIR,$(DIRS),$(MAKE) -i -C $(DIR) fix;)


# The different Libraries and Programs...

disklib_test:	$(NIKLIB_PATH) disklib_test.$(OFILE) $(LDNAME)
	$(CLXX) $(LDFLAGS) -o disklib_test disklib_test.$(OFILE) ../../more_src/nlib/nlibdbug.o$(NIKLIB_L) $(CPP_PATHS)  $(LIBS) -ldisklib_static


# dependencies
# NOTE: it's ok to see errors when running makedepend
depend:
	makedepend $(DEPENDFLAGS) *.cpp
# Libraries...
	$(foreach DIR,$(DIRS),$(MAKE) -i -C $(DIR) depend;)


remove_depends:
	rm -f Makefile.dep
	touch Makefile.dep
# Libraries...
	$(foreach DIR,$(DIRS),$(MAKE) -ik -C $(DIR) remove_depends;)


include $(PATH_TO_BASE)/Makefile.tail

include Makefile.dep

# DO NOT DELETE
