#include "myosc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
