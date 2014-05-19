#include "cam.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>

#include "util.h"

#define NUM_BUFFERS 4

// Try to perform an ioctl until we are not interupted
static int xioctl(int fh, int request, void * arg) {
  int r;
  do {
    r = ioctl(fh, request, arg);
  } while (r < 0 && EINTR == errno);
  return r;
}

struct camera_buffer * alloc_buffers(unsigned int num_buffers, unsigned int buffer_size) {
  //user read mode only ever needs 1 buffer
  struct camera_buffer * tr = calloc(num_buffers, sizeof(struct camera_buffer));

  CHECK_ALLOC(tr);

  for (unsigned int i = 0; i < num_buffers; ++i) {
    tr[i].length = buffer_size;
    tr[i].start = malloc(buffer_size);

    CHECK_ALLOC(tr[i].start);
  }

  return tr;
}

void init_userpointer(int fd) {
  struct v4l2_requestbuffers reqbuff;

  memset(&reqbuff, 0, sizeof(reqbuff));
  reqbuff.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  reqbuff.memory = V4L2_MEMORY_USERPTR;
  reqbuff.count = NUM_BUFFERS;

  if (xioctl(fd, VIDIOC_REQBUFS, &reqbuff) < 0) {
    perror("VIDIOC_REQBUFFS failed");
    exit(EXIT_FAILURE);
  }
}

// returns a camera file descriptor
struct camera open_camera(const char * fname) {
  struct v4l2_capability cap;
  struct v4l2_crop crop;
  struct v4l2_format fmt;

  int camera_fd = open(fname, O_RDONLY);

  if (camera_fd < 0) {
    perror("Failed to open video device");
    exit(EXIT_FAILURE);
  }

  if (xioctl(camera_fd, VIDIOC_QUERYCAP, &cap) < 0) {
    if (EINVAL == errno) {
      fprintf(stderr, "%s is not a V4L2 device\n", fname);
      exit(EXIT_FAILURE);
    } else {
      fprintf(stderr, "\nioctl VIDIOC_QUERYCAP failed\n");
      exit(EXIT_FAILURE);
    }
  }

  if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
    fprintf(stderr, "%s can't capture video =(\n", fname);
    exit(EXIT_FAILURE);
  }

  if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
    fprintf(stderr, "%s doesn't support read or write D=\n", fname);
    exit(EXIT_FAILURE);
  }

  memset(&fmt, 0, sizeof(fmt));

  fmt.type          = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width = 320;
  fmt.fmt.pix.height= 240;

  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
  fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
  fmt.fmt.pix.colorspace  = V4L2_COLORSPACE_SRGB;
  if (xioctl(camera_fd, VIDIOC_S_FMT, &fmt) < 0) {
    fprintf(stderr, "ioctl to set pixel format failed\n");
      exit(EXIT_FAILURE);
  }

  printf("Camera is running at %dx%d with %d things per pixel\n", fmt.fmt.pix.width, fmt.fmt.pix.height, fmt.fmt.pix.bytesperline / fmt.fmt.pix.width);

  init_userpointer(camera_fd);
  struct camera_buffer * buffers = alloc_buffers(NUM_BUFFERS, fmt.fmt.pix.sizeimage);

  struct camera tr = {.fd = camera_fd, .buffers = buffers, .num_buffers = NUM_BUFFERS};

  fflush(0);

  return tr;
}

void start_capturing(struct camera * cam) {
  for (unsigned int i = 0; i < cam->num_buffers; ++i) {
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_USERPTR;
    buf.index = i;
    buf.m.userptr = (unsigned long)cam->buffers[i].start;
    buf.length = cam->buffers[i].length;

    if (xioctl(cam->fd, VIDIOC_QBUF, &buf)) {
      perror("buffer enqueue failed");
      exit(EXIT_FAILURE);
    }
  }
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (xioctl(cam->fd, VIDIOC_STREAMON, &type) < 0) {
    perror("VIDIOC_STREAMON");
    exit(EXIT_FAILURE);
  }
}

int read_frame(struct camera * cam) {
  struct v4l2_buffer buff;
  memset(&buff, 0, sizeof(buff));
  buff.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buff.memory = V4L2_MEMORY_USERPTR;

  if (xioctl(cam->fd, VIDIOC_DQBUF, &buff) < 0) {
    switch(errno) {
      case EAGAIN:
      case EIO:
        return -1;
      default:
        perror("Buffer DQ failed");
        exit(EXIT_FAILURE);
    }
  }

  int i = buff.index;

  assert(i < cam->num_buffers);

  if (xioctl(cam->fd, VIDIOC_QBUF, &buff) < 0) {
    perror("Failed to reenqueue buffer");
    exit(EXIT_FAILURE);
  }

  return i;
}
