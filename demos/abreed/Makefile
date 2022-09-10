# =============================================================================
#	Base build config
# =============================================================================

# project-relative location of agtools repo root dir
# note: can remove this and set $AGTROOT var externally if preferred
AGTROOT = ../..

# project name - mainly for layout - target binaries can be individually named
PROJECT := abreed

# debug/testing/release
BUILD_CFG = testing

# remote testing post-build step, if required [ultradev,hxcusb]
POST_REMOTE = ultradev
REMOTE_PRG = coop.prg

# make job can specify one AGT 'architecture' to target at a time (mainly STE)
AGLIB=$(AGLIB_STE)
AGLIB_OBJ=$(AGLIB_STE_OBJ)

PACK=${AGTROOT}/scripts/pack.sh

# =============================================================================
#	Optional build config
# =============================================================================

# if yes, will rebuild colourmaps on changes. otherwise only build if missing
# notes: 
# - colourmap generation can be expensive to run - can take several minutes
# - some generate modes are nondeterministic, not guaranteed same result every time
# - projects with prebuilt colourmaps will be committed with 'no'
REBUILD_COLMAPS = no

# links Hatari natfeats lib into program
USE_HATARI_NATFEATS = no


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
		-DAGT_CONFIG_PACKER_ANY \


# =============================================================================
#	Debugging, diagnostic logging & profiling options
# =============================================================================
# Notes:
#     - see Wiki: https://bitbucket.org/d_m_l/agtools/wiki/Compiletime%20Flags
# -----------------------------------------------------------------------------

COMPILERDEFS += \
#		-DTIMING_RASTERS -DTIMING_RASTERS_MAIN \
#		-DAGT_CONFIG_PRINT_PLAYFIELD_METRICS \
#		-DAGT_CONFIG_SAFETY_CHECKS \
#		-DAGT_CONFIG_DEBUGDRAW \
#		-DUSE_MEMTRACE \

#		-DAGT_CONFIG_STEEM_DEBUG \
#		-DAGT_CONFIG_DEBUG_CONSOLE \

# =============================================================================
#	Additional project-local sourcefiles (if any)
# =============================================================================

PROJECT_SRC_LIST = \

PROJECT_INC_LIST = \

# =============================================================================
#	Project assets
# =============================================================================

# assets common to all stages
COM_SRC = source_assets

# if colourmap is required for asset conversion, specify it here
COLMAP = $(COM_SRC)/colmap.ccs

# input files which cause colourmaps to be rebuilt with [REBUILD_COLMAPS = yes]
# NOTE: optional, to trigger auto-rebuild of colourmaps when these change

COLMAP_SOURCES = \
	$(COM_SRC)/ab1x.png \

