
# =============================================================================
#	Choose RMAC or VASM as preferred assembler
# =============================================================================
USE_ASSEMBLER=rmac
# -----------------------------------------------------------------------------
# Notes:
#	Tested with RMAC v1.4.17
#	Tested with VASM v1.6c
# -----------------------------------------------------------------------------

# =============================================================================
#	Use minimal CRT & library code (i.e. avoiding MiNTlib)
# =============================================================================
LINK_MINIMAL=yes
# -----------------------------------------------------------------------------
# Notes:
#	Prefer 'yes' for final optimized builds - saves a lot of space but
#	some C library functionality will be missing. Implements only bare
#	minimum functionality.
# -----------------------------------------------------------------------------

# =============================================================================
#	Pack final executable
# =============================================================================
USE_UPX_PACKER=no
# -----------------------------------------------------------------------------
# Notes:
#	Tested with UPX 3.91w but any recent version should do
# -----------------------------------------------------------------------------

# =============================================================================
#	AGT project root dir
# =============================================================================
AGTROOT = ../..

# =============================================================================
#	Emulator testing (via GEMDOS share)
# =============================================================================
TEST_EMULATOR=yes
EMULATOR_GEMDOS_DIR = .
# -----------------------------------------------------------------------------
# Notes:
#	Bundles .prg into $(EMULATOR_GEMDOS_DIR)/AUTO for easy testing
# Requires:
#	Hatari/STEEM
#	STEEM SSE 3.82B or later for STF synchshift tricks
# -----------------------------------------------------------------------------

# =============================================================================
#	Emulator (Hatari) debugger & profiler support
# =============================================================================
# Hatari base path
HATARI_DEBUG_DIR = /cygdrive/d/projects/local/hatari19
# pipe printf messages to Hatari debugger via natfeats
USE_HATARI_NATFEATS=no
# produce symbols for Hatari debugger
USE_HATARI_DEBUGGING=yes
# -----------------------------------------------------------------------------
# Requires:
# -----------------------------------------------------------------------------
#	Hatari & support tools install
# -----------------------------------------------------------------------------


# =============================================================================
#	Some standard C/ASM config - probably won't need changed too often
# =============================================================================

TARGETFLAGS =	-m68000

LIBPATHS =	\
		-L./3rdparty/m68k/

LIBS =		\

INCPATHS =	-I$(AGTROOT) -I3rdparty/

ASMROOT = $(USE_ASSEMBLER)

# -----------------------------------------------------------------------------
#	With LINK_MINIMAL, we modify the linker command to use thin CRT
# -----------------------------------------------------------------------------
ifeq ($(LINK_MINIMAL),yes)
# -----------------------------------------------------------------------------
COMPILERDEFS += -DUSE_TINY_CRT
LDFLAGS = -Wl,-Map,$*.map -Wl,--gc-sections -nostdlib -nostartfiles
AGLIB_MINICRT = $(AGTROOT)/$(ASMROOT)/mStartupAsmCpp.o $(AGTROOT)/agtsys/mStartup.o
# -----------------------------------------------------------------------------
else
# -----------------------------------------------------------------------------
LDFLAGS = -Wl,--stack,$(STACKSIZE) -Wl,-Map,$*.map -Wl,--gc-sections
AGLIB_MINICRT =
# -----------------------------------------------------------------------------
endif
# -----------------------------------------------------------------------------

VASM = $(AGTROOT)/bin/vasm16c.exe -Faout -showcrit -noesc -I$(AGTROOT)/vasm -I$(AGTROOT)/
RMAC = $(AGTROOT)/bin/rmac.exe -fa +oall -i"$(AGTROOT)/rmac;$(AGTROOT)"

TOOLCHAIN_PREFIX = m68k-atari-mint-

CC = $(TOOLCHAIN_PREFIX)gcc
CXX = $(TOOLCHAIN_PREFIX)g++
AR = $(TOOLCHAIN_PREFIX)ar
NM = $(TOOLCHAIN_PREFIX)nm
LINK = $(TOOLCHAIN_PREFIX)g++
STRIP = $(TOOLCHAIN_PREFIX)strip
STACK = $(TOOLCHAIN_PREFIX)stack
FLAGS = $(TOOLCHAIN_PREFIX)flags

STACKSIZE = 16384


# =============================================================================
#	Projects start here!
# =============================================================================


COMPILERDEFS += \
		-D__ATARI__ -D__M68000__ \
		-DAGT_CONFIG_STACK=$(STACKSIZE) \
		-DAGT_CONFIG_WORLD_XMAJOR \

#		-DUSE_MEMTRACE \

# -----------------------------------------------------------------------------
#	Tweaks
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#	Additional dev/debug options
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#	C Code generation
# -----------------------------------------------------------------------------

# release
CODEGENFLAGS = $(TARGETFLAGS) -Ofast -fomit-frame-pointer -fstrict-aliasing -fcaller-saves
# debug
#CODEGENFLAGS = $(TARGETFLAGS) -g -O0

CFLAGS	= $(CODEGENFLAGS) $(COMPILERDEFS) $(INCPATHS) -Wall
CXXFLAGS = $(CODEGENFLAGS) $(COMPILERDEFS) $(INCPATHS) -x c++ -std=c++0x -fno-rtti -fno-exceptions -fno-threadsafe-statics -Wall -Wno-reorder



# -----------------------------------------------------------------------------
#	AGLIB objects
# -----------------------------------------------------------------------------


