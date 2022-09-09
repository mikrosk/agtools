# =============================================================================
#	Base build config
# =============================================================================

# project-relative location of agtools repo root dir
# note: can remove this and set $AGTROOT var externally if preferred
AGTROOT = ../..

# project name - mainly for layout - target binaries can be individually named
PROJECT := hshmup

# debug/testing/release
BUILD_CFG = testing

# remote testing post-build step, if required [ultradev,hxcusb]
POST_REMOTE = ultradev
REMOTE_PRG = hshmup.prg

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
		-DENABLE_AGT_AUDIO \

#		-DAGT_CONFIG_SYS_GUARDY=0 \
#		-DAGT_CONFIG_STF_OPT_SIZE \
#		-DAGT_CONFIG_VISTRIGGER \
#		-DAGT_CONFIG_DYNAMIC_TICKPRIO \

# =============================================================================
#	Debugging, diagnostic logging & profiling options
# =============================================================================
# Notes:
#     - see Wiki: https://bitbucket.org/d_m_l/agtools/wiki/Compiletime%20Flags
# -----------------------------------------------------------------------------

COMPILERDEFS += \
#		-DTIMING_RASTERS -DTIMING_RASTERS_MAIN \
#		-DAGT_CONFIG_STEEM_DEBUG \
#		-DAGT_CONFIG_SAFETY_CHECKS \
#		-DAGT_CONFIG_DEBUG_CONSOLE \
#		-DAGT_CONFIG_DEBUGDRAW \
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
	${COM_SRC}/sprites.png \
	${COM_SRC}/player.png \

ASSETS_COMMON = \
	$(ASSETS)/vicviper.emx \
	$(ASSETS)/options.emx \
	$(ASSETS)/lasers.emx \
	$(ASSETS)/dlasers.emx \
	$(ASSETS)/plasers.emx \
	$(ASSETS)/flames.emx \
	$(ASSETS)/powerup.emx \
	$(ASSETS)/bullet.emx \
	$(ASSETS)/bulletd.emx \
	$(ASSETS)/missile.emx \
	$(ASSETS)/sparks.emx \
	\
	$(ASSETS)/sounds/pasnar.wav \
	$(ASSETS)/sounds/pbass1.wav \
	$(ASSETS)/sounds/pbass2.wav \
	$(ASSETS)/sounds/pccym1.wav \
	$(ASSETS)/sounds/pccym2.wav \
	$(ASSETS)/sounds/pchcym.wav \
	$(ASSETS)/sounds/pchhat.wav \
	$(ASSETS)/sounds/pclap.wav \
	$(ASSETS)/sounds/pesnar.wav \
	$(ASSETS)/sounds/phftom.wav \
	$(ASSETS)/sounds/phitom.wav \
	$(ASSETS)/sounds/phmtom.wav \
	$(ASSETS)/sounds/plftom.wav \
	$(ASSETS)/sounds/plmtom.wav \
	$(ASSETS)/sounds/plotom.wav \
	$(ASSETS)/sounds/pohhat.wav \
	$(ASSETS)/sounds/pphhat.wav \
	$(ASSETS)/sounds/prbell.wav \
	$(ASSETS)/sounds/prcym1.wav \
	$(ASSETS)/sounds/prcym2.wav \
	$(ASSETS)/sounds/pspcym.wav \
	$(ASSETS)/sounds/ptambo.wav \


# assets specific to stage 1
S1_SRC = source_assets

# source asset list is optional - triggers assets_s1.sh
SOURCE_ASSETS_S1 = \
	${S1_SRC}/map.png \
	${S1_SRC}/sprites.png \

ASSETS_S1 = \
	$(ASSETS)/demo1.ccm \
	$(ASSETS)/demo10.cct \
	$(ASSETS)/demo11.cct \
	\
	$(ASSETS)/amoeba.emx \
	$(ASSETS)/spinny.emx \
	$(ASSETS)/square.emx \
	$(ASSETS)/drone.emx \
	$(ASSETS)/cylon.emx \
	$(ASSETS)/bee.emx \
	$(ASSETS)/fly.emx \
	$(ASSETS)/fly2.emx \
	$(ASSETS)/tie.emx \
	$(ASSETS)/quad.emx \
	$(ASSETS)/pellets.emx \
	$(ASSETS)/diamonds.emx \
	$(ASSETS)/blobs.emx \
	$(ASSETS)/jelly.emx \
	$(ASSETS)/beam.emx \
	$(ASSETS)/walker.emx \
	\
	$(ASSETS)/music/salslash.bmu \


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
images: $(CACHE)/disk1.tag #$(CACHE)/disk2.tag
	$(AGTROOT)/scripts/post_image.sh $(PRODUCT_DISK1) $(IMAGES) disk
	#$(AGTROOT)/scripts/post_image.sh $(PRODUCT_DISK2) $(IMAGES) disk2
.PHONY: images

# -----------------------------------------------------------------------------
# define content of disk1
# -----------------------------------------------------------------------------

DISK1_BINARY = $(BUILD_DIR)/hshmup.prg
DISK1_ASSETS = $(ASSETS_COMMON) $(ASSETS_S1)

disk1: $(CACHE)/disk1.tag
.PHONY: disk1

clean-disk1:
	rm -rf $(PRODUCT_DISK1)
	rm -f $(CACHE)/disk1.tag


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
# build assets from source assets
# -----------------------------------------------------------------------------
# - if sources change or asset intermediates are removed, rebuild them
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# common assets (typically on disk1)
# -----------------------------------------------------------------------------

$(CACHE)/assets_common.tag: $(CACHE)/asset_tree.tag $(SOURCE_ASSETS_COMMON) 
	@echo "BUILDING COMMON ASSETS: $(PROJECT)"
	./assets_com.sh
	@touch $(CACHE)/assets_common.tag

$(ASSETS_COMMON): $(CACHE)/assets_common.tag

assets_common: $(ASSETS_COMMON)
.PHONY: assets_common


# -----------------------------------------------------------------------------
# stage1 assets (typically on disk1 or maybe disk2)
# -----------------------------------------------------------------------------

$(CACHE)/assets_s1.tag: $(CACHE)/asset_tree.tag $(SOURCE_ASSETS_S1) 
	@echo "BUILDING S1 ASSETS: $(PROJECT)"
	./assets_s1.sh
	@touch $(CACHE)/assets_s1.tag

$(ASSETS_S1): $(CACHE)/assets_s1.tag

assets_s1: $(ASSETS_S1)
.PHONY: assets_s1


# -----------------------------------------------------------------------------

# prep asset directory tree before making any assets
$(CACHE)/asset_tree.tag:
	@mkdir -p $(ASSETS)
	@mkdir -p $(ASSETS)/sounds
	@mkdir -p $(ASSETS)/music
	@mkdir -p $(CACHE)
	@touch $(CACHE)/asset_tree.tag

# -----------------------------------------------------------------------------

assets: assets_common assets_s1
.PHONY: assets

# =============================================================================
#	transfer cross-project assets
# =============================================================================

# rule to transfer common samples to assets dir
$(ASSETS)/sounds/%.wav: ../../data/sounds/%.wav
	cp $< $@

# rule to transfer shared music to assets dir
$(ASSETS)/music/%.bmu: ../../data/music/%.bmu
	cp $< $@

# rule to pack shared music to assets dir
$(ASSETS)/music/%.bmz: ../../data/music/%.bmu
	cp $< $@
	${PACK} $@ pk2e

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

