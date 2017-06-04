mkdir -f stf

# cut 4 sprites into IMSPR format for STF with no stored mask (-nogm) and transparent colour = 0 (-key 0)

../../bin/agtcut -cm imspr -nogm -key 0 -prv -vis -s tutor4_spr.png -crn 224 -p xpal.pi1 -o stf/x.ims -sxp 0 -syp 0 -sxs 54 -sys 42 -sc 1
../../bin/agtcut -cm imspr -nogm -key 0 -prv -vis -s tutor4_spr.png -crn 224 -p xpal.pi1 -o stf/e.ims -sxp 0 -syp 71 -sxs 21 -sys 23 -sc 1
../../bin/agtcut -cm imspr -nogm -key 0 -prv -vis -s tutor4_spr.png -crn 224 -p xpal.pi1 -o stf/n.ims -sxp 0 -syp 48 -sxs 25 -sys 23 -sc 1
../../bin/agtcut -cm imspr -nogm -key 0 -prv -vis -s tutor4_spr.png -crn 224 -p xpal.pi1 -o stf/o.ims -sxp 0 -syp 94 -sxs 25 -sys 23 -sc 1


# cut background map & tiles from [tutor4_map.png]
#
# note: produces [tutor3.cct], [tutor3.ccm]

../../bin/agtcut -cm tiles -vis -t 0.0 -ccf 1 -bp 4 -om direct -s tutor4_map.png -crn 224 -p xpal.pi1 -o stf/tutor4.cct
