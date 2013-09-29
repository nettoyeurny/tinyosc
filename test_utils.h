#ifndef __TEST_UTILS_H__
#define __TEST_UTILS_H__

#include <stdio.h>

#define EXPECT(x) \
  do { \
    if (!(x)) { \
      fprintf(stderr, "\033[31;1m%s:%i failed.\033[0m %s\n", \
          __FUNCTION__, __LINE__, #x); \
      return -1; \
    } \
  } while (0)

#define TEST(x) \
  do { \
    if (!x()) fprintf(stderr, "\033[32;1m%s passed.\033[0m\n", #x); \
  } while (0)

#endif
