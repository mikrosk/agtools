#==============================================================================
#	Wrap asset using a packer or a simple CRC
#==============================================================================
#	supported wrapping modes:
#	- wrap		wrap with CRC, no compression
#	- lz77		low compression, fast decompression
#	- pk2e		good compression, fair decompression
#	- arj4		very good compression, slow decompression
#	- arj7		best compression, slow decompression
#==============================================================================

#AGTROOT=../..

# locate AGTBIN for this host
#AGTBIN=${AGTROOT}/bin/`${AGTROOT}/config.sh`
#echo "configured AGT native tools @" ${AGTBIN}

#==============================================================================

DIR=$1
SRC=$2
DST=$3

echo "transfer assets ${SRC} -> ${DST}"

cd ${DIR} && cp -r --parents ${SRC} ${DST}
