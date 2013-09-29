#include "tinyosc.h"

#include "test_utils.h"

#define CAPACITY 256

static int buffers_match(const char *p, const char *q, int n) {
  int i;
  for (i = 0; i < n; ++i) {
    if (p[i] != q[i]) {
      int j;
      for (j = 0; j < n; ++j) {
        fprintf(stderr, "%x ", p[j]);
      }
      fprintf(stderr, "\n");
      for (j = 0; j < n; ++j) {
        fprintf(stderr, "%x ", q[j]);
      }
      fprintf(stderr, "\n");
      return 0;
    }
  }
  return 1;
}

static int test_pack_errors() {
  char data[CAPACITY];
  osc_packet packet;
  packet.data = data;

  EXPECT(osc_pack_message(&packet, CAPACITY, "", "") != 0);
  EXPECT(osc_pack_message(&packet, CAPACITY, "#", "") != 0);
  EXPECT(osc_pack_message(&packet, CAPACITY, "#foo", "") != 0);
  EXPECT(osc_pack_message(&packet, CAPACITY, "/foo", "_") != 0);
  
  return 0;
}

static int test_pack_capacity() {
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

  EXPECT(osc_pack_message(&packet, CAPACITY, "/", "") == 0);

  EXPECT(osc_pack_message(&packet, CAPACITY, "/ab", "") == 0);
  EXPECT(packet.size == 8);
  char ref0[] = { '/', 'a', 'b', 0, ',', 0, 0, 0 };
  EXPECT(buffers_match(ref0, packet.data, packet.size));

  EXPECT(osc_pack_message(&packet, CAPACITY, "/foo/bar", "") == 0);
  EXPECT(packet.size == 16);
  char ref1[] = {
    '/', 'f', 'o', 'o', '/', 'b', 'a', 'r', 0, 0, 0, 0,
    ',', 0, 0, 0
  };
  EXPECT(buffers_match(ref1, packet.data, packet.size));

  return 0;
}

static int test_pack_one_arg() {
  char data[CAPACITY];
  osc_packet packet;
  packet.data = data;

  EXPECT(osc_pack_message(&packet, CAPACITY, "/ab", "i", 0x12345678) == 0);
  EXPECT(packet.size == 12);
  char ref0[] = {
    '/', 'a', 'b', 0, ',', 'i', 0, 0,
    0x12, 0x34, 0x56, 0x78
  };
  EXPECT(buffers_match(ref0, packet.data, packet.size));

  EXPECT(osc_pack_message(&packet, CAPACITY, "/abc", "m", 0x01020304) == 0);
  EXPECT(packet.size == 16);
  char ref1[] = {
    '/', 'a', 'b', 'c', 0, 0, 0, 0, ',', 'm', 0, 0,
    0x01, 0x02, 0x03, 0x04
  };
  EXPECT(buffers_match(ref1, packet.data, packet.size));

  EXPECT(osc_pack_message(&packet, CAPACITY, "/ab", "f", 1.234) == 0);
  EXPECT(packet.size == 12);
  char ref2[] = {
    '/', 'a', 'b', 0, ',', 'f', 0, 0,
    0x3f, 0x9d, 0xf3, 0xb6
  };
  EXPECT(buffers_match(ref2, packet.data, packet.size));

  EXPECT(osc_pack_message(&packet, CAPACITY, "/ab", "s", "") == 0);
  EXPECT(packet.size == 12);
  char ref3[] = {
    '/', 'a', 'b', 0, ',', 's', 0, 0,
    0, 0, 0, 0
  };
  EXPECT(buffers_match(ref3, packet.data, packet.size));

  EXPECT(osc_pack_message(&packet, CAPACITY, "/ab", "s", "xyz") == 0);
  EXPECT(packet.size == 12);
  char ref4[] = {
    '/', 'a', 'b', 0, ',', 's', 0, 0,
    'x', 'y', 'z', 0
  };
  EXPECT(buffers_match(ref4, packet.data, packet.size));

  EXPECT(osc_pack_message(&packet, CAPACITY, "/ab", "s", "abcdef") == 0);
  EXPECT(packet.size == 16);
  char ref5[] = {
    '/', 'a', 'b', 0, ',', 's', 0, 0,
    'a', 'b', 'c', 'd', 'e', 'f', 0, 0
  };
  EXPECT(buffers_match(ref5, packet.data, packet.size));

  return 0;
}

int main(int argc, char **argv) {
  TEST(test_pack_errors);
  TEST(test_pack_capacity);
  TEST(test_pack_no_args);
  TEST(test_pack_one_arg);
}
