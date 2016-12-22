
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
#	Choose TOS/GCC4 or BrownElf/GCC6 as preferred compiler
# =============================================================================
USE_BROWNELF=no
# -----------------------------------------------------------------------------
# Notes:
#     - Vincent's GCC 4.6.4 emits .tos files directly but does not support
#	Linktime Optimisation (LTO) or named sections. Symbols must be gathered
#	and converted using GNU NM then loaded into Hatari.
#     - BrownElf GCC 6.2 emits .elf files which must be converted to .tos
#	using 'brownout'. This compiler supports LTO, named sections & GST
#	symbols but requires extra support objects to perform c++ static init.
# -----------------------------------------------------------------------------

# =============================================================================
#	Use minimal CRT & library code (i.e. avoiding MiNTlib)
# =============================================================================
LINK_MINIMAL=yes
# -----------------------------------------------------------------------------
# Notes:
#	Prefer 'yes' for final optimized builds - saves a lot of space but
#	most LIBC functionality will be missing. Implements only bare
#	minimum functionality. Use 'no' if you need full LIBC support.
# -----------------------------------------------------------------------------

# =============================================================================
#	debug	= Prevent optimization & add debug info
#	testing	= Optimize but retain symbols
#	release	= Optimize, strip & permit packing
# =============================================================================
USE_CFG=testing
# -----------------------------------------------------------------------------

# =============================================================================
#	Pack final executable (only in 'release' mode)
# =============================================================================
USE_UPX_PACKER=yes
# -----------------------------------------------------------------------------
# Notes:
#	Tested with UPX 3.91w but any recent version should do
# -----------------------------------------------------------------------------

# =============================================================================
#	AGT project root dir, relative to this project
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
HATARI_DEBUG_DIR = .
# pipe printf messages to Hatari debugger via natfeats INSTEAD of tos console
# note: this catches all printfs - even when the display is locked for graphics
USE_HATARI_NATFEATS=no
# produce symbols for Hatari debugger
USE_HATARI_DEBUGGING=no
# -----------------------------------------------------------------------------
# Requires:
# -----------------------------------------------------------------------------
#	Hatari & support tools install
# -----------------------------------------------------------------------------


# =============================================================================
#	Some standard C/ASM config - probably won't need changed too often
# =============================================================================

TARGETFLAGS =	-m68000

ifeq ($(USE_BROWNELF),yes)

LIBPATHS =	\
		-L/lib/gcc/m68k-ataribrown-elf/6.2.0/m68000 \
		-L/usr/m68k-ataribrown-elf/lib/m68000
else

LIBPATHS =	\
		-L/opt/cross-mint/lib/gcc/m68k-atari-mint/4.6.4/ \
		-L/opt/cross-mint/m68k-atari-mint/lib/
endif

LIBS =		\

INCPATHS =	-I$(AGTROOT) -I3rdparty/

ASMROOT = $(USE_ASSEMBLER)

CRTSEQ_G =
CRTSEQ_S =
CRTSEQ_E = 

# -----------------------------------------------------------------------------
#	With LINK_MINIMAL, we modify the linker command to use minimal LIBC
# -----------------------------------------------------------------------------
ifeq ($(LINK_MINIMAL),yes)
# -----------------------------------------------------------------------------

COMPILERDEFS += -DUSE_TINY_CRT

ifeq ($(USE_BROWNELF),yes)

CRTSEQ_S = \
	$(AGTROOT)/agtsys/libcxx/$(ASMROOT)/brownboot.o \
	$(AGTROOT)/agtsys/libcxx/browncrti.o \
	$(AGTROOT)/agtsys/libcxx/browncrt++.o \
	$(AGTROOT)/agtsys/libcxx/zerolibc.o \
	$(AGTROOT)/agtsys/libcxx/zerocrtfini.o
CRTSEQ_E = \
	$(AGTROOT)/agtsys/libcxx/browncrtn.o

