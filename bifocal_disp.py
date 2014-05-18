from pyoogl.windowmanagement import Window
from OpenGL.GL import *
from sys import argv
from PIL import Image
from time import time

def disp():
    glClear(GL_COLOR_BUFFER_BIT, GL_DEPTH_BUFFER_BIT)

MAX_DIFF = 5
WINDOW_RADIUS = 1

def compute_region_diff(imgr, imgl, x, y, d):
    diff = 0
    for xx in range(-WINDOW_RADIUS, WINDOW_RADIUS):
        for yy in range(-WINDOW_RADIUS, WINDOW_RADIUS):
            p1 = (x + xx, y + yy)
            p2 = (x + xx + d, y + yy)
            dd = imgr.getpixel(p1) - imgl.getpixel(p2)
            diff += dd * dd
    return diff

def compute_dif(imgr, imgl):
    # is going to be a greyscale image
    if not imgr.size == imgl.size:
        raise ValueError("Image sizes do not match [%s, %s]" % (str(imgr.size), str(imgl.size)))
    size = imgr.size
    diff = Image.new("L", size)
    depth = Image.new("L", size)

    print(imgr.size, imgl.size)

    pixel_count = 0
    skiped_count = 0

    tick = time()
    last_print = tick

    for x in range(MAX_DIFF + WINDOW_RADIUS, size[0] - WINDOW_RADIUS):
        for y in range(MAX_DIFF + WINDOW_RADIUS, size[1] - WINDOW_RADIUS):
            p = (x, y)
            img_diff = (imgr.getpixel(p) - imgl.getpixel(p) + 255) // 2
            diff.putpixel(p, img_diff)
            min_diff = 1 << 31 # really friggin big number
            min_diff_pos = 0
            if abs(img_diff) > 100:
                for d in range(-MAX_DIFF, 1):
                    region_diff = compute_region_diff(imgr, imgl, x, y, d)
                    if region_diff < min_diff:
                        min_diff_pos = d
                        min_diff = region_diff
            else:
                #assume no difference
                skiped_count += 1
                min_diff_pos = 0
            min_diff_pos = -min_diff_pos
            depth.putpixel(p, min_diff_pos * 20)
            pixel_count += 1
        if time() - last_print > 1:
            print(x)
            last_print = time()

    tock = time()

    diff.save("testdiff.png", "png")
    depth.save("testdepth.png", "png")

    print("Skipped %%%1.2d of pixels. Throughput was %3.2d pixels per second" % (100 * skiped_count / pixel_count, pixel_count / (tock - tick)))
    return depth


def init():
    imgr = Image.open(argv[1]).convert('L')
    imgl = Image.open(argv[2]).convert('L')
    compute_dif(imgr, imgl)

# win = Window("bifocal_display", display_func=disp)
# win.initialize(argv)
init()
# win.main_loop()
