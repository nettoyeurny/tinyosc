#include "pattern.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

typedef struct {
  int32_t size;
  void *data;
} osc_packet;

void osc_align(char **p, int *n) {
  int d = *n & 0x03;
  *p += d;
  *n -= d;
}

static int osc_append_int(char **p, int i, int *n) {
  if (*n < 4) return -1;
  *(int *)*p = htonl(i);
  *p += 4;
  *n -= 4;
  return 0;
}

static int osc_append_string(char **p, const char *s, int *n) {
  int len = strnlen(s, *n) + 1;
  if (len > *n) return -1;
  strcpy(*p, s);
  *p += len;
  *n -= len;
  osc_align(p, n);
  return 0;
}

static int osc_append_blob(char **p, int s, const void *b, int *n) {
  if (*n < s + 4) return -1;
  *(int *)*p = htonl(s);
  memcpy(*p + 4, b, s);
  *p += s + 4;
  *n -= s + 4;
  osc_align(p, n);
  return 0;
}

int osc_pack_message(osc_packet *packet, int capacity,
    const char *address, const char *types, ...) {
  if (capacity & 0x03) return -2;
  int nleft = capacity;
  char *p = (char *) packet->data;
  if (osc_append_string(&p, address, &nleft)) return -1;
  if (nleft <= 0) return -1;
  *p++ = ',';
  --nleft;
  if (osc_append_string(&p, types, &nleft)) return -1;
  int iv;
  float fv;
  const char *sv;
  const void *vv;
  const char *t;
  va_list ap;
  va_start(ap, types);
  for (t = types; *t; ++t) {
    switch (*t) {
      case 'i':  // int32
        iv = va_arg(ap, int32_t);
        if (osc_append_int(&p, iv, &nleft)) return -1;
        break;
      case 'f':  // float32
        fv = (float) va_arg(ap, double);
        if (osc_append_int(&p, *(int *)&fv, &nleft)) return -1;
        break;
      case 's':  // OSC-string
        sv = va_arg(ap, const char *);
        if (osc_append_string(&p, sv, &nleft)) return -1;
        break;
      case 'b':  // OSC-blob
        iv = va_arg(ap, int32_t);
        vv = va_arg(ap, const void *);
        if (osc_append_blob(&p, iv, vv, &nleft)) return -1;
        break;
    }
  }
  va_end(ap);
  packet->size = capacity - nleft;
  return 0;
}

int osc_unpack_message(const osc_packet *packet,
    const char *address, const char *types, ...) {
  int nleft = packet->size;
  char *p = packet->data;
  if (!pattern_matches(p, address)) return -1;
  int n = strlen(p) + 1;
  p += n;
  nleft -= n;
  osc_align(&p, &nleft);
  if (strcmp(p + 1, types)) return -2;
  n = strlen(types) + 2;
  p += n;
  nleft -= n;
  osc_align(&p, &nleft);
  const char *t;
  int32_t *ip;
  float *fp;
  char *sp;
  void *vp;
  va_list ap;
  va_start(ap, types);
  for (t = types; *t; ++t) {
    switch (*t) {
      case 'i':  // int32
      case 'f':  // float32
        ip = va_arg(ap, int32_t *);
        *ip = ntohl(*(int32_t *)p);
        p += 4;
        nleft -= 4;
        break;
      case 's':  // OSC-string
        sp = va_arg(ap, char *);
        strcpy(sp, p);
        n = strlen(sp) + 1;
        p += n;
        nleft -= n;
        osc_align(&p, &nleft);
        break;
      case 'b':  // OSC-blob
        ip = va_arg(ap, int32_t *);
        *ip = ntohl(*(int32_t *)p);
        vp = va_arg(ap, void *);
        memcpy(vp, p + 4, *ip);
        p += 4 + *ip;
        nleft -= 4 + *ip;
        osc_align(&p, &nleft);
        break;
    }
  }
  va_end(ap);
  if (nleft != 0) return -3;
  return 0;
}

int main(int argc, char **argv) {
  int N = 256;
  osc_packet packet;
  packet.data = malloc(N);
  char v[3] = { 65, 66, 0 };
  int r1 = osc_pack_message(&packet, N, "/*/bar", "ibsfi",
      4, 3, v, "abcde", 2.5, -3);
  int i = 0, j = 0, nb = 0;
  float f;
  char s[16];
  char b[16];
  int r2 = osc_unpack_message(&packet, "/foo/bar", "ibsfi",
      &i, &nb, b, s, &f, &j);
  printf("%d %d %d %d %s %s %f %d\n", r1, r2, i, nb, b, s, f, j);
  free(packet.data);
  return 0;
}
