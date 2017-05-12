mkdir stf

# create a superpalette from all art assets (only needs done once, or after significant art changes)
# WARNING: this is a compute-intensive step. it takes time. best disabled once the palette is produced.

#../../bin/pcs -cd ste -ccmode 3 -ccrounds 6 -ccthreads 6 -ccsat 1.0 -ccincap 8192 -ccl 0=0:0:0 -ccl 15=f:f:f -ccpopctrl 0.5 -ccapc 0.75 -ccdpc 0.75 -ccdcs 0.0001 -ccdls 0.1 -v ab1x.png


# cut background map & tiles

../../bin/agtcut -vis -cm tiles -t 0.25 -ccf 2 -bp 4 -om direct -s ab1x.png -ttags solidtiles.csv -o stf/abreed.cct -p chunky8.ccs


# cut the monster sprite: 8 rotation frames x 4 animation frames = 32 cuts

../../bin/agtcut -cm imspr -nogm -key 0 -ccr low -prv -keyrgb ff:00:ff -kg -kt 0.01 -s sprites-stf.png -o stf/monster.ims -p chunky8.ccs -sxp 0 -syp 256 -sxs 32 -sys 32 -sxi 32 -syi 32 -sxc 8 -syc 4
