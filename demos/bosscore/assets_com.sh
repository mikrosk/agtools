#==============================================================================
#	Build sprite assets
#==============================================================================

AGTROOT=../..

# output dir
OUT=assets

# location of source assets
SRC=source_assets

# dither or dualfield colour translation (from PCS), or ST palette source
COLMAP="source_assets/xenpal.pi1"

# locate AGTBIN for this host
AGTBIN=${AGTROOT}/bin/`${AGTROOT}/config.sh`
echo "configured AGT native tools @" ${AGTBIN}

# working directory for AGTCUT/EMX codegen tasks
GENTEMP=./build
mkdir -p ${GENTEMP}

PACK=${AGTROOT}/scripts/pack.sh


#==============================================================================
#	Map cutting config
#==============================================================================

# we can pass these to all tasks
MAP_DEFAULTS="-t 0.33 -bp 4 -v -vis"

# composite cutting command for all maps
MAP="${AGTBIN}/agtcut -cm tiles -om direct ${MAP_DEFAULTS} -p ${COLMAP}"

#------------------------------------------------------------------------------
#	Map
#------------------------------------------------------------------------------

# cut map & tiles, single-field
${MAP} -o ${OUT}/bosscore.ccm -s ${SRC}/map3.png

# optionally compress the map & tileset
${PACK} ${OUT}/bosscore.ccm pk2e
${PACK} ${OUT}/bosscore.cct pk2e

#==============================================================================
#	Sprite cutting config
#==============================================================================

# for diagnostics, verbosity etc. #-v -q
DEBUG="-prv -vis"	

# keycolour, tolerance
KEYCFG="-keyrgb ff:00:ff -kg -kt 0.01"

# we can pass these to all tasks
SPR_DEFAULTS="-crn 224 -gm -bld ${GENTEMP}"

# default optimization level (lower value for faster cutting, quicker turnaround)
DEFAULT_OPTI=""
OPTI="-emxopt 20"

#------------------------------------------------------------------------------
# composite cutting commands for most cases
#------------------------------------------------------------------------------

# composite cutting command for all sprites
SPR="${AGTBIN}/agtcut ${SPR_DEFAULTS} -p ${COLMAP} ${KEYCFG} ${DEBUG} ${DEFAULT_OPTI}"
SLS="${AGTBIN}/agtcut -cm slabs -p ${COLMAP} ${KEYCFG} ${DEBUG}"
SLR="${AGTBIN}/agtcut -cm slabrestore -p ${COLMAP} ${KEYCFG} ${DEBUG}"

#------------------------------------------------------------------------------
#	Sprites
#------------------------------------------------------------------------------
# -nc = no clipping (saves memory)

# boss
${SLS} -o ${OUT}/bosscore.sls -s ${SRC}/xenboss.png -sxp 0 -syp 0 -sxs 144 -sys 96 -sc 1
${SLR} -o ${OUT}/bosscore.slr -s ${SRC}/xenboss.png -sxp 0 -syp 0 -sxs 144 -sys 96 -sc 1

# boss parts
${SPR} -o ${OUT}/bosseye.emx -s ${SRC}/bosseye.png -sxp 0 -syp 0 -sxs 22 -sys 16 -sxi 22 -syi 0 -sc 6	-cm emxspr 
${SPR} -o ${OUT}/bosspulp.emx -s ${SRC}/bosspulp.png -sxp 0 -syp 0 -sxs 30 -sys 8 -sxi 0 -syi 8 -sc 6	-cm emxspr -nc 

# boss weapons
${SPR} -o ${OUT}/balls.emx -s ${SRC}/balls.png -sxp 0 -syp 0 -sxs 10 -sys 10 -sxi 0 -syi 10 -sc 9	-cm emxspr -nc 
${SPR} -o ${OUT}/beam.emx -s ${SRC}/sprites.png -sxp 361 -syp 162 -sxs 30 -sys 4 -sc 1			-cm emxspr -nc 

# player
${SPR} -o ${OUT}/vicviper.emx -s ${SRC}/player.png -sxp 46 -syp 2 -sxs 23 -sys 16 -syi 18 -sc 9		-cm emxspr 
${SPR} -o ${OUT}/bullet.emx -s ${SRC}/sprites.png -sxp 172 -syp 28 -sxs 9 -sys 2 -sxi 12 -sc 2		-cm emxspr -nc 

# compress assets with pk2e
${PACK} ${OUT}/bosscore.sls pk2e
${PACK} ${OUT}/bosscore.slr pk2e
${PACK} ${OUT}/bosseye.emx pk2e
${PACK} ${OUT}/bosspulp.emx pk2e
${PACK} ${OUT}/balls.emx pk2e
${PACK} ${OUT}/beam.emx pk2e
${PACK} ${OUT}/vicviper.emx pk2e
${PACK} ${OUT}/bullet.emx pk2e
