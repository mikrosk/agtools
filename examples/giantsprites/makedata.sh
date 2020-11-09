
# cut sprites
# note: project uses x-guardband of 64 pixels, so use '-emgx 64' to generate correct EMX sprites

#
# one small sprite (<=32 wide)
#
../../bin/agtcut -cm emxspr           -emgx 64 -prv -vis -s b32w.png -crn 224 -p xpal.pi1 -o b32w.emx -sxp 0 -syp 0 -sxs 32 -sys 31 -sc 1

#
# two big sprites (>32 wide)
#
../../bin/agtcut -cm emxspr      -emc -emgx 64 -prv -vis -s b48w.png -crn 224 -p xpal.pi1 -o b48w.emx -sxp 0 -syp 0 -sxs 48 -sys 54 -sc 1
../../bin/agtcut -cm emxspr      -emc -emgx 64 -prv -vis -s bug64w.png -crn 224 -p xpal.pi1 -o b64w.emx -sxp 0 -syp 0 -sxs 64 -sys 72 -sc 1

#
# two giant sprites, with occlusion maps
#
../../bin/agtcut -cm emxspr -ocm -emc -emgx 64 -prv -vis -s bug80w.png -crn 224 -p xpal.pi1 -o b80w.emx -sxp 0 -syp 0 -sxs 80 -sys 105 -sc 1
../../bin/agtcut -cm emxspr -ocm -emc -emgx 64 -prv -vis -s xenboss.png -crn 224 -p xpal.pi1 -o b144w.emx -sxp 0 -syp 0 -sxs 144 -sys 96 -sc 1


# cut background map & tiles from [bg_map.png]
#
# note: produces [bg.cct], [bg.ccm]

../../bin/agtcut -cm tiles -v -vis -t 0.0 -ccf 1 -bp 4 -om direct -s bg_map.png -crn 224 -p xpal.pi1 -o bg.cct

