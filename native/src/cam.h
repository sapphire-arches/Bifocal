#ifndef _CAM_H_
#define _CAM_H_

#include <stdio.h>

struct camera_buffer {
  size_t length;
  void * start;
};

struct camera {
  struct camera_buffer * buffers;
  int fd;
  unsigned int num_buffers;
};

struct camera open_camera(const char * fname);

void start_capturing(struct camera * cam);

int read_frame(struct camera * cam);

//TODO: cleanup/shutdown functions (stop_capturing, close_camera)

#endif
