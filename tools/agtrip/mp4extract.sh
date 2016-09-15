# extract every 32nd frame from a .mp4 video, for pushing through the agtrip tool

ffmpeg -i g3_8o8.mp4 -vf "select=not(mod(n\,32))" -vsync vfr img_%03d.png
