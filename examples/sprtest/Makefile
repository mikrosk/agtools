# =============================================================================
#	Base build config
# =============================================================================

# project-relative location of agtools repo root dir
# note: can remove this and set $AGTROOT var externally if preferred
AGTROOT = ../..

# project name - mainly for layout - target binaries can be individually named
PROJECT := sprtest

# debug/testing/release
BUILD_CFG = release

# remote testing post-build step, if required [ultradev,hxcusb]
POST_REMOTE = ultradev
REMOTE_PRG = sprtest.prg

# make job can specify one AGT 'architecture' to target at a time (mainly STE)
AGLIB=$(AGLIB_STE)
AGLIB_OBJ=$(AGLIB_STE_OBJ)

PACK=${AGTROOT}/scripts/pack.sh

# =============================================================================
#	Optional build config
# =============================================================================

# if yes, will rebuild palettes on changes. otherwise only build if missing
# note: projects with prebuilt palettes will be committed with 'no'
REBUILD_PALETTES = no

# links Hatari natfeats lib into program
USE_HATARI_NATFEATS = no

# select debug console (no/none, native, udev)
USE_DEBUG_CONSOLE = native


# =============================================================================
#	Post-build steps
# =============================================================================

# pack (release) executables, post-build
POST_UPX_PACKER = no


# =============================================================================
#	Compiletime config
# =============================================================================

# directory to pull all product assets+program together for testing/zipping etc.
PRODUCT_DISK1 := ./disk1
PRODUCT_DISK2 := ./disk2

# directory for final disk images, if used
IMAGES := ./images

# =============================================================================
#	AGT engine configuration
# =============================================================================
# Notes:
#     - see Wiki: https://bitbucket.org/d_m_l/agtools/wiki/Compiletime%20Flags
# -----------------------------------------------------------------------------

COMPILERDEFS += \
		-DAGT_CONFIG_WORLD_XMAJOR \
		-DAGT_CONFIG_SYS_GUARDX=64 \
		-DAGT_CONFIG_SYS_GUARDY=32 \
		-DAGT_CONFIG_DEBUG_SHOWCLIPS \
		-DAGT_CONFIG_PACKER_ANY \

#		-DAGT_CONFIG_ENTITYLAYERS=4 \
#		-DAGT_CONFIG_DYNAMIC_TICKPRIO \
#		-DENABLE_AGT_AUDIO \

# =============================================================================
#	Debugging, diagnostic logging & profiling options
# =============================================================================
# Notes:
#     - see Wiki: https://bitbucket.org/d_m_l/agtools/wiki/Compiletime%20Flags
# -----------------------------------------------------------------------------

COMPILERDEFS += \
		-DAGT_CONFIG_ENABLE_DISPLAYTIMER \
		-DTIMING_RASTERS_MAIN

#		-DTIMING_RASTERS \
#		-DAGT_CONFIG_SAFETY_CHECKS \
#		-DAGT_CONFIG_DEBUG_CONSOLE \
#		-DAGT_CONFIG_DEBUGDRAW \
#		-DAGT_CONFIG_STEEM_DEBUG \
#		-DTIMING_RASTERS -DTIMING_RASTERS_MAIN \
#		-DUSE_MEMTRACE \

# =============================================================================
#	Additional project-local sourcefiles (if any)
# =============================================================================

PROJECT_SRC_LIST = \

PROJECT_INC_LIST = \

# =============================================================================
#	Project assets
# =============================================================================

# input files which cause palettes to be rebuilt with [REBUILD_PALETTES = yes]
# NOTE: optional, to trigger auto-rebuild of palettes when these change

PALETTE_SOURCES = \

# assets common to all stages
COM_SRC = source_assets

# source asset list is optional - triggers assets_common.sh
SOURCE_ASSETS_COMMON = \
	${COM_SRC}/bg.png \

