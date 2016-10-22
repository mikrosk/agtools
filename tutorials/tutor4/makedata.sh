
# cut 4 sprites into 2 different formats:
# X = IMSPR (because larger than 32 wide)
# e,n,o = EMXSPR (faster for <= 32 wide)

../../bin/agtcut -cm imspr -gm -prv -vis -s tutor4_spr.png -crn 224 -p xpal.pi1 -o x.ims -sxp 0 -syp 0 -sxs 54 -sys 42 -sc 1
../../bin/agtcut -cm emxspr -prv -vis -s tutor4_spr.png -crn 224 -p xpal.pi1 -o e.emx -sxp 0 -syp 71 -sxs 21 -sys 23 -sc 1
../../bin/agtcut -cm emxspr -prv -vis -s tutor4_spr.png -crn 224 -p xpal.pi1 -o n.emx -sxp 0 -syp 48 -sxs 25 -sys 23 -sc 1
../../bin/agtcut -cm emxspr -prv -vis -s tutor4_spr.png -crn 224 -p xpal.pi1 -o o.emx -sxp 0 -syp 94 -sxs 25 -sys 23 -sc 1


# cut background map & tiles from [tutor4_map.png]
#
# note: produces [tutor3.cct], [tutor3.ccm]

../../bin/agtcut -cm tiles -vis -t 0.0 -ccf 1 -bp 4 -om direct -s tutor4_map.png -crn 224 -p xpal.pi1 -o tutor4.cct

