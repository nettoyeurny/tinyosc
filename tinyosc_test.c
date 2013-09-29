#include "tinyosc.h"

#include "test_utils.h"

#define CAPACITY 256

static int test_capacity() {
  char data[CAPACITY];
  osc_packet packet;
  packet.data = data;
  EXPECT(osc_pack_message(&packet, 0, "/ab", "") != 0);
  EXPECT(osc_pack_message(&packet, 4, "/ab", "") != 0);
  EXPECT(osc_pack_message(&packet, 8, "/ab", "") == 0);
  EXPECT(osc_pack_message(&packet, 4, "/foo", "") != 0);
  EXPECT(osc_pack_message(&packet, 8, "/foo", "") != 0);
  EXPECT(osc_pack_message(&packet, 12, "/foo", "") == 0);
  EXPECT(osc_pack_message(&packet, 12, "/foo", "i", 0) != 0);
  EXPECT(osc_pack_message(&packet, 16, "/foo", "i", 0) == 0);
  EXPECT(osc_pack_message(&packet, 12, "/foo", "f", 1.5) != 0);
  EXPECT(osc_pack_message(&packet, 16, "/foo", "f", 1.5) == 0);
  EXPECT(osc_pack_message(&packet, 12, "/foo", "s", "abc") != 0);
  EXPECT(osc_pack_message(&packet, 16, "/foo", "s", "abc") == 0);
  EXPECT(osc_pack_message(&packet, 16, "/foo", "s", "abcd") != 0);
  EXPECT(osc_pack_message(&packet, 20, "/foo", "s", "abcd") == 0);
  EXPECT(osc_pack_message(&packet, 20, "/foo", "b", 5, "abcde") != 0);
  EXPECT(osc_pack_message(&packet, 24, "/foo", "b", 5, "abcde") == 0);
  EXPECT(osc_pack_message(&packet, 16, "/foo", "ii", 0, 0) != 0);
  EXPECT(osc_pack_message(&packet, 20, "/foo", "ii", 0, 0) == 0);
  EXPECT(osc_pack_message(&packet, 16, "/foo", "fi", 1.5, 0) != 0);
  EXPECT(osc_pack_message(&packet, 20, "/foo", "fi", 1.5, 0) == 0);
  EXPECT(osc_pack_message(&packet, 16, "/foo", "si", "abc", 0) != 0);
  EXPECT(osc_pack_message(&packet, 20, "/foo", "si", "abc", 0) == 0);
  EXPECT(osc_pack_message(&packet, 24, "/foo", "bi", 5, "abcde", 0) != 0);
  EXPECT(osc_pack_message(&packet, 28, "/foo", "bi", 5, "abcde", 0) == 0);
  return 0;
}

static int test_pack_no_args() {
  char data[CAPACITY];
  osc_packet packet;
  packet.data = data;

  EXPECT(osc_pack_message(&packet, CAPACITY, "", "") != 0);
  EXPECT(osc_pack_message(&packet, CAPACITY, "/", "") == 0);
  EXPECT(osc_pack_message(&packet, CAPACITY, "#", "") != 0);
  EXPECT(osc_pack_message(&packet, CAPACITY, "#foo", "") != 0);

  EXPECT(osc_pack_message(&packet, CAPACITY, "/ab", "") == 0);
  EXPECT(packet.size == 8);
  char ref0[] = { '/', 'a', 'b', '\0', ',', '\0', '\0', '\0' };
  EXPECT(!strncmp(ref0, packet.data, packet.size));

  EXPECT(osc_pack_message(&packet, CAPACITY, "/foo/bar", "") == 0);
  EXPECT(packet.size == 16);
  char ref1[] = {
    '/', 'f', 'o', 'o', '/', 'b', 'a', 'r', '\0', '\0', '\0', '\0',
    ',', '\0', '\0', '\0'
  };
  EXPECT(!strncmp(ref1, packet.data, packet.size));
  return 0;
}

static int test_pack_one_args() {
  char data[CAPACITY];
  osc_packet packet;
  packet.data = data;

  return 0;
}

int main(int argc, char **argv) {
  TEST(test_capacity);
  TEST(test_pack_no_args);
  TEST(test_pack_one_args);
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
