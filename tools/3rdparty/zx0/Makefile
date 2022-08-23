# -----------------------------------------------------------------------------
#	Configure project
# -----------------------------------------------------------------------------

TARGETNAME = zx0

TARGETFLAGS =

AGTROOT = ../../..

BUILD_DIR = build

# -----------------------------------------------------------------------------
#	Detect Windows so we can coerce more compatible binaries
# -----------------------------------------------------------------------------

ifeq ($(OS),Windows_NT)
	host_system := Windows
else
	host_system := $(shell uname -s)
endif

# -----------------------------------------------------------------------------
#	Windows mingw32 build
# -----------------------------------------------------------------------------

ifeq ($(host_system),Windows)

# produce statically-linked mingw32 binaries for cross-system compatibility
#
TOOLCHAIN_NAME = mingw32
TOOLCHAIN_INCLUDES = -Iusr/i686-pc-mingw32/sys-root/mingw/include
TOOLCHAIN_LDFLAGS = -s -static -mconsole
TOOLCHAIN_LIB_DIRS = -L/lib/w32api
TOOLCHAIN_LIBS =
TOOLCHAIN_PREFIX = $(TOOLCHAIN_NAME)-

else

# -----------------------------------------------------------------------------
#	MacOS, Linux...
# -----------------------------------------------------------------------------

# native/default
#
TOOLCHAIN_NAME =
TOOLCHAIN_INCLUDES =
TOOLCHAIN_LDFLAGS =
TOOLCHAIN_LIB_DIRS =
TOOLCHAIN_LIBS =
TOOLCHAIN_PREFIX =

endif

# -----------------------------------------------------------------------------

CC = $(TOOLCHAIN_PREFIX)gcc -x c
CXX = $(TOOLCHAIN_PREFIX)g++ -x c++
AR = $(TOOLCHAIN_PREFIX)ar
AS = $(TOOLCHAIN_PREFIX)as
NM = $(TOOLCHAIN_PREFIX)nm
LINK = $(TOOLCHAIN_PREFIX)g++
STRIP = $(TOOLCHAIN_PREFIX)strip
STACK = $(TOOLCHAIN_PREFIX)stack
FLAGS = $(TOOLCHAIN_PREFIX)flags

# -----------------------------------------------------------------------------
#	Configure the host
# -----------------------------------------------------------------------------

HOST_STUB = $(shell $(AGTROOT)/config.sh)
HOST_TOOL_DIR = ${AGTROOT}/bin/$(HOST_STUB)

OBJ_DIR = $(BUILD_DIR)/$(HOST_STUB)

# -----------------------------------------------------------------------------
#	Configure compilation
# -----------------------------------------------------------------------------

CODEGENFLAGS = -O3 -Wall
COMPILERDEFS =
CCINCLUDES =
LDOPTS =

# -----------------------------------------------------------------------------
#	Final flags
# -----------------------------------------------------------------------------

CFLAGS	= $(CODEGENFLAGS) $(COMPILERDEFS) -Wall -fno-stack-check

#	-ox -ob -ol+ -onatx -oh -zp8 -fp6 -g0 -Ot -oe -ot -Wall -xc -s \
#	-finline-functions -finline-intrinsics -finline-math -floop-optimize -frerun-optimizer -fno-stack-check \
#	-march=i386 -mtune=i686


LDFLAGS += $(CFLAGS) $(LDOPTS)

LIBPATHS = $(TOOLCHAIN_LIB_DIRS)
LIBS = $(TOOLCHAIN_LIBS)

# -----------------------------------------------------------------------------
#	Sourcefile list
# -----------------------------------------------------------------------------

SRC_LIST = \
	zx0.c \
	compress.c \
	optimize.c \
	memory.c

# -----------------------------------------------------------------------------
#	Translate src->obj for local build intermediates
# -----------------------------------------------------------------------------

TMP00 = $(SRC_LIST)
TMP10 = $(TMP00:%.c=$(OBJ_DIR)/%.o)
TMP20 = $(TMP10:%.cpp=$(OBJ_DIR)/%.o)
OBJ_LIST = $(TMP20)

# -----------------------------------------------------------------------------

# =============================================================================
#	Common build rules & automation for all projects
# =============================================================================

# limit recognised suffixes
#
.SUFFIXES:
.SUFFIXES: .c .h

# keep compiled intermediates in build directory
#
#.PRECIOUS: $(OBJ_DIR)/%.o

all: $(OBJ_DIR)/$(TARGETNAME)

# =============================================================================
#	Build rules for project
# =============================================================================

$(OBJ_DIR)/$(TARGETNAME): $(OBJ_LIST) | Makefile
	@echo "LINKING -> $@"
	@mkdir -p '$(@D)'
	$(LINK) $(LIBPATHS) -o $(OBJ_DIR)/$(TARGETNAME) $(OBJ_LIST) $(LDFLAGS) $(LIBS)
	@echo "INSTALLING $@"
	@mkdir -p $(HOST_TOOL_DIR)
	cp $(OBJ_DIR)/$(TARGETNAME) $(HOST_TOOL_DIR)/.

clean:
	@mkdir -p $(BUILD_DIR)/$(HOST_STUB)
	rm -f $(OBJ_DIR)/$(TARGETNAME)
	rm -f $(OBJ_LIST)
	find $(OBJ_DIR) -type f -name '*.o' -exec rm {} +

# =============================================================================
#	Build rules for project-local sourcefiles
# =============================================================================

# rule to compile .c files
$(OBJ_DIR)/%.o: %.c
	@echo "COMPILING $< -> $@"
	@mkdir -p '$(@D)'
	$(CC) $(CCINCLUDES) $(CFLAGS) -c -o $@ $<

# rule to compile .cpp files
$(OBJ_DIR)/%.o: %.cpp
	@echo "COMPILING $< -> $@"
	@mkdir -p '$(@D)'
	$(CXX) $(CCINCLUDES) $(CXXFLAGS) -c -o $@ $<