COMPILERDEFS += -DAGT_GCC6_ELF

# GCC6/ELF also needs to generate code with base address = 0 for postprocessing
LDOPTS = -Wl,--emit-relocs -Wl,-e_start -Ttext=0

else

# GCC4/TOS linking is simpler
CRTSEQ_S = \
	$(AGTROOT)/agtsys/libcxx/$(ASMROOT)/zeroboot.o \
	$(AGTROOT)/agtsys/libcxx/zerolibc.o \
	$(AGTROOT)/agtsys/libcxx/zerocrtfini.o

LDOPTS = -Wl,--gc-sections

endif

# -----------------------------------------------------------------------------

LDFLAGS = -Wl,-Map,$*.map $(LDOPTS) -nostdlib -nostartfiles $(CODEGENFLAGS)

# -----------------------------------------------------------------------------
else
# -----------------------------------------------------------------------------

ifeq ($(USE_BROWNELF),yes)

# GCC6/ELF linking for MiNTlib & C++ is complicated by changes to ctor/dtor
# list representation, code required to implement them and having to specify
# all of the link pieces manually.

# crt0:
#  gcc-provided entrypoint/boot code
#
# browncrti:
#  marks beginning of BrownElf .init_array/.fini_array - must be linked early
# browncrtn:
#  marks end of BrownElf .init_array/.fini_array - must be linked last
# brownmint++:
#  when combining MiNTlib with C++ using BrownElf, crtinit.c must be replaced with one
#  which performs the necessary ELF-style static initialisation because MiNTlib
#  doesn't do this and GCC6 doesn't quietly inject static inits before main() as
#  GCC4 does
# browncrt++:
#  implements BrownElf-specific C++ .init_array/.fini_array procedures
#
# zerocrtfini:
#  implements __cxa_atexit/__cxa_finalize needed by C++ static init. missing
#  from MiNTlib and reused for bare-bones builds on both compilers.

CRTSEQ_G = /usr/m68k-ataribrown-elf/lib/crt0.o
CRTSEQ_S = \
	$(AGTROOT)/agtsys/libcxx/browncrti.o \
	$(AGTROOT)/agtsys/libcxx/browncrt++.o \
	$(AGTROOT)/agtsys/libcxx/brownmint++.o \
	$(AGTROOT)/agtsys/libcxx/zerocrtfini.o
CRTSEQ_E = \
	$(AGTROOT)/agtsys/libcxx/browncrtn.o

LDOPTS = -Wl,--gc-sections -Wl,--emit-relocs -Wl,-e__start -Ttext=0 -nostdlib -nostartfiles
LIBS = -lgcc -lc
 
else

# GCC4/TOS linking is simpler & C++ static init is injected by compiler so no
# extra startup stuff required. we just add the c++ lib if required.

LDOPTS = -Wl,--stack,$(STACKSIZE) -Wl,--gc-sections
LIBS =

endif

# -----------------------------------------------------------------------------

LDFLAGS = -Wl,-Map,$*.map $(LDOPTS) $(CODEGENFLAGS)

# -----------------------------------------------------------------------------
endif
# -----------------------------------------------------------------------------

ifeq ($(USE_BROWNELF),yes)

VASM = $(AGTROOT)/bin/vasm16c -Felf -showcrit -noesc -I$(AGTROOT)/vasm -I$(AGTROOT)/
RMAC = unsupported
TOOLCHAIN_PREFIX = m68k-ataribrown-elf-
EXT = elf

else

VASM = $(AGTROOT)/bin/vasm16c -Faout -showcrit -noesc -I$(AGTROOT)/vasm -I$(AGTROOT)/
RMAC = $(AGTROOT)/bin/rmac -fa +oall -i"$(AGTROOT)/rmac;$(AGTROOT)"
TOOLCHAIN_PREFIX = m68k-atari-mint-
EXT = tos

endif


