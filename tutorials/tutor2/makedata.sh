
# use PhotoChrome's GA colour reducer to produce 16-colour target palette from 35-colour [tutor2.png]
#
# note: produces [chunky8.cc?] files used to quickly remap RGB to chunky indexed 16-colour
# note: configured to lock colour 0=000, colour 15=fff
# note: adjust -ccthreads to suit the number of threading cores on your CPU (4-core + hyperthreading = 6 threads optimal)
# note: for extra giggles, try reducing [tutor2.png] to 16 colours, undithered with another popular program of choice. emjoy!

../../bin/pcs -cd ste -ccmode 3 -ccfields 1 -ccrounds 10 -ccthreads 6 -ccsat 1.0 -ccincap 8192 -ccl 0=0:0:0 -ccl 15=f:f:f -ccpopctrl 0.5 -ccapc 1.0 -ccdpc 1.0 tutor2.png


# cut background map & tiles from [tutor2.png]
#
# note: produces [tutor2.cct], [tutor2.ccm]
# note: produces [tutor2_mapvis.tga] preview image for inspection

../../bin/agtcut -cm tiles -vis -t 0.0 -ccf 1 -bp 4 -om direct -s tutor2.png -p chunky8.ccs -o tutor2.cct

