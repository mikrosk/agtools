mkdir stf

# cut 3 sprites into 3 different formats

../../bin/agtcut -cm imspr -nogm -key 0 -prv -vis -s tutor3_spr.png -crn 224 -p xpal.pi1 -o stf/armor.ims -sxp 96 -syp 0 -sxs 16 -sys 16 -sc 1
../../bin/agtcut -cm imspr -nogm -key 0 -prv -vis -s tutor3_spr.png -crn 224 -p xpal.pi1 -o stf/spinner.ims -sxp 80 -syp 0 -sxs 16 -sys 16 -syi 16 -sc 8
../../bin/agtcut -cm imspr -nogm -key 0 -prv -vis -s tutor3_spr.png -crn 224 -p xpal.pi1 -o stf/ball.ims -sxp 64 -syp 0 -sxs 16 -sys 15 -syi 15 -sc 8
../../bin/agtcut -cm imspr -nogm -key 0 -prv -vis -s tutor3_spr.png -crn 224 -p xpal.pi1 -o stf/n.ims -sxp 0 -syp 94 -sxs 25 -sys 23 -sc 1


# cut background map & tiles from [tutor3_map.png]
#
# note: produces [tutor3.cct], [tutor3.ccm]

../../bin/agtcut -cm tiles -vis -t 0.0 -ccf 1 -bp 4 -om direct -s tutor3_map.png -crn 224 -p xpal.pi1 -o stf/tutor3.cct

