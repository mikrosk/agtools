# create a superpalette from all art assets (only needs done once, or after significant art changes)
#
# generate STE palette
# use cc mode #3 (batched superpalette)
# use 6 solver rounds on 6 threads (takes several minutes - can get a fast preview with 1,1)
# reduce colour satuation to 0.8
# cap input colours to 4096 to save time (only about 300 used anyway)
# ignore any magenta present (FF:00:FF) in spritesheet sources
# set colour population control to log(population), so over-used colours don't steal disproportionat significance
# set accuracy-population-curve and differential-population-curve low, so dominant colours don't get exaggerated importance or exaggerated dither suppression
# set differential-chroma-significance high, to avoid dithering with strongly opposing colours
# specify all source images

#../../bin/pcs -cd ste -ccmode 3 -ccrounds 6 -ccthreads 6 -ccsat 0.9 -ccl 15=f:f:f -ccignore ff:00:ff -ccpopctrl 0.5 -ccapc 0.5 -ccdpc 0.3 -ccdcs 0.06 -v bigmap.png player.png sprites.png


# cut background map & tiles
#
# [tiles] mode
# specify 2-field rendering (complementary dithered fields)
# specify 4 bitplane tiles
# specify direct output (.cct/.ccm) for playfield engine
# specify source image (level map .png)
# specify output file
# specify palette source (chunky8.ccs from PCS)

../../bin/agtcut -cm tiles -t 0.75 -ccf 2 -bp 4 -om direct -s bigmap.png -o demo1.cct -p chunky8.ccs


# cut vicviper sprite animation
#
# [sprite] mode
# enable colour index/mask preview
# select unused key colour index (99) and key transparency off magenta (ff:00:ff)
# enable key colour guard, set tolerance low
# generate mask for STE blitter (plane-interleaved)
# specify source, output files
# specify palette source (chunky8.ccs from PCS)
# specify cutting coordinates & frame count

../../bin/agtcut -cm emxspr -prv -key 99 -keyrgb ff:00:ff -kg -kt 0.01 -gm -s player.png -o vicviper.emx -p chunky8.ccs -sxp 46 -syp 2 -sxs 23 -sys 16 -syi 18 -sc 9


# cut options sprite animation
#
# [sprite] mode
# enable colour index/mask preview
# select unused key colour index (99) and key transparency off magenta (ff:00:ff)
# enable key colour guard, set tolerance low
# generate mask for STE blitter (plane-interleaved)
# specify source, output files
# specify palette source (chunky8.ccs from PCS)
# specify cutting coordinates & frame count

# option (multiple)
../../bin/agtcut -cm emxspr -prv -ccr low -key 99 -keyrgb ff:00:ff -kg -kt 0.01 -gm -s sprites.png -o options.emx -p chunky8.ccs -sxp 309 -syp 23 -sxs 16 -sys 12 -sxi 17 -sc 3

# lasers
../../bin/agtcut -cm emxspr -nc -prv -ccr low -key 99 -keyrgb ff:00:ff -kg -kt 0.01 -gm -s sprites.png -o lasers.emx -p chunky8.ccs -sxp 216 -syp 19 -sxs 32 -sys 1 -sxi 0 -sc 1
../../bin/agtcut -cm emxspr -nc -prv -ccr low -key 99 -keyrgb ff:00:ff -kg -kt 0.01 -gm -s sprites.png -o dlasers.emx -p chunky8.ccs -sxp 216 -syp 23 -sxs 32 -sys 3 -sxi 0 -sc 1
../../bin/agtcut -cm emxspr -nc -prv -ccr high -key 99 -keyrgb ff:00:ff -kg -kt 0.01 -gm -s sprites.png -o plasers.emx -p chunky8.ccs -sxp 216 -syp 27 -sxs 14 -sys 5 -sxi 0 -sc 1

