from pyoogl.windowmanagement import Window
from OpenGL.GL import *
from sys import argv
from PIL import Image

def disp():
    glClear(GL_COLOR_BUFFER_BIT, GL_DEPTH_BUFFER_BIT)

MAX_DIFF = 5

def compute_region_diff(imgr, imgl, x, y, d):
    diff = 0
    for xx in range(-2, 2):
        for yy in range(-2, 2):
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

    for x in range(2 * MAX_DIFF, size[0] - MAX_DIFF):
        for y in range(2 * MAX_DIFF, size[1] - MAX_DIFF):
            p = (x, y)
            img_diff = (imgr.getpixel(p) - imgl.getpixel(p) + 255) // 2
            diff.putpixel(p, img_diff)
            min_diff = 1 << 31 # really friggin big number
            min_diff_pos = 0
            if abs(img_diff) > 20:
                for d in range(-MAX_DIFF, 1):
                    region_diff = compute_region_diff(imgr, imgl, x, y, d)
                    if region_diff < min_diff:
                        min_diff_pos = d
                        min_diff = region_diff
            else:
                #assume noe difference
                min_diff_pos = 0
            min_diff_pos = -min_diff_pos
            depth.putpixel(p, min_diff_pos * 20)
        print (x)


    diff.save("testdiff.png", "png")
    depth.save("testdepth.png", "png")


def init():
    imgr = Image.open(argv[1]).convert('L')
    imgl = Image.open(argv[2]).convert('L')
    compute_dif(imgr, imgl)

win = Window("bifocal_display", display_func=disp)
win.initialize(argv)
init()
win.main_loop()
