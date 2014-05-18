#include "cam.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <GL/glut.h>
#include <GL/gl.h>

static struct camera cam;

void diff_time(struct timeval * x, struct timeval * y, struct timeval * diff);

void reshape(int w, int h) {
  glLoadIdentity();
  glTranslatef(-1.f, -1.f, 0.f);
  glScalef(2.f / w, 2.f / h, 0.f);
}

void display(void) {
  struct timeval start, end, diff;
  fd_set rfds;
  struct timeval tv;
  int retval;

  gettimeofday(&start, NULL);

  FD_ZERO(&rfds);
  FD_SET(0, &rfds);
  FD_SET(cam.fd, &rfds);

  tv.tv_usec = 0;
  tv.tv_sec = 0;

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

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glutSwapBuffers();

  gettimeofday(&end, NULL);

  diff_time(&end, &start, &diff);

  if (diff.tv_sec * 1000000 + diff.tv_usec < 16666) {
    struct timespec to_sleep = {.tv_sec = 0, .tv_nsec = (1000 * (16666 - diff.tv_usec))};
    nanosleep(&to_sleep, NULL);
  }
}

void init_window(int * argc, char ** argv) {
  glutInit(argc, argv);
  glutInitWindowSize(800, 600);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutCreateWindow("camtest");

  glutReshapeFunc(reshape);
  glutDisplayFunc(display);
  glutIdleFunc(display);

  glClearColor(1.f, 1.f, 1.f, 0.f);
}

int main(int argc, char ** argv) {
  cam = open_camera(argv[1]);
  start_capturing(&cam);

  init_window(&argc, argv);

  glutMainLoop();


  return 0;
}

void diff_time(struct timeval * x, struct timeval * y, struct timeval * diff) {
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }
  /* Compute the time remaining to wait.
   *           tv_usec is certainly positive. */
  diff->tv_sec = x->tv_sec - y->tv_sec;
  diff->tv_usec = x->tv_usec - y->tv_usec;
}
