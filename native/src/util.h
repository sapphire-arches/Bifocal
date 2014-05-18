#ifndef _UTIL_H_
#define _UTIL_H_

#define CHECK_ALLOC(x) if (!x) {fprintf(stderr, "alloc failed at %s:%d", __FILE__, __LINE__); exit(EXIT_FAILURE);}

#endif