# flame (really, volcano lavabomb)
../../bin/agtcut -cm emxspr -prv -key 99 -keyrgb ff:00:ff -kg -kt 0.01 -gm -s sprites.png -o flames.emx -p chunky8.ccs -sxp 220 -syp 124 -sxs 16 -sys 16 -sxi 16 -sc 3

# powerup drone
../../bin/agtcut -cm emxspr -prv -key 99 -keyrgb ff:00:ff -kg -kt 0.01 -gm -s sprites.png -o powerup.emx -p chunky8.ccs -sxp 156 -syp 44 -sxs 16 -sys 14 -sxi 19 -sc 8

# bullet #1
../../bin/agtcut -cm emxspr -nc -prv -key 99 -keyrgb ff:00:ff -kg -kt 0.01 -gm -s sprites.png -o bullet.emx -p chunky8.ccs -sxp 172 -syp 28 -sxs 9 -sys 2 -sxi 12 -sc 2

# bullet #2
../../bin/agtcut -cm emxspr -nc -prv -key 99 -keyrgb ff:00:ff -kg -kt 0.01 -gm -s sprites.png -o bulletd.emx -p chunky8.ccs -sxp 196 -syp 26 -sxs 7 -sys 7 -sxi 10 -sc 2

# missile
../../bin/agtcut -cm emxspr -nc -prv -key 99 -keyrgb ff:00:ff -kg -kt 0.01 -gm -s sprites.png -o missile.emx -p chunky8.ccs -sxp 311 -syp 45 -sxs 12 -sys 12 -sxi 13 -sc 5


# pink splat
../../bin/agtcut -cm emxspr -prv -key 99 -keyrgb ff:00:ff -kg -kt 0.01 -gm -s sprites.png -o sparks.emx -p chunky8.ccs -sxp 228 -syp 451 -sxs 16 -sys 16 -sxi 19 -sc 5


# nme: amoeba
../../bin/agtcut -cm emxspr -prv -key 99 -keyrgb ff:00:ff -kg -kt 0.01 -gm -s sprites.png -o amoeba.emx -p chunky8.ccs -sxp 5 -syp 124 -sxs 17 -sys 16 -sxi 18 -sc 12

# nme: spinny (z-rotating)
../../bin/agtcut -cm emxspr -prv -key 99 -keyrgb ff:00:ff -kg -kt 0.01 -gm -s sprites.png -o spinny.emx -p chunky8.ccs -sxp 7 -syp 83 -sxs 16 -sys 16 -sxi 17 -sc 4 -sm 3:1:xy

# nme: square (y-rotating)
../../bin/agtcut -cm emxspr -prv -key 99 -keyrgb ff:00:ff -kg -kt 0.01 -gm -s sprites.png -o square.emx -p chunky8.ccs -sxp 89 -syp 65 -sxs 12 -sys 12 -sxi 14 -sc 6 -sm 4:2:x -sm 5:1:x

# nme: drone (x-rotating)
../../bin/agtcut -cm emxspr -prv -key 99 -keyrgb ff:00:ff -kg -kt 0.01 -gm -s sprites.png -o drone.emx -p chunky8.ccs -sxp 108 -syp 83 -sxs 18 -sys 16 -sxi 19 -sc 5

# nme: cylon-ish fighter (x-rotating)
../../bin/agtcut -cm emxspr -prv -key 99 -keyrgb ff:00:ff -kg -kt 0.01 -gm -s sprites.png -o cylon.emx -p chunky8.ccs -sxp 205 -syp 83 -sxs 16 -sys 16 -sxi 19 -sc 8 -sm 5:3:y -sm 6:2:y -sm 7:1:y

# nme: bee-ish fighter (x-rotating)
../../bin/agtcut -cm emxspr -prv -key 99 -keyrgb ff:00:ff -kg -kt 0.01 -gm -s sprites.png -o bee.emx -p chunky8.ccs -sxp 300 -syp 83 -sxs 16 -sys 16 -sxi 19 -sc 7 -sm 5:2:y -sm 6:1:y