CC = $(TOOLCHAIN_PREFIX)gcc
CXX = $(TOOLCHAIN_PREFIX)g++
AR = $(TOOLCHAIN_PREFIX)ar
AS = $(TOOLCHAIN_PREFIX)as
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
		-D__ATARI__ -D__M68000__  \
		-DAGT_CONFIG_STACK=$(STACKSIZE) \

# -----------------------------------------------------------------------------
#	Tweaks
# -----------------------------------------------------------------------------

COMPILERDEFS += \
		-DAGT_CONFIG_WORLD_YMAJOR \

# -----------------------------------------------------------------------------
#	Additional dev/debug options
# -----------------------------------------------------------------------------

COMPILERDEFS += \
#		-DUSE_MEMTRACE \
#		-DTIMING_RASTERS_MAIN \

# -----------------------------------------------------------------------------
#	C Code generation
# -----------------------------------------------------------------------------

ifeq ($(USE_CFG),debug)

CODEOPTFLAGS = $(TARGETFLAGS) -g -O0
CODEGENFLAGS = $(CODEOPTFLAGS)

else

CODEOPTFLAGS = $(TARGETFLAGS) -Ofast -fomit-frame-pointer -fstrict-aliasing
CODEGENFLAGS = $(CODEOPTFLAGS) -fcaller-saves
ifeq ($(USE_BROWNELF),yes)
CODEGENFLAGS += -flto -ffunction-sections -fdata-sections
endif

endif

ifeq ($(USE_BROWNELF),yes)
CODEGENFLAGS += -fleading-underscore
endif

CFLAGS	= $(CODEGENFLAGS) $(COMPILERDEFS) $(INCPATHS) -Wall
CXXFLAGS = $(CODEGENFLAGS) $(COMPILERDEFS) $(INCPATHS) -x c++ -std=c++0x -fno-exceptions -fno-rtti -fno-threadsafe-statics -Wall -Wno-reorder



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
		
# -----------------------------------------------------------------------------
#	Join objects into one list, adding any extra stuff we need
# -----------------------------------------------------------------------------

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

all: tutorial_5a tutorial_5b tutorial_5c

# individual programs...

# for each target to build, specify:
# - the AGLIB platform to use in the build rules (STF/STE)
# - the prerequisites themselves (AGLIB and the name of the demo ?.c/cpp -> ?.tos)

tutorial_5a: AGLIB=$(AGLIB_STE)
tutorial_5a: $(AGLIB_STE) tutor5a.$(EXT)

tutorial_5b: AGLIB=$(AGLIB_STE)
tutorial_5b: $(AGLIB_STE) tutor5b.$(EXT)

tutorial_5c: AGLIB=$(AGLIB_STE)
tutorial_5c: $(AGLIB_STE) tutor5c.$(EXT)


# =============================================================================
#	Rule to clean projects
# =============================================================================
# note: CRTSEQ_S, CRTSEQ_E are local to AGT so they can be cleaned. CRTSEQ_G
# however belongs to GCC as a precompiled .o, and should never be cleaned!
# -----------------------------------------------------------------------------

clean:
	-rm -f *.elf
	-rm -f *.tos
	-rm -f $(CRTSEQ_S)
	-rm -f $(CRTSEQ_E)
	-rm -f $(AGLIB_STE)
	-rm -rf *.o
	-rm -rf *.lst




# =============================================================================
#	Rule build a single program (demo!)
# =============================================================================
%.tos: $(CRTSEQ_S) $(AGLIB) %.o | Makefile
	$(LINK) $(LIBPATHS) -o $*.tos $(CRTSEQ_S) $(AGLIB) $*.o $(LDFLAGS) $(LIBS)
# -----------------------------------------------------------------------------
#	Configure user stack size (MiNTlib CRT only!)
# -----------------------------------------------------------------------------
ifeq ($(LINK_MINIMAL),no)
	$(STACK) $*.tos
