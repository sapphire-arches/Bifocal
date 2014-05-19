#include "cam.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <GL/glut.h>
#include <GL/gl.h>

#define PRINT_GL_ERROR(tmpvar) if ((tmpvar = glGetError()) != 0) {fprintf(stderr, "GL error is %d at %s:%d\n", tmpvar, __FILE__, __LINE__);}

static struct camera cam;
static GLuint texture;
static unsigned char * swap_tmp;

void diff_time(struct timeval * x, struct timeval * y, struct timeval * diff);

void reshape(int w, int h) {
  glLoadIdentity();
  glTranslatef(-1.f, -1.f, 0.f);
  glScalef(2.f / w, 2.f / h, 0.f);
  glViewport(0, 0, w, h);
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
      printf("%d %d\n", retval, cam.buffers[retval].length);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 320, 240, 0, GL_RED, GL_UNSIGNED_SHORT, cam.buffers[retval].start);
      PRINT_GL_ERROR(retval);
    }
  }

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glBindTexture(GL_TEXTURE_2D, texture);
  glBegin(GL_QUADS);
  {
    glVertex2f(0, 0);
    glTexCoord2f(1, 1);

    glVertex2f(320, 0);
    glTexCoord2f(1, 0);

    glVertex2f(320, 240);
    glTexCoord2f(0, 0);

    glVertex2f(0, 240);
    glTexCoord2f(0, 1);
  }
  glEnd();

  PRINT_GL_ERROR(retval);

  glutSwapBuffers();

  gettimeofday(&end, NULL);

  diff_time(&end, &start, &diff);

  if (diff.tv_sec * 1000000 + diff.tv_usec < 16666) {
    struct timespec to_sleep = {.tv_sec = 0, .tv_nsec = (1000 * (16666 - diff.tv_usec))};
    nanosleep(&to_sleep, NULL);
  }
}

void init_window(int * argc, char ** argv) {
  int temp;
  glutInit(argc, argv);
  glutInitWindowSize(800, 600);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutCreateWindow("camtest");

  glutReshapeFunc(reshape);
  glutDisplayFunc(display);
  glutIdleFunc(display);

  glEnable(GL_TEXTURE_2D);
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  PRINT_GL_ERROR(temp);

  glClearColor(0.5f, 0.5f, 0.5f, 0.f);
}

int main(int argc, char ** argv) {
  cam = open_camera(argv[1]);
  start_capturing(&cam);
  swap_tmp = malloc(cam.buffers[0].length);

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
