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

AGTROOT=../..

# locate AGTBIN for this host
AGTBIN=${AGTROOT}/bin/`${AGTROOT}/config.sh`
echo "configured AGT native tools @" ${AGTBIN}

#==============================================================================

ASSET=$1
MODE=$2

echo "wrapping ${ASSET} with ${MODE}"

case $MODE in

  "wrap")
    ${AGTBIN}/packwrap ${ASSET} ${ASSET} ${ASSET} wrap
    ;;

  "lz77")
    ${AGTBIN}/pack2e ${ASSET} ${ASSET}.lz77
    ${AGTBIN}/packwrap ${ASSET} ${ASSET}.lz77 ${ASSET} lz77
    ;;

  "pk2e")
    ${AGTBIN}/pack2e ${ASSET} ${ASSET}.pk2e
    ${AGTBIN}/packwrap ${ASSET} ${ASSET}.pk2e ${ASSET} pk2e
    ;;

  "arj4")
    ${AGTBIN}/pack2e ${ASSET} ${ASSET}.arj4
    ${AGTBIN}/packwrap ${ASSET} ${ASSET}.arj4 ${ASSET} arj4
    ;;

  "arj7")
    ${AGTBIN}/pack2e ${ASSET} ${ASSET}.arj7
    ${AGTBIN}/packwrap ${ASSET} ${ASSET}.arj7 ${ASSET} arj7
    ;;

  *)
    @echo "unknown wrapping code: ${MODE}"
    ;;

esac