ASSETS_COMMON = \
	$(ASSETS)/bg.ccm \
	$(ASSETS)/bg.cct \
	\
	$(ASSETS)/1ems.ems \
	$(ASSETS)/2emx.emx \
	$(ASSETS)/3emh.emh \
	$(ASSETS)/4slab.slr \
	$(ASSETS)/4slab.sls \

# =============================================================================
#	Shared build definitions, common to all projects
# =============================================================================

include $(AGTROOT)/makedefs


# =============================================================================
#	Shared build definitions, common to all projects
# =============================================================================

include $(AGTROOT)/makedefs


# =============================================================================
#	Targets to be built
# =============================================================================

# -----------------------------------------------------------------------------
# by default, build product directory (programs + assets)
# -----------------------------------------------------------------------------
all: disk1


# -----------------------------------------------------------------------------
# build disk images using external script
# -----------------------------------------------------------------------------
image: $(CACHE)/disk1.dep $(CACHE)/disk2.dep
	$(AGTROOT)/post_image.sh $(PRODUCT_DISK1) $(IMAGES) disk1


# -----------------------------------------------------------------------------
# define content of disk1
# -----------------------------------------------------------------------------

DISK1_BINARY = $(BUILD_DIR)/sprtest.prg
DISK1_ASSETS = $(ASSETS_COMMON)

disk1: $(CACHE)/disk1.dep

clean-disk1:
	rm -rf $(PRODUCT_DISK1)
	rm -f $(CACHE)/disk1.dep


# -----------------------------------------------------------------------------
# rule to build product disk 1
# -----------------------------------------------------------------------------

$(CACHE)/disk1.dep: $(DISK1_BINARY) $(DISK1_ASSETS)
	@echo "GENERATING DISK $(PRODUCT_DISK1)"
	@mkdir -p $(CACHE)
	@mkdir -p $(PRODUCT_DISK1)/auto
	cp -r $(DISK1_BINARY) $(PRODUCT_DISK1)/auto/.
	$(if $(strip $(DISK1_FILES)),$(AGTROOT)/scripts/copyassets.sh $(ASSETS) "$(DISK1_FILES)" ../$(PRODUCT_DISK1)/)
	@touch $(CACHE)/disk1.dep

DISK1_FILES = $(DISK1_ASSETS:$(ASSETS)/%=%)

# -----------------------------------------------------------------------------

assets: assets_common

# -----------------------------------------------------------------------------
# build assets from source assets
# -----------------------------------------------------------------------------
# - if sources change or asset intermediates are removed, rebuild them
# -----------------------------------------------------------------------------

# common assets (typically on disk1)

$(CACHE)/assets_common.dep: $(CACHE)/asset_tree.dep $(SOURCE_ASSETS_COMMON) 
	@echo "BUILDING COMMON ASSETS: $(PROJECT)"
	./assets_com.sh
	@touch $(CACHE)/assets_common.dep

$(ASSETS_COMMON): $(CACHE)/assets_common.dep

assets_common: $(ASSETS_COMMON)


# prep asset directory tree before making any assets
$(CACHE)/asset_tree.dep:
	@mkdir -p $(ASSETS)
	@mkdir -p $(CACHE)
	@touch $(CACHE)/asset_tree.dep

# -----------------------------------------------------------------------------
# treat these names as build targets, not sourcefiles
# -----------------------------------------------------------------------------
.PHONY: image

# =============================================================================

# clean just the asset cache tags
clean-cache:
	rm -f $(CACHE)/*.dep

# clean just the assets
clean-assets: clean-cache
	rm -rf $(ASSETS)

# clean everything
clean-all: clean clean-assets
	rm -rf $(PRODUCT_DISK1)
	rm -rf $(IMAGES)
	rm -rf $(CACHE)
	
cleanall: clean-all

# =============================================================================
#	Build machinery
# =============================================================================

include $(AGTROOT)/makerules

# =============================================================================
#
# =============================================================================
