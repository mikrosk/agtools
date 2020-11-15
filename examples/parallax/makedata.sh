# create a superpalette from all art assets (only needs done once, or after significant art changes)
# WARNING: this is a compute-intensive step. it takes time. best disabled once the palette is produced.

#../../bin/pcs -cd ste -ccmode 3 -ccrounds 1 -ccthreads 6 -ccsat 0.75 -ccincap 1024 -ccl 0=0:0:0 -ccpopctrl 0.75 -ccdcs 0.1 -ccdls 0.1 -v map.png


# cut background map & tiles

../../bin/agtcut -vis -cm tiles -t 0.05 -ccf 2 -bp 4 -om direct -s map3.png -o map.cct -p chunky8.ccs
../../bin/agtcut -vis -cm tiles -t 0.25 -ccf 2 -bp 4 -om direct -s mapf.png -o mapf.cct -p chunky8.ccs


# cut the monster sprite: 8 rotation frames x 4 animation frames = 32 cuts

#../../bin/agtcut -cm emxspr -ccf 2 -prv -ackeyrgb ff:ff:00 -keyrgb ff:00:ff -kg -kt 0.01 -s object.png -o object.emx -p chunky8.ccs
../../bin/agtcut -cm emspr -ccf 2 -prv -ackeyrgb ff:ff:00 -keyrgb ff:00:ff -kg -kt 0.01 -s object.png -o object.ems -p chunky8.ccs
