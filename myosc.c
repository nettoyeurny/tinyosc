// TODO: Error checking and tests.

#include "myosc.h"
#include "pattern.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

static uint64_t htonll(uint64_t m) {
  int32_t *p = (int32_t *) &m;
  uint64_t n;
  int32_t *q = (int32_t *) &n;
  *q = (int32_t) htonl((int32_t) (m >> 32));
  *(q + 1) = htonl((int32_t) m);
  return n;
}

#define ntohll(x) htonll(x)

static void osc_align(char **p, int *n) {
  int d = *n & 0x03;
  *p += d;
  *n -= d;
}

static int osc_append_int(char **p, int32_t i, int *n) {
  if (*n < 4) return -1;
  *(int32_t *)*p = htonl(i);
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
  char *p = packet->data;
  if (osc_append_string(&p, address, &nleft)) return -1;
  if (nleft <= 0) return -1;
  *p++ = ',';
  --nleft;
  if (osc_append_string(&p, types, &nleft)) return -1;
  int32_t iv;
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
        // Float-as-int aliasing FTW!
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

int osc_is_bundle(osc_packet *packet) {
  return *packet->data == '#';
}

int osc_make_bundle(osc_packet *bundle, int capacity, uint64_t time) {
  if (capacity < 16) return -1;
  char *p = bundle->data;
  strcpy(p, "#bundle");
  *(uint64_t *) (p + 8) = htonll(time);
  bundle->size = 16;
  return 0;
}

int osc_add_packet_to_bundle(
    osc_packet *bundle, int capacity, osc_packet *packet) {
  if (!osc_is_bundle(bundle)) return -1;
  int bs = bundle->size;
  int ps = packet->size;
  if (capacity - bs < ps + 4) return -1;
  char *p = bundle->data;
  p += bs;
  *(int *)p = htonl(packet->size);
  p += 4;
  memcpy(p, packet->data, ps);
  bundle->size = bs + ps + 4;
  return 0;
}

int osc_time_from_bundle(osc_packet *bundle, uint64_t *time) {
  if (!osc_is_bundle(bundle)) return -1;
  *time = ntohll(*(uint64_t *) (bundle->data + 8));
  return 0;
}

int osc_next_packet_from_bundle(
    osc_packet *bundle, osc_packet *current) {
  if (!osc_is_bundle(bundle)) return -1;
  int bs = bundle->size;
  char *p = bundle->data;
  if (bs <= 16) return -2;
  if (!current->data) {
    current->size = ntohl(*(int32_t *) (p + 16));
    current->data = p + 20;
    return 0;
  }
  int ps = current->size;
  char *q = current->data;
  q += ps;
  if (q - p >= bs) return -1;
  current->size = ntohl(*(int32_t *) q);
  current->data = q + 4;
  return 0;
}

