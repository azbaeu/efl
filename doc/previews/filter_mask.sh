TEXT="Mask"
FONT="Sans:style=bold"
SIZE=64
FILTER="
buffer: a (alpha);
blur(5, dst = a);
curve(points = 0:255 - 128:255 - 255:0, src = a, dst = a);
blend(color = black);
mask(mask = a, color = cyan);
"
