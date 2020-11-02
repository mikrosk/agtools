# create a superpalette from all art assets (only needs done once, or after significant art changes)
# WARNING: this is a compute-intensive step. it takes time. best disabled once the palette is produced.

#../../bin/pcs -cd ste -ccmode 3 -ccrounds 6 -ccthreads 6 -ccsat 1.0 -ccincap 8192 -ccl 0=0:0:0 -ccl 15=f:f:f -ccpopctrl 0.5 -ccapc 0.75 -ccdpc 0.75 -ccdcs 0.0001 -ccdls 0.1 -v tmap256.png


# cut background map & tiles

../../bin/agtcut -cm tiles -t 0.75 -ccf 2 -bp 4 -om direct -s tmap256.png -o isoscrl.cct -p chunky8.ccs


# cut sprite [emsxpr] : optimized [E]nd [M]ask sprites : up to 32 pixels wide

# for this example we'll cut using a spriteguide file (villager.sgd), which overrides cutting commands with a simple script

# this version cuts single-field colour sprite, biased for the high brightness field (-ccr high)
#../../bin/agtcut -cm emxspr -vis -prv -ccr high -keyrgb ff:00:ff -kg -kt 0.01 -gm -s villager.png -o villager.emx -p chunky8.ccs -sg villager.sgd

# this version cuts a dual-field colour sprite, incorporating both low/high colour fields interlaced @ 50hz
../../bin/agtcut -cm emxspr -ccf 2 -vis -prv -keyrgb ff:00:ff -kg -kt 0.01 -s villager.png -o villager.emx -p chunky8.ccs -sg villager.sgd

