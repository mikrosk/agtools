# -----------------------------------------------------------------------------
#	Configure project
# -----------------------------------------------------------------------------

TARGETNAME = agtcut

TARGETFLAGS =

AGTROOT = ../..

BUILD_DIR = build

USE_CFG = release

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
#TOOLCHAIN_NAME = mingw32
TOOLCHAIN_NAME = i686-w64-mingw32
#TOOLCHAIN_INCLUDES = -Iusr/i686-pc-mingw32/sys-root/mingw/include
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
CXX = $(TOOLCHAIN_PREFIX)g++ -x c++ -std=c++11
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

CCINCLUDES = $(TOOLCHAIN_INCLUDES) -I$(AGTROOT) -I$(AGTROOT)/tools/3rdparty  -I$(AGTROOT)/tools/common

COMPILERDEFS += \

ifeq ($(USE_CFG),debug)

CODEOPTFLAGS = $(TARGETFLAGS) -g -Od -fomit-frame-pointer
CODEGENFLAGS = $(CODEOPTFLAGS)

else

CODEOPTFLAGS = $(TARGETFLAGS) -O3
CODEGENFLAGS = $(CODEOPTFLAGS)

endif

LDOPTS = $(TOOLCHAIN_LDFLAGS)

ifeq ($(host_system),Linux)
CODEGENFLAGS += -pthread
endif

# -----------------------------------------------------------------------------
#	Final flags
# -----------------------------------------------------------------------------

SHAREDFLAGS = -Wall -Wno-unused-function -Wno-sign-compare -Wno-narrowing -Wno-unused-variable -Wno-unused-but-set-variable -Wno-multichar -Wno-misleading-indentation -Wno-dangling-else

CXXFLAGS = $(CODEGENFLAGS) $(COMPILERDEFS) $(SHAREDFLAGS) -Wno-reorder -fno-rtti 

LDFLAGS += $(CODEGENFLAGS) $(LDOPTS)

LIBPATHS = $(TOOLCHAIN_LIB_DIRS)
LIBS = $(TOOLCHAIN_LIBS)

# -----------------------------------------------------------------------------
#	Sourcefile list
# -----------------------------------------------------------------------------

SRC_LIST = \
	agtcut.cpp

INC_LIST = \
	68ksimops.h \
	68kxforms.inl \
	emxgen.inl \
	emximpl_v2.inl \

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
.SUFFIXES: .c .cpp .h .inl

# keep compiled intermediates in build directory
#
.PRECIOUS: $(OBJ_DIR)/%.o

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
$(OBJ_DIR)/%.o: %.c $(INC_LIST)
	@echo "COMPILING $< -> $@"
	@mkdir -p '$(@D)'
	$(CC) $(CCINCLUDES) $(CFLAGS) -c -o $@ $<

# rule to compile .cpp files
$(OBJ_DIR)/%.o: %.cpp $(INC_LIST)
	@echo "COMPILING $< -> $@"
	@mkdir -p '$(@D)'
	$(CXX) $(CCINCLUDES) $(CXXFLAGS) -c -o $@ $<
