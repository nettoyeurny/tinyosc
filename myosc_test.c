#include "myosc.h"

#include "test_utils.h"

#define CAPACITY 256

static int test_capacity() {
  char data[CAPACITY];
  osc_packet packet;
  packet.data = data;
  EXPECT(osc_pack_message(&packet, 0, "/foo", "") != 0);
  EXPECT(osc_pack_message(&packet, 8, "/foo", "") != 0);
  EXPECT(osc_pack_message(&packet, 11, "/foo", "") != 0);
  EXPECT(osc_pack_message(&packet, 12, "/foo", "") == 0);
  EXPECT(osc_pack_message(&packet, 15, "/foo", "i", 0) != 0);
  EXPECT(osc_pack_message(&packet, 16, "/foo", "i", 0) == 0);
  return 0;
}

static int test_no_args() {
  EXPECT(0 == 1 + 1);
  return 0;
}

int main(int argc, char **argv) {
  TEST(test_capacity);
  TEST(test_no_args);
}

/*
int main(int argc, char **argv) {
  int N = 256;
  osc_packet packet;
  packet.data = malloc(N);
  char v[3] = { 65, 66, 0 };
  int r1 = osc_pack_message(&packet, N, "/f*o/bar", "ibsfi",
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
*/
