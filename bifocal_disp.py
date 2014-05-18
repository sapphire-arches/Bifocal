from pyoogl.windowmanagement import Window
from OpenGL.GL import *
from sys import argv
from PIL import Image
from time import time

def disp():
    glClear(GL_COLOR_BUFFER_BIT, GL_DEPTH_BUFFER_BIT)

MAX_DIFF = 5
WINDOW_RADIUS = 1

def compute_region_diff(imgr, imgl, x, y, d, current_min):
    diff = 0
    for xx in range(-WINDOW_RADIUS, WINDOW_RADIUS):
        for yy in range(-WINDOW_RADIUS, WINDOW_RADIUS):
            p1 = (x + xx, y + yy)
            p2 = (x + xx + d, y + yy)
            dd = imgr.getpixel(p1) - imgl.getpixel(p2)
            diff += dd * dd
            if diff > current_min:
                return diff
    return diff

def compute_sobel(x, y, img):
    sobel_x = [[-1, 0, 1], [-2, 0, 2], [-1, 0, 1]]
    sobel_y = [[1, 2, 1], [0, 0, 0], [-1, -2, -1]]
    val_x = 0
    val_y = 0
    for xx in range(3):
        for yy in range(3):
            val = img.getpixel((x + xx - 2, y + yy - 2))
            val_x += sobel_x[xx][yy] * val
            val_y += sobel_y[xx][yy] * val
    return val_x * val_x + val_y * val_y

def compute_dif(imgr, imgl):
    # is going to be a greyscale image
    if not imgr.size == imgl.size:
        raise ValueError("Image sizes do not match [%s, %s]" % (str(imgr.size), str(imgl.size)))
    size = imgr.size
    diff = Image.new("L", size)
    depth = Image.new("L", size)
    sobel = Image.new("L", size)

    print(imgr.size, imgl.size)

    pixel_count = 0

    tick = time()
    last_print = tick

    sobel_counter = 0
    sobel_passes = 0

    for x in range(MAX_DIFF + WINDOW_RADIUS, size[0] - WINDOW_RADIUS):
        for y in range(MAX_DIFF + WINDOW_RADIUS, size[1] - WINDOW_RADIUS):
            p = (x, y)
            sobel_value = compute_sobel(x, y, imgr)
            img_diff = (imgr.getpixel(p) - imgl.getpixel(p) + 255) // 2
            diff.putpixel(p, img_diff)
            min_diff = 1 << 31 # really friggin big number
            min_diff_pos = 0
            # random constant is the average sobel value for my test image + a few hundred for good measure
            if sobel_value > 37800:
                sobel.putpixel(p, 255)
                sobel_passes += 1
                for d in range(-MAX_DIFF, 1):
                    region_diff = compute_region_diff(imgr, imgl, x, y, d, min_diff)
                    if region_diff < min_diff:
                        min_diff_pos = d
                        min_diff = region_diff
                min_diff_pos = -min_diff_pos
                depth.putpixel(p, min_diff_pos * 20)
            else:
                depth.putpixel(p, 255)
                sobel.putpixel(p, 0)
            pixel_count += 1
            sobel_counter += sobel_value
        if time() - last_print > 1:
            print(x)
            last_print = time()

    tock = time()

    diff.save("testdiff.png", "png")
    depth.save("testdepth.png", "png")
    sobel.save("testsobel.png", "png")

    print("Throughput was %3.2d pixels per second, average sobel value of %d with %2.2d%% of pixels passing" % (pixel_count / (tock - tick), sobel_counter / pixel_count, 100 * sobel_passes / pixel_count))
    return depth


def init():
    imgr = Image.open(argv[1]).convert('L')
    imgl = Image.open(argv[2]).convert('L')
    compute_dif(imgr, imgl)

# win = Window("bifocal_display", display_func=disp)
# win.initialize(argv)
init()
# win.main_loop()
