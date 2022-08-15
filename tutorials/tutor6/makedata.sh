# create a superpalette from all art assets (only needs done once, or after significant art changes)
# WARNING: this is a compute-intensive step. it takes time. best disabled once the palette is produced.

#../../bin/pcs -cd ste -ccfields 2 -ccmode 3 -ccrounds 16 -ccthreads 6 -ccsat 1.0 -ccincap 1024 -ccl 0=0:0:0 -ccl 15=f:f:f -ccignore 90:c8:e8 -ccignore 00:ff:00 -ccpopctrl log -ccapc 0.5 -ccdpc 0.3 -ccdcs 0.06 -v karin5515_ac.png bg.jpg


# cut sprites

../../bin/agtcut -cm emspr -emc -vis -prv -ccr high -keyrgb 90:c8:e8 -kg -kt 0.01 -gm -s karin5515_ac.png -o karin.ems -p chunky8.ccs -ackeyrgb 00:ff:00

# cut background map & tiles

../../bin/agtcut -cm tiles -vis -t 0.0 -ccf 1 -bp 4 -om direct -s tutor6_map.png -crn 224 -p xpal.pi1 -o tutor6.cct
../../bin/agtcut -cm tiles -vis -t 0.0 -ccf 2 -bp 4 -om direct -s bg.jpg -crn 224 -p chunky8.ccs -o tutor6.cct