AGLIB_O =	\
		$(AGTROOT)/agtsys/system.o \
		$(AGTROOT)/agtsys/shifter.o \
		$(AGTROOT)/agtsys/tileset.o \
		$(AGTROOT)/agtsys/playfield.o \
		$(AGTROOT)/agtsys/arena.o \
		$(AGTROOT)/agtsys/entity.o \
		$(AGTROOT)/agtsys/input.o \
		$(AGTROOT)/agtsys/ealloc.o \
		$(AGTROOT)/agtsys/compress.o \
		$(AGTROOT)/agtsys/fileio.o \
		$(AGTROOT)/agtsys/os.o \
		$(AGTROOT)/agtsys/sound/sound.o \
		$(AGTROOT)/agtsys/sound/music.o \
		$(AGTROOT)/3rdparty/vsnprint.o \
		$(AGTROOT)/$(ASMROOT)/sys.o \
		$(AGTROOT)/$(ASMROOT)/qmem.o \
		$(AGTROOT)/$(ASMROOT)/ikbd.o \
		$(AGTROOT)/$(ASMROOT)/sound.o \
		$(AGTROOT)/$(ASMROOT)/pack2e.o \


AGLIB_STE_O =	\
		$(AGTROOT)/agtsys/shifter_ste.o \
		$(AGTROOT)/agtsys/pcs.o \
		$(AGTROOT)/$(ASMROOT)/ste/dispv6.o \
		$(AGTROOT)/$(ASMROOT)/ste/pflib.o \
		$(AGTROOT)/$(ASMROOT)/ste/display.o \
		$(AGTROOT)/$(ASMROOT)/ste/spritelib.o \
		$(AGTROOT)/$(ASMROOT)/ste/drawent.o \
		

ifeq ($(USE_HATARI_NATFEATS),yes)
COMPILERDEFS +=	-DHATARI_NF_PRINTF
NATFEATS_O =	\
		$(AGTROOT)/$(ASMROOT)/natfeats_.o \
		$(AGTROOT)/3rdparty/natfeats.o
else
NATFEATS_O =
endif


AGLIB_STE = $(AGLIB_O) $(AGLIB_STE_O) $(NATFEATS_O)


# =============================================================================
#	Targets to be built
# =============================================================================

# rule to build programs

all: tutorial

# individual programs...

# for each target to build, specify:
# - the AGLIB platform to use in the build rules (STF/STE)
# - the prerequisites themselves (AGLIB and the name of the demo ?.c/cpp -> ?.tos)

tutorial: AGLIB=$(AGLIB_STE)
tutorial: $(AGLIB_STE) tutor1.tos


# =============================================================================
#	Rule to clean projects
# =============================================================================

clean:
	-rm -f *.o
	-rm -f $(AGLIB_MINICRT)
	-rm -f $(AGLIB_STE)
	-rm -f tutor1.tos


# =============================================================================
#	Rule build a single program (demo!)
# =============================================================================

%.tos: $(AGLIB_MINICRT) $(AGLIB) %.o | Makefile
	$(LINK) $(LIBPATHS) -o $*.tos $(AGLIB_MINICRT) $(AGLIB) $*.o $(LIBS) $(LDFLAGS)

# -----------------------------------------------------------------------------
#	Configure user stack size (MiNTlib CRT only!)
# -----------------------------------------------------------------------------
ifeq ($(LINK_MINIMAL),no)
	$(STACK) $*.tos
endif
# -----------------------------------------------------------------------------
#	Process symbols to .sym files via NM (minus .locals, interrupts etc)
# -----------------------------------------------------------------------------
ifeq ($(USE_HATARI_DEBUGGING),yes)
	$(NM) -n $*.tos > $*_.sym
	fgrep -v -e ".L" -e " ." -e ".o" -e "/" -e " W " -e " b " -e " d " \
	  -e " w " -e " a " -e "___gnu_compiled_c" -e "gcc2_compiled." \
	  -e " _rcsid"  \
	  -e "_vbi" -e "shifter_vbl" -e "mouse_d" \
	  $*_.sym > $*.sym
	cp $*.sym $(HATARI_DEBUG_DIR)/.
	cp $*_.sym $(HATARI_DEBUG_DIR)/.
endif
# -----------------------------------------------------------------------------
#	Strip symbols & pack executable
# -----------------------------------------------------------------------------
ifeq ($(LINK_MINIMAL),yes)
	$(STRIP) $*.tos
ifeq ($(USE_UPX_PACKER),yes)
	$(AGTROOT)/bin/upx -9 $*.tos
#	$(AGTROOT)/bin/upx -9 --brute --ultra-brute $*.tos
endif
endif
# -----------------------------------------------------------------------------
#	Trasfer .prg to emulator GEMDOS auto folder for tests
# -----------------------------------------------------------------------------
ifeq ($(TEST_EMULATOR),yes)
	cp $*.tos $(EMULATOR_GEMDOS_DIR)/AUTO/$*.prg
endif

# =============================================================================
#	Build rules for source files
# =============================================================================

# rule to compile C files
%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

# rule to compile C++ files
%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<


# rules to assemble unspecified .s files -> .o
%.o: %.s
ifeq ($(USE_ASSEMBLER),rmac)
	$(RMAC) $(COMPILERDEFS) -l$@.lst -o $@ $<
else
	$(VASM) $(COMPILERDEFS) -L $@.lst -o $@ $<
endif

# =============================================================================
#	Build rules for preparing data
# =============================================================================

# ...nothing needed so far
	
# =============================================================================
#	Build rules for collating common data
# =============================================================================

# ...nothing needed so far
	