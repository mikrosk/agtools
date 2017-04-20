# create a superpalette from all art assets (only needs done once, or after significant art changes)
# WARNING: this is a compute-intensive step. it takes time. best disabled once the palette is produced.

#../../bin/pcs -cd ste -ccfields 1 -ccmode 3 -ccrounds 32 -ccthreads 6 -ccsat 1.0 -ccincap 8192 -ccl 0=0:0:0 -ccl 15=f:f:f -ccpopctrl 0.5 -ccapc 0.75 -ccdpc 0.75 -v map-2.png

# cut background map & tiles

../../bin/agtcut -vis -cm tiles -ts 8 -t 0.0 -ccf 1 -bp 4 -om direct -s map-2.png -o map8x8.cct -p chunky8.ccs

