#include "cam.h"
#include "globals.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <GL/glut.h>
#include <GL/gl.h>

static struct camera cams[NUM_CAMS];
static GLuint textures[NUM_CAMS];
static unsigned int * convert_tmp;

void diff_time(struct timeval * x, struct timeval * y, struct timeval * diff);
void convert_image(unsigned short * yuyv, unsigned int * rgb);

void reshape(int w, int h) {
  glLoadIdentity();
  glTranslatef(-1.f, -1.f, 0.f);
  glScalef(4.f / w, 4.f / h, 0.f);
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
  for (int i = 0; i < NUM_CAMS; ++i) {
    FD_SET(cams[i].fd, &rfds);
  }

  tv.tv_usec = 0;
  tv.tv_sec = 0;

  retval = select(cams[NUM_CAMS - 1].fd + 1, &rfds, NULL, NULL, &tv);

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
    for (int i = 0; i < NUM_CAMS; ++i) {
      struct camera cam = cams[i];
      if (FD_ISSET(cam.fd, &rfds)) {
        retval = read_frame(&cam);
        printf("%d %zd\n", retval, cam.buffers[retval].length);
        convert_image(cams[i].buffers[retval].start, convert_tmp);
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, convert_tmp);
        PRINT_GL_ERROR(retval);
      }
    }
  }

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glBegin(GL_QUADS);
  for (int i = 0; i < NUM_CAMS; ++i) {
    glVertex2f(0, HEIGHT * i);
    glTexCoord2f(1, 1);
    glBindTexture(GL_TEXTURE_2D, textures[i]);

    glVertex2f(WIDTH, HEIGHT * i);
    glTexCoord2f(1, 0);

    glVertex2f(WIDTH, HEIGHT * (i + 1));
    glTexCoord2f(0, 0);

    glVertex2f(0, HEIGHT * (i + 1));
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
  glGenTextures(NUM_CAMS, textures);
  for (int i = 0; i < NUM_CAMS; ++i) {
    glBindTexture(GL_TEXTURE_2D, textures[i]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    PRINT_GL_ERROR(temp);
  }

  glClearColor(0.5f, 0.5f, 0.5f, 0.f);
}

int main(int argc, char ** argv) {
  cams[0] = open_camera("/dev/video1");
  cams[1] = open_camera("/dev/video2");
  for (int i = 0; i < NUM_CAMS; ++i) {
    start_capturing(cams + i);
  }
  convert_tmp = malloc(WIDTH * HEIGHT * sizeof(int));

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

unsigned char color_clampi(int f) {
  printf("%d ", f);
  if (f < 0) {
    return 0;
  }
  if (f > 255) {
    return 255;
  }
  return (unsigned char) f;
}

unsigned char color_clampd(double f) {
  if (f < 0) {
    return 0;
  }
  if (f > 255) {
    return 255;
  }
  return (unsigned char)(f);
}

unsigned int rgb_from_yuyv_components(unsigned char y, unsigned char u, unsigned char v) {
  uint8_t lookup[16] = {
    0x0, 0x8, 0x4, 0xC,
    0x2, 0xA, 0x6, 0xE,
    0x1, 0x9, 0x5, 0xD,
    0x3, 0xB, 0x7, 0xF };
  union {
    unsigned int rgb;
    struct {
      unsigned char padding;
      unsigned char b;
      unsigned char g;
      unsigned char r;
    } colors;
  } output;

  output.rgb = 0;

  y <<= 4;
  u = lookup[u];
  u <<= 4;
  v = lookup[v];
  v <<= 4;

  output.colors.r = color_clampd(y);// + (1.4065 * (u - 128.0)));
  output.colors.g = color_clampd(y);// - (0.3455 * (v - 128.0)) - (0.7169 * (v - 128.0)));
  output.colors.b = color_clampd(y);// + (1.7790 * (v - 128.0)));
  return output.rgb;
}

void rgb_from_yuyv(unsigned short yuyv, unsigned int * c1, unsigned int * c2) {
  union {
    unsigned short yuyv;
    struct {
      unsigned char u:4;
      unsigned char y1:4;
      unsigned char v:4;
      unsigned char y2:4;
    } components;
  } input;
  input.yuyv = yuyv;

  *c1 = rgb_from_yuyv_components(input.components.y1, input.components.u, input.components.v);
  *c2 = rgb_from_yuyv_components(input.components.y1, input.components.u, input.components.v);
}

void convert_image(unsigned short * yuyv, unsigned int * rgb) {
  for (unsigned int i = 0; i < WIDTH * HEIGHT; i += 2) {
    unsigned int * rgb_base = rgb + i;
    rgb_from_yuyv(yuyv[i], rgb_base, rgb_base + 1);
  }
}
