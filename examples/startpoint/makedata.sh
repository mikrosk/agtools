# 16c superpalette & dither map
#
#../../bin/pcs -cd ste -ccfields 2 -ccmode 3 -ccignore ff:00:ff -ccrounds 3 -ccthreads 6 -ccsat 0.75 -ccincap 4096 -ccpopctrl 0.5 -ccdcs 0.01 -ccapc 0.75 -ccdpc 0.75 -v bg.png thingy.png

# thingy sprite
#
../../bin/agtcut -cm emspr -prv -vis -sg thingy.sgd -s thingy.png -crn 224 -p chunky8.ccs -o thingy.ems

# cut background map & tiles from [bg.png]
#
# note: produces [bg.cct], [bg.ccm]
#
../../bin/agtcut -cm tiles -v -vis -t 0.0 -ccf 1 -bp 4 -om direct -s bg.png -crn 224 -p chunky8.ccs -o bg.cct

