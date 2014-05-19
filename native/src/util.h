#ifndef _UTIL_H_
#define _UTIL_H_

#define CHECK_ALLOC(x) if (!x) {fprintf(stderr, "alloc failed at %s:%d", __FILE__, __LINE__); exit(EXIT_FAILURE);}

#define PRINT_GL_ERROR(tmpvar) if ((tmpvar = glGetError()) != 0) {fprintf(stderr, "GL error is %d at %s:%d\n", tmpvar, __FILE__, __LINE__);}

#endif
