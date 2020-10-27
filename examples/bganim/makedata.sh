# create a superpalette from all art assets (only needs done once, or after significant art changes)
# WARNING: this is a compute-intensive step. it takes time. best disabled once the palette is produced.

../../bin/pcs -cd ste -ccfields 1 -ccmode 3 -ccrounds 1 -ccthreads 1 -ccsat 0.75 -ccincap 8192 -ccpopctrl 0.5 -ccapc 0.75 -ccdpc 0.75 -v bg.png bganim.png
#../../bin/pcs -cd ste -ccfields 2 -ccmode 3 -ccrounds 32 -ccthreads 6 -ccsat 0.75 -ccincap 8192 -ccpopctrl 0.5 -ccapc 0.75 -ccdpc 0.75 -v world.png trees-2m.png

# cut background map & tiles

# cut two independent maps (.ccm) into a single shared tileset (.cct) via PCS-generated colourmapping (.ccs)
../../bin/agtcut -vis -cm tiles -ts 16 -t 0.0 -ccf 1 -bp 4 -om direct -s bg.png -s bganim.png -o shared.cct -p chunky8.ccs

# cut sprite (autocut)
../../bin/agtcut -cm emspr -ackeyrgb ff:ff:00 -keyrgb ff:00:ff -prv -vis -s sprite.png -crn 224 -p chunky8.ccs -o sprite.ems
