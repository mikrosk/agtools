# palette generate step deprecated - now uses prepared 16c palette from Xenon (xenpal.pi1)
#../../bin/pcs -cd ste -ccmode 3 -ccrounds 6 -ccthreads 4 -ccincap 8192 -ccignore ff:00:ff -ccpopctrl 0.5 -ccapc 0.5 -ccdpc 0.5 -v rtlevel7_8b.png xenboss.png


# boss
#../../bin/agtcut -cm slabs -prv -keyrgb ff:00:ff -kg -kt 0.01 -gm -s xenboss.png -o bosscore.sls -p xenpal.pi1 -sxp 0 -syp 0 -sxs 144 -sys 96 -sc 1
#../../bin/agtcut -cm slabrestore -prv -keyrgb ff:00:ff -kg -kt 0.01 -gm -s xenboss.png -o bosscore.slr -p xenpal.pi1 -sxp 0 -syp 0 -sxs 144 -sys 96 -sc 1

# boss parts
../../bin/agtcut -cm emxspr -prv -keyrgb ff:00:ff -kg -kt 0.01 -gm -s bosseye.png -o bosseye.emx -p xenpal.pi1 -sxp 0 -syp 0 -sxs 22 -sys 16 -sxi 22 -syi 0 -sc 6
../../bin/agtcut -cm emxspr -nc -prv -keyrgb ff:00:ff -kg -kt 0.01 -gm -s bosspulp.png -o bosspulp.emx -p xenpal.pi1 -sxp 0 -syp 0 -sxs 30 -sys 8 -sxi 0 -syi 8 -sc 6

# boss weapons
../../bin/agtcut -cm emxspr -nc -prv -keyrgb ff:00:ff -kg -kt 0.01 -gm -s balls.png -o balls.emx -p xenpal.pi1 -sxp 0 -syp 0 -sxs 10 -sys 10 -sxi 0 -syi 10 -sc 9
../../bin/agtcut -cm emxspr -nc -prv -keyrgb ff:00:ff -kg -kt 0.01 -gm -s sprites.png -o beam.emx -p xenpal.pi1 -sxp 361 -syp 162 -sxs 30 -sys 4 -sc 1

# player
../../bin/agtcut -cm emxspr -prv -keyrgb ff:00:ff -kg -kt 0.01 -gm -s player.png -o vicviper.emx -p xenpal.pi1 -sxp 46 -syp 2 -sxs 23 -sys 16 -syi 18 -sc 9
../../bin/agtcut -cm emxspr -nc -prv -keyrgb ff:00:ff -kg -kt 0.01 -gm -s sprites.png -o bullet.emx -p xenpal.pi1 -sxp 172 -syp 28 -sxs 9 -sys 2 -sxi 12 -sc 2

# map & tiles
../../bin/agtcut -cm tiles -t 0.33 -ccf 1 -om direct -s map3.png -o bosscore.cct -p xenpal.pi1

