#include "cam.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <GL/glut.h>
#include <GL/gl.h>

int main(int argc, const char ** argv) {
  fd_set rfds;
  struct timeval tv;
  int retval;

  FD_ZERO(&rfds);

  struct camera cam = open_camera(argv[1]);
  start_capturing(&cam);

  while(1) {
    FD_SET(0, &rfds);
    FD_SET(cam.fd, &rfds);

    tv.tv_usec = 0;
    tv.tv_sec = 2;

    retval = select(cam.fd + 1, &rfds, NULL, NULL, &tv);

    if (retval < 0) {
      printf("Warning: select errored");
    } else {
      if (FD_ISSET(0, &rfds)) {
        char * buff = malloc(16);
        if ((retval = read(0, buff, 15)) > 0) {
          buff[retval] = 0;
          printf("hi input %s\n", buff);
        } else {
          printf("read of fd 0 failed, exiting assuming it was ^D\n");
          exit(EXIT_SUCCESS);
        }
        free(buff);
      }
      if (FD_ISSET(cam.fd, &rfds)) {
        retval = read_frame(&cam);
        char * vals = (char*)cam.buffers[retval].start;
        printf("%d %2x %d\n", retval, vals[cam.buffers[retval].length] & 0xFF, cam.buffers[retval].length);
      }
    }
  }

  return 0;
}