endif
# -----------------------------------------------------------------------------
ifeq ($(USE_CFG),release)
# -----------------------------------------------------------------------------
#	Strip symbols & pack executable
# -----------------------------------------------------------------------------
	$(STRIP) $*.tos
ifeq ($(USE_UPX_PACKER),yes)
	$(AGTROOT)/bin/upx -9 $*.tos
#	$(AGTROOT)/bin/upx -9 --brute --ultra-brute $*.tos
endif
# -----------------------------------------------------------------------------
else
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
endif
# -----------------------------------------------------------------------------
#	Trasfer .prg to emulator GEMDOS auto folder for tests
# -----------------------------------------------------------------------------
ifeq ($(TEST_EMULATOR),yes)
	cp $*.tos $(EMULATOR_GEMDOS_DIR)/auto/$*.prg
endif






# =============================================================================
#	Rule build a single program (demo!)
# =============================================================================
%.elf: $(CRTSEQ_G) $(CRTSEQ_S) $(AGLIB) %.o $(CRTSEQ_E) | Makefile
	$(LINK) $(LIBPATHS) -o $*.elf $(CRTSEQ_G) $(CRTSEQ_S) $(AGLIB) $*.o $(LDFLAGS) $(LIBS) $(CRTSEQ_E)
# -----------------------------------------------------------------------------
#	Strip symbols & pack executable
# -----------------------------------------------------------------------------
ifeq ($(USE_CFG),release)
# -----------------------------------------------------------------------------
	$(AGTROOT)/bin/brownout -p 0 -i $*.elf -o $*.tos
ifeq ($(USE_UPX_PACKER),yes)
	$(AGTROOT)/bin/upx -9 $*.tos
#	$(AGTROOT)/bin/upx -9 --brute --ultra-brute $*.tos
endif
# -----------------------------------------------------------------------------
else
# -----------------------------------------------------------------------------
	$(TOOLCHAIN_PREFIX)objdump -h $*.elf
	$(AGTROOT)/bin/brownout -s -x -p 0 -i $*.elf -o $*.tos
# -----------------------------------------------------------------------------
endif
# -----------------------------------------------------------------------------
#	Trasfer .prg to emulator GEMDOS auto folder for tests
# -----------------------------------------------------------------------------
ifeq ($(TEST_EMULATOR),yes)
	cp $*.tos $(EMULATOR_GEMDOS_DIR)/auto/$*.prg
endif


# =============================================================================
# to use MiNTLib with GCC6 static initialisation (for c++) we need to inject
# a custom version of crtinit.c which calls __libc_csu_init(). 
# The compile flags are very specific and the MiNTLib source includes are not
# installed with the compiler so we have copies.
# -----------------------------------------------------------------------------
# note: alternative LIBC packages may require the same type of modification if
# they don't already call __libc_csu_init()
# -----------------------------------------------------------------------------
# todo: ship this as a precompiled object!
# =============================================================================

$(AGTROOT)/agtsys/libcxx/brownmint++.o: $(AGTROOT)/agtsys/libcxx/brownmint++.c
	$(TOOLCHAIN_PREFIX)gcc -Wall -m68000 -O2 -std=gnu89 -fleading-underscore -nostdinc \
	-I$(AGTROOT)/agtsys/libcxx \
	-I/usr/lib/gcc/m68k-ataribrown-elf/6.2.0/include \
	-I/usr/lib/gcc/m68k-ataribrown-elf/6.2.0/include-fixed \
	-I/usr/m68k-ataribrown-elf/include \
	-DHAVE_CONFIG_H -D_LIBC -D_REENTRANT \
	-c $< -o $@


# =============================================================================
#	Build rules for source files
# =============================================================================

# rule to compile C files
%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

# rule to compile C++ files
%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

# rules to assemble .gas files -> .o
%.o: %.gas
	$(AS) -o $@ $<


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
	