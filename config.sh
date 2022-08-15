#!/usr/bin/env bash

# script to autoconfigure $AGTBIN tool dir for a supported host environment

# notes:
# - not all host platforms have been tested - some of these are placeholders e.g. RPI
# - not all architectures are provided for e.g. MacOS/32bit
# - Cygwin 32/64 both point at statically-linked Win32 binaries (mingw32)
# - MinGW 32/64 both point at statically-linked Win32 binaries (mingw32)
# - Linux may be tricky for the pre-built binaries (PCS, RMAC) due to dylib issues.
#   other binaries can be built from source using the autodetermined sys/arch pair

# user's project script passes AGTROOT base directory
#AGTROOT="${1}" # unwound, no longer required. produce stub only.


# some defaults... provides feedback via the constructed path if we're not supported yet
HOST_ARCH=`uname -m`
HOST_SYSTEM=`uname -s`
HOST_NAME=unknown

# <Darwin
if [ `uname -s` == "Darwin" ]; then
HOST_NAME=MacOS
HOST_ARCH=x86_64		# default, for now
if [ `uname -m` == "arm64" ]; then
HOST_ARCH=x86_64		# M1 (arm64) forces Rosetta2 until binaries are added
elif [ `uname -m` == "i686" ]; then
@echo "unsupported 32bit arch"	# just don't bother with Darwin/32. Apple doesn't!
error
fi
# Darwin>

# <RPI
elif [ `uname -s` == "raspberrypi" ]; then
HOST_SYSTEM=rpi			# hmm, equalize this for now. one set of binaries.
HOST_NAME=RaspberryPI
#HOST_ARCH=armv71/aarch64s	# auto-determined
# RPI>

# <Linux/RPI-OS-64
elif [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
HOST_NAME=Linux
# need binaries for Linux, probably x64 only for now
# Linux>

# <Windows mess
elif [ "$(expr substr $(uname -s) 1 10)" == "MINGW32_NT" ]; then
HOST_NAME=Mingw32
HOST_SYSTEM=win
HOST_ARCH=x86
elif [ "$(expr substr $(uname -s) 1 10)" == "MINGW64_NT" ]; then
HOST_NAME=Mingw64
HOST_SYSTEM=win
HOST_ARCH=x86		# 64bit uses 32bit builds for now
elif [ "$(expr substr $(uname -s) 1 6)" == "CYGWIN" ]; then
HOST_NAME=Cygwin
HOST_SYSTEM=win
if [ `uname -m` == "i686" ]; then
HOST_NAME=Cygwin32
HOST_ARCH=x86
else
HOST_NAME=Cygwin64
HOST_ARCH=x86		# 64bit uses 32bit builds for now
fi # i686
else
HOST_NAME=unknown
# Windows mess>

fi # [host]

# build the host-specific bin/ path and return it...
HOST_STUB=${HOST_SYSTEM}/${HOST_ARCH}
echo "$HOST_STUB"