# nme: fly (seeking)
# the spritesheet provides 1 rotation quadrant of 4 needed, so we map the rest as x/y flips
../../bin/agtcut -cm emxspr -prv -key 99 -keyrgb ff:00:ff -kg -kt 0.01 -gm -s sprites.png -o fly.emx -p chunky8.ccs -sxp 6 -syp 105 -sxs 17 -sys 16 -sxi 19 -sc 16 -sm 5:3:x -sm 6:2:x -sm 7:1:x -sm 8:0:x -sm 9:1:xy -sm 10:2:xy -sm 11:3:xy -sm 12:4:xy -sm 13:3:y -sm 14:2:y -sm 15:1:y

# nme: fly2 (seeking)
# the spritesheet provides 1 rotation quadrant of 4 needed, so we map the rest as x/y flips
../../bin/agtcut -cm emxspr -prv -key 99 -keyrgb ff:00:ff -kg -kt 0.01 -gm -s sprites.png -o fly2.emx -p chunky8.ccs -sxp 193 -syp 104 -sxs 16 -sys 16 -sxi 16 -sc 16 -sm 5:3:x -sm 6:2:x -sm 7:1:x -sm 8:0:x -sm 9:1:xy -sm 10:2:xy -sm 11:3:xy -sm 12:4:xy -sm 13:3:y -sm 14:2:y -sm 15:1:y

# nme: tie (x-rotating)
../../bin/agtcut -cm emxspr -prv -key 99 -keyrgb ff:00:ff -kg -kt 0.01 -gm -s sprites.png -o tie.emx -p chunky8.ccs -sxp 101 -syp 104 -sxs 16 -sys 16 -sxi 19 -sc 8 -sm 5:3:y -sm 6:2:y -sm 7:1:y

# nme: quad
../../bin/agtcut -cm emxspr -prv -key 99 -keyrgb ff:00:ff -kg -kt 0.01 -gm -s sprites.png -o quad.emx -p chunky8.ccs -sxp 279 -syp 104 -sxs 16 -sys 16 -sxi 17 -sc 5

# nme: pellets
../../bin/agtcut -cm emxspr -prv -key 99 -keyrgb ff:00:ff -kg -kt 0.01 -gm -s sprites.png -o pellets.emx -p chunky8.ccs -sxp 345 -syp 275 -sxs 4 -sys 5 -sxi 57 -sc 2

# nme: diamonds
../../bin/agtcut -cm emxspr -prv -key 99 -keyrgb ff:00:ff -kg -kt 0.01 -gm -s sprites.png -o diamonds.emx -p chunky8.ccs -sxp 312 -syp 202 -sxs 6 -sys 6 -sxi 9 -sc 2

# nme: blobs
../../bin/agtcut -cm emxspr -prv -key 99 -keyrgb ff:00:ff -kg -kt 0.01 -gm -s sprites.png -o blobs.emx -p chunky8.ccs -sxp 323 -syp 273 -sxs 8 -sys 8 -sxi 11 -sc 2

# nme: jelly
../../bin/agtcut -cm emxspr -prv -key 99 -keyrgb ff:00:ff -kg -kt 0.01 -gm -s sprites.png -o jelly.emx -p chunky8.ccs -sxp 283 -syp 254 -sxs 32 -sys 30 -sc 1

# nme: beam
../../bin/agtcut -cm emxspr -nc -prv -key 99 -keyrgb ff:00:ff -kg -kt 0.01 -gm -s sprites.png -o beam.emx -p chunky8.ccs -sxp 361 -syp 162 -sxs 30 -sys 4 -sc 1

# nme: walker
../../bin/agtcut -cm emxspr -ccr high -prv -key 99 -keyrgb ff:00:ff -kg -kt 0.01 -gm -s sprites.png -o walker.emx -p chunky8.ccs -sxp 8 -syp 196 -sxs 21 -sys 15 -sxi 23 -sc 4
