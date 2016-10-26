
# cut sprites

# letters
../../bin/agtcut -cm imspr -gm -prv -vis -s tutor5_spr.png -crn 224 -p xpal.pi1 -o x.ims -sxp 0 -syp 0 -sxs 54 -sys 42 -sc 1
../../bin/agtcut -cm emxspr -prv -vis -s tutor5_spr.png -crn 224 -p xpal.pi1 -o e.emx -sxp 0 -syp 71 -sxs 21 -sys 23 -sc 1
../../bin/agtcut -cm emxspr -prv -vis -s tutor5_spr.png -crn 224 -p xpal.pi1 -o n.emx -sxp 0 -syp 48 -sxs 25 -sys 23 -sc 1
../../bin/agtcut -cm emxspr -prv -vis -s tutor5_spr.png -crn 224 -p xpal.pi1 -o o.emx -sxp 0 -syp 94 -sxs 25 -sys 23 -sc 1

# armor powerup
../../bin/agtcut -cm emxspr -prv -vis -s tutor5_spr.png -crn 224 -p xpal.pi1 -o armor.emx -sxp 96 -syp 0 -sxs 16 -sys 16 -sc 1

# spinner
../../bin/agtcut -cm emxspr -prv -vis -s tutor5_spr.png -crn 224 -p xpal.pi1 -o spinner.emx -sxp 80 -syp 0 -sxs 16 -sys 16 -sxi 0 -syi 16 -sc 8

# craft
../../bin/agtcut -cm emxspr -prv -vis -s tutor5_spr.png -crn 224 -p xpal.pi1 -o craft.emx -sxp 208 -syp 0 -sxs 32 -sys 32 -sxi 0 -syi 32 -sc 6

# small explosion
../../bin/agtcut -cm emxspr -prv -vis -s tutor5_spr.png -crn 224 -p xpal.pi1 -o smlexp.emx -sxp 112 -syp 0 -sxs 16 -sys 16 -sxi 0 -syi 16 -sc 14

# big explosion
../../bin/agtcut -cm emxspr -prv -vis -s tutor5_spr.png -crn 224 -p xpal.pi1 -o bigexp.emx -sxp 144 -syp 0 -sxs 32 -sys 32 -sxi 0 -syi 32 -sc 11

# cut background map & tiles

../../bin/agtcut -cm tiles -vis -t 0.0 -ccf 1 -bp 4 -om direct -s tutor5_map.png -crn 224 -p xpal.pi1 -o tutor5.cct

