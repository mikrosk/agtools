# create a superpalette from all art assets (only needs done once, or after significant art changes)
# WARNING: this is a compute-intensive step. it takes time. best disabled once the palette is produced.

#../../bin/pcs -cd ste -ccfields 2 -ccmode 3 -ccrounds 32 -ccthreads 6 -ccsat 0.75 -ccincap 8192 -ccpopctrl 0.5 -ccapc 0.75 -ccdpc 0.75 -v world.png trees-2m.png

# cut background map & tiles

# cut the base layer
../../bin/agtcut -vis -cm tiles -ts 16 -t 0.0 -ccf 2 -bp 4 -om direct -s world.png -o map.cct -p chunky8.ccs

# cut the overlay/transparent layer with mask
../../bin/agtcut -vis -cm tiles -ts 16 -layer -t 0.0 -ccf 2 -bp 4 -om direct -s trees-2m.png -o mapo.cct -p chunky8.ccs
