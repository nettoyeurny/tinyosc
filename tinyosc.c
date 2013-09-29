#include "tinyosc.h"
#include "pattern.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

static uint64_t htonll(uint64_t m) {
  uint64_t n;
  int32_t *p = (int32_t *) &n;
  *p = htonl((int32_t) (m >> 32));
  *(p + 1) = htonl((int32_t) m);
  return n;
}

#define ntohll(x) htonll(x)

static void osc_advance(char **p, int d, int *n) {
  *p += d;
  *n -= d;
  int r = *n & 0x03;
  *p += r;
  *n -= r;
}

static int osc_append_int(char **p, int32_t i, int *n) {
  if (*n < 4) return -1;
  *(int32_t *)*p = htonl(i);
  *p += 4;
  *n -= 4;
  return 0;
}

static int osc_get_int(char **p, int32_t *ip, int *n) {
  if (*n < 4) return -1;
  *ip = ntohl(*(int32_t *)*p);
  *p += 4;
  *n -= 4;
  return 0;
}

static int osc_append_bytes(char **p, int s, const char *b, int *n) {
  if (*n < s) return -1;
  memcpy(*p, b, s);
  osc_advance(p, s, n);
  return 0;
}

static int osc_get_bytes(char **p, int s, char *b, int *n) {
  if (*n < s) return -1;
  memcpy(b, *p, s);
  osc_advance(p, s, n);
  return 0;
}

static int osc_append_char(char **p, char c, int *n) {
  if (*n < 1) return -1;
  **p = c;
  *p += 1;
  *n -= 1;
  return 0;
}

int osc_pack_message(osc_packet *packet, int capacity,
    const char *address, const char *types, ...) {
  if (capacity & 0x03) return -1;
  int nleft = capacity;
  char *p = packet->data;
  if (osc_append_bytes(&p, strlen(address) + 1, address, &nleft)) return -1;
  if (osc_append_char(&p, ',', &nleft)) return -1;
  if (osc_append_bytes(&p, strlen(types) + 1, types, &nleft)) return -1;
  int32_t iv;
  float fv;
  const char *sv;
  const char *t;
  va_list ap;
  va_start(ap, types);
  for (t = types; *t; ++t) {
    switch (*t) {
      case 'i':  // int32
      case 'm':  // 4-byte MIDI message
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
        iv = strlen(sv) + 1;  // Count terminating \0 character.
        if (osc_append_bytes(&p, iv, sv, &nleft)) return -1;
        break;
      case 'b':  // OSC-blob
        iv = va_arg(ap, int32_t);
        sv = va_arg(ap, const char *);
        if (osc_append_int(&p, iv, &nleft)) return -1;
        if (osc_append_bytes(&p, iv, sv, &nleft)) return -1;
        break;
      default:
        return -1;  // Unknown or unsupported data type.
    }
  }
  va_end(ap);
  packet->size = capacity - nleft;
  return 0;
}

int osc_unpack_message(const osc_packet *packet,
    const char *address, const char *types, ...) {
  int nleft = packet->size;
  if (nleft & 0x03) return -1;
  char *p = packet->data;
  int n = strlen(p) + 1;
  if (nleft < n) return -1;
  if (!pattern_matches(p, address)) return -1;
  osc_advance(&p, n, &nleft);
  n = strlen(types) + 2;
  if (nleft < n) return -1;
  if (*p != ',') return -1;
  if (strcmp(p + 1, types)) return -1;
  osc_advance(&p, n, &nleft);
  const char *t;
  int32_t *ip;
  float *fp;
  char *sp;
  va_list ap;
  va_start(ap, types);
  for (t = types; *t; ++t) {
    switch (*t) {
      case 'i':  // int32
      case 'm':  // 4-byte MIDI message.
      case 'f':  // float32
        ip = va_arg(ap, int32_t *);
        if (osc_get_int(&p, ip, &nleft)) return -1;
        break;
      case 's':  // OSC-string
        sp = va_arg(ap, char *);
        n = strlen(p) + 1;
        if (osc_get_bytes(&p, n, sp, &nleft)) return -1;
        break;
      case 'b':  // OSC-blob
        ip = va_arg(ap, int32_t *);
        sp = va_arg(ap, void *);
        if (osc_get_int(&p, ip, &nleft)) return -1;
        if (osc_get_bytes(&p, *ip, sp, &nleft)) return -1;
        break;
      default:
        return -1;  // Unknown or unsupported data type.
    }
  }
  va_end(ap);
  if (nleft != 0) return -1;
  return 0;
}

int osc_is_bundle(osc_packet *packet) {
  return !strcmp(packet->data, "#bundle");
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
  if (bs <= 16) return -1;
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