# source asset list is optional - triggers assets_common.sh
SOURCE_ASSETS_COMMON = \
	$(COM_SRC/ab1x.png \
	$(COM_SRC)/sprites.png \
	$(COM_SRC)/solidtiles.csv \

ASSETS_COMMON = \
	$(ASSETS)/abreed.ccm \
	$(ASSETS)/abreed0.cct \
	$(ASSETS)/abreed1.cct \
	$(ASSETS)/monster.ems \

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
all: disk1 disk2


# -----------------------------------------------------------------------------
# build disk images using external script
# -----------------------------------------------------------------------------
images: $(CACHE)/disk1.tag $(CACHE)/disk2.tag
	$(AGTROOT)/scripts/post_image.sh $(PRODUCT_DISK1) $(IMAGES) disk1
	$(AGTROOT)/scripts/post_image.sh $(PRODUCT_DISK2) $(IMAGES) disk2
.PHONY: images

# -----------------------------------------------------------------------------
# define content of disk1
# -----------------------------------------------------------------------------

DISK1_BINARY = $(BUILD_DIR)/coop.prg
DISK1_ASSETS = $(ASSETS_COMMON)

disk1: $(CACHE)/disk1.tag
.PHONY: disk1

clean-disk1:
	rm -rf $(PRODUCT_DISK1)
	rm -f $(CACHE)/disk1.tag

# -----------------------------------------------------------------------------
# define content of disk2
# -----------------------------------------------------------------------------

DISK2_BINARY = $(BUILD_DIR)/abreed.prg
DISK2_ASSETS = $(ASSETS_COMMON)

disk2: $(CACHE)/disk2.tag
.PHONY: disk2

clean-disk2:
	rm -rf $(PRODUCT_DISK2)
	rm -f $(CACHE)/disk2.tag

# -----------------------------------------------------------------------------
# rule to build product disk 1
# -----------------------------------------------------------------------------

$(CACHE)/disk1.tag: $(DISK1_BINARY) $(DISK1_ASSETS)
	@echo "GENERATING DISK $(PRODUCT_DISK1)"
	@mkdir -p $(CACHE)
	@mkdir -p $(PRODUCT_DISK1)/auto
	cp -r $(DISK1_BINARY) $(PRODUCT_DISK1)/auto/.
	$(if $(strip $(DISK1_FILES)),$(AGTROOT)/scripts/copyassets.sh $(ASSETS) "$(DISK1_FILES)" ../$(PRODUCT_DISK1)/)
	@touch $(CACHE)/disk1.tag

DISK1_FILES = $(DISK1_ASSETS:$(ASSETS)/%=%)

# -----------------------------------------------------------------------------
# rule to build product disk 2
# -----------------------------------------------------------------------------
$(CACHE)/disk2.tag: $(DISK2_BINARY) $(DISK2_ASSETS)
	@echo "GENERATING DISK $(PRODUCT_DISK2)"
	@mkdir -p $(CACHE)
	@mkdir -p $(PRODUCT_DISK2)/auto
	cp -r $(DISK2_BINARY) $(PRODUCT_DISK2)/auto/.
	$(if $(strip $(DISK2_FILES)),$(AGTROOT)/scripts/copyassets.sh $(ASSETS) "$(DISK2_FILES)" ../$(PRODUCT_DISK2)/)
	@touch $(CACHE)/disk2.tag

DISK2_FILES = $(DISK2_ASSETS:$(ASSETS)/%=%)

# -----------------------------------------------------------------------------

assets: assets_common

# -----------------------------------------------------------------------------
# build assets from source assets
# -----------------------------------------------------------------------------
# - if sources change or asset intermediates are removed, rebuild them
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# common assets (typically on disk1)
# -----------------------------------------------------------------------------

$(CACHE)/assets_common.tag: $(CACHE)/asset_tree.tag $(CACHE)/colmap.tag $(SOURCE_ASSETS_COMMON) 
	@echo "BUILDING COMMON ASSETS: $(PROJECT)"
	./assets_com.sh
	@touch $(CACHE)/assets_common.tag

$(ASSETS_COMMON): $(CACHE)/assets_common.tag

assets_common: $(ASSETS_COMMON)
.PHONY: assets_common


# -----------------------------------------------------------------------------
# generate colourmaps - manually invoked as necessary
# -----------------------------------------------------------------------------
# 	optionally rebuild on changes
# -----------------------------------------------------------------------------
$(CACHE)/colmap_change.tag: $(CACHE)/asset_tree.tag $(COLMAP_SOURCES)
ifeq ($(REBUILD_COLMAPS),yes)
	@echo "REBUILDING COLOURMAPS: $(PROJECT)"
	./colourmaps.sh
endif
	@touch $(CACHE)/colmap_change.tag
# -----------------------------------------------------------------------------
# 	build only if missing (e.g. not under source control)
# -----------------------------------------------------------------------------
$(COLMAP):
	@echo "BUILDING MISSING COLOURMAP: $(PROJECT)"
	@mkdir -p $(ASSETS)
	@mkdir -p $(CACHE)	
	./colourmaps.sh
	@touch $(CACHE)/colmap_change.tag

# -----------------------------------------------------------------------------
# 	combined build rule for colourmap refresh
# -----------------------------------------------------------------------------
$(CACHE)/colmap.tag: $(COLMAP) $(CACHE)/colmap_change.tag
	@touch $(CACHE)/colmap.tag

colourmaps: $(CACHE)/colmap.tag
.PHONY: colourmaps

# -----------------------------------------------------------------------------

# prep asset directory tree before making any assets
$(CACHE)/asset_tree.tag:
	@mkdir -p $(ASSETS)
	@mkdir -p $(CACHE)
	@touch $(CACHE)/asset_tree.tag

# -----------------------------------------------------------------------------

assets: assets_common
.PHONY: assets

# =============================================================================
#	transfer cross-project assets
# =============================================================================
# nothing required here...
# =============================================================================

# =============================================================================
#	cleaning rules
# =============================================================================

# clean just the asset cache tags
clean-cache:
	rm -f $(CACHE)/*.tag

# clean just the assets
clean-assets: clean-cache
	rm -rf $(ASSETS)

clean-images:
	rm -rf $(IMAGES)


# clean everything
clean-all: clean clean-assets
	rm -rf $(PRODUCT_DISK1)
	rm -rf $(PRODUCT_DISK2)
	rm -rf $(IMAGES)
	rm -rf $(CACHE)

# =============================================================================
#	Build machinery
# =============================================================================

include $(AGTROOT)/makerules

# =============================================================================
#
# =============================================================================

