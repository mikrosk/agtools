
# =============================================================================
#       Choose RMAC or VASM as preferred assembler
# =============================================================================
USE_ASSEMBLER=rmac
# -----------------------------------------------------------------------------
# Notes:
#       Tested with RMAC v1.5.3
#       Tested with VASM v1.6c
# -----------------------------------------------------------------------------

# =============================================================================
#       Choose TOS/GCC4 or BrownElf/GCC6 as preferred compiler
# =============================================================================
USE_BROWNELF=no
# -----------------------------------------------------------------------------
# Notes:
#     - Tested with m68k-atari-mint GCC 4.6.4
#     - Tested with m68k-ataribrown-elf GCC 6.2.0
#     - m68k-atari-mint emits .tos files directly.
#     - m68k-ataribrown-elf emits .elf files which are then converted to .tos.
#     - m68k-atari-mint symbols can be exported using m68k-atari-mint-nm and
#       importing the ascii manually into Hatari using 'symbols' command.
#     - m68k-ataribrown-elf requires careful linkage of custom object files,
#       particularly for static initialisation and ctor/dtor behaviour in C++.
#       This is handled automatically via the common [Makerules] file.
#     - m68k-ataribrown-elf provides improved functionality vs atari-mint:
#       * C++11 support
#       * LTO (linktime global optimisation)
#       * standard 8-character and extended format debugging symbols in .tos
#       * named sections, grouping & layout preserved until conversion to .tos
#       * alignment (up to 4 bytes)
# -----------------------------------------------------------------------------

# =============================================================================
#       Use minimal CRT & library code (i.e. avoiding MiNTlib)
# =============================================================================
LINK_MINIMAL=yes
# -----------------------------------------------------------------------------
# Notes:
#       'yes' = Provides the barest minimum LIBC functionality (zerolib) used 
#       by the current sample projects. Results in a small executable.
#       'no'  = Link to MiNTlib, providing full LIBC functionality. Results in
#       typically very large executables, but actual size depends on use.
# -----------------------------------------------------------------------------

# =============================================================================
#       Build configurations
# =============================================================================
USE_CFG=release
# -----------------------------------------------------------------------------
# Notes:
#       debug	= Limit optimization & extra debug info
#       testing	= Optimize fully but retain assert() & debugging symbols
#       release	= Optimize, strip & permit packing
# -----------------------------------------------------------------------------

# =============================================================================
#       Pack final executable (only used in 'release' mode)
# =============================================================================
USE_UPX_PACKER=yes
# -----------------------------------------------------------------------------
# Requires:
#       Tested with UPX 3.91w but any recent version should do
# -----------------------------------------------------------------------------

# =============================================================================
#       AGT project root dir, relative to *this* project
# =============================================================================
AGTROOT = ../..

# =============================================================================
#       Emulator testing (via GEMDOS share)
# =============================================================================
TEST_EMULATOR=yes
EMULATOR_GEMDOS_DIR = .
# -----------------------------------------------------------------------------
# Notes:
#       Bundles .prg into $(EMULATOR_GEMDOS_DIR)/AUTO for easy testing
# Requires:
#       Hatari/STEEM or other emulator with hosted GEMDOS directory emulation
# -----------------------------------------------------------------------------

# =============================================================================
#       Emulator (Hatari) debugger & profiler support
# =============================================================================
USE_HATARI_NATFEATS=no
USE_HATARI_DEBUGGING=no
HATARI_DEBUG_DIR = .
# -----------------------------------------------------------------------------
# Notes:
#     - USE_HATARI_NATFEATS=yes will cause printf activity to be redirected
#       to Hatari, assuming Hatari is started with '--natfeats=yes'.
#       For STEEM printf logging, add -DAGT_CONFIG_STEEM_DEBUG instead (but
#       be aware using this switch will crash on real hardware!)
#     - USE_HATARI_DEBUGGING will export ascii-formatted symbols for manually
#       importing into Hatari using command 'symbols TEXT DATA BSS'.
#       The exported symbols contain local labels and other junk and benefit
#       from extensive grep filtering beforehand (see Makerules).
#     - HATARI_DEBUG_DIR specifies where to export the ascii symbols for use.
# Requires:
#	Hatari & support tools install
# -----------------------------------------------------------------------------


# =============================================================================
#	Specify stack size
# =============================================================================
STACKSIZE = 16384
# -----------------------------------------------------------------------------
# Notes:
#     - Specify supervisor stack: used for main stack, interrupts & exceptions
#     - Size in terms of bytes
#     - Used by zeroboot.s/brownboot.s (LINK_MINIMAL=yes)
#     - Used by S_SUPER_SSP/S_SUPER/S_USER macros (LINK_MINIMAL=no)
# -----------------------------------------------------------------------------


# =============================================================================
#	AGT engine configuration
# =============================================================================
# Notes:
#     - see Wiki: https://bitbucket.org/d_m_l/agtools/wiki/Compiletime%20Flags
# -----------------------------------------------------------------------------

COMPILERDEFS += \
		-DAGT_CONFIG_WORLD_YMAJOR \
#		-DAGT_CONFIG_VISTRIGGER \
#		-DAGT_CONFIG_DYNAMIC_TICKPRIO \
#		-DENABLE_AGT_AUDIO \

# =============================================================================
#	Debugging, diagnostic logging & profiling options
# =============================================================================
# Notes:
#     - see Wiki: https://bitbucket.org/d_m_l/agtools/wiki/Compiletime%20Flags
# -----------------------------------------------------------------------------

COMPILERDEFS += \
#		-DAGT_CONFIG_SAFETY_CHECKS \
#		-DAGT_CONFIG_DEBUG_CONSOLE \
#		-DAGT_CONFIG_DEBUGDRAW \
#		-DAGT_CONFIG_STEEM_DEBUG \
#		-DTIMING_RASTERS -DTIMING_RASTERS_MAIN \
#		-DUSE_MEMTRACE \

# -----------------------------------------------------------------------------
#	Additional project-local object files to be built (if any)
# -----------------------------------------------------------------------------

PROJECT_O = \

# =============================================================================
#	Targets to be built
# =============================================================================

# rule to build programs

all: ste


# =============================================================================
#	Shared build rules
# =============================================================================

include $(AGTROOT)/Makerules


# =============================================================================
# individual programs...
# =============================================================================

# for each target to build, specify:
# - the AGLIB platform to use in the build rules (STF/STE)
# - the prerequisites themselves (AGLIB and the name of the demo ?.c/cpp -> ?.tos)

ste: AGLIB=$(AGLIB_STE)
ste: $(AGLIB_STE) isoscrol.$(EXT)

# treat these names as targets not sourcefiles otherwise bad stuff can get
# accidentally added to the link line (we have some sourcefiles with the same
# names as some targets, sending make to confusionland)
.PHONY: ste

# =============================================================================
#	Build rules for preparing data
# =============================================================================

# ...nothing needed so far
	
# =============================================================================
#	Build rules for collating common data
# =============================================================================

# ...nothing needed so far
	