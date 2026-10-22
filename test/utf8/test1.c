#include "../../src/tomlc17.c"

// Validate raw UTF-8 and the Unicode scalar range of escaped values.

static void failed(const char *msg) {
  printf("FAILED: %s\n", msg);
  exit(1);
}

#define CHECK(x)                                                               \
  if (x)                                                                       \
    ;                                                                          \
  else                                                                         \
    failed(#x)

// Pass the document length excluding the trailing NUL to toml_parse().
static toml_result_t parse_raw(const char *prefix, const char *bytes, int n,
                               const char *suffix) {
  int plen = (int)strlen(prefix), slen = (int)strlen(suffix);
  int len = plen + n + slen;
  char *buf = (char *)malloc((size_t)len + 1);
  if (!buf)
    failed("out of memory");
  memcpy(buf, prefix, (size_t)plen);
  memcpy(buf + plen, bytes, (size_t)n);
  memcpy(buf + plen + n, suffix, (size_t)slen);
  buf[len] = 0;
  toml_result_t r = toml_parse(buf, len);
  free(buf);
  return r;
}

static void expect_reject_raw(const char *what, const char *bytes, int n) {
  toml_result_t r = parse_raw("a = \"", bytes, n, "\"\n");
  if (r.ok) {
    printf("FAILED: accepted invalid UTF-8 (%s)\n", what);
    toml_free(r);
    exit(1);
  }
  toml_free(r);
}

static void expect_accept_raw(const char *what, const char *bytes, int n) {
  toml_result_t r = parse_raw("a = \"", bytes, n, "\"\n");
  if (!r.ok) {
    printf("FAILED: rejected valid UTF-8 (%s): %s\n", what, r.errmsg);
    toml_free(r);
    exit(1);
  }
  toml_datum_t v = toml_get(r.toptab, "a");
  CHECK(v.type == TOML_STRING);
  CHECK(v.u.str.len == n);
  CHECK(0 == memcmp(v.u.str.ptr, bytes, (size_t)n));
  toml_free(r);
}

static void test_raw_utf8(void) {
  printf("Running test_raw_utf8...\n");

  toml_option_t opt = toml_default_option();
  opt.check_utf8 = true;
  toml_set_option(opt);

  // Invalid: overlong encodings of ASCII values.
  static const char ol_nul[] = {(char)0xC0, (char)0x80};
  static const char ol_slash2[] = {(char)0xC0, (char)0xAF};
  static const char ol_slash3[] = {(char)0xE0, (char)0x80, (char)0xAF};
  static const char ol_slash4[] = {(char)0xF0, (char)0x80, (char)0x80,
                                   (char)0xAF};
  // Invalid: above U+10FFFF.
  static const char above_max[] = {(char)0xF4, (char)0x90, (char)0x80,
                                   (char)0x80};
  // Invalid: 0xF5..0xF7 lead bytes can only encode values > U+10FFFF.
  static const char lead_f5[] = {(char)0xF5, (char)0x80, (char)0x80,
                                 (char)0x80};
  static const char lead_f7[] = {(char)0xF7, (char)0xBF, (char)0xBF,
                                 (char)0xBF};

  expect_reject_raw("overlong NUL", ol_nul, (int)sizeof(ol_nul));
  expect_reject_raw("overlong '/' (2 bytes)", ol_slash2,
                    (int)sizeof(ol_slash2));
  expect_reject_raw("overlong '/' (3 bytes)", ol_slash3,
                    (int)sizeof(ol_slash3));
  expect_reject_raw("overlong '/' (4 bytes)", ol_slash4,
                    (int)sizeof(ol_slash4));
  expect_reject_raw("above U+10FFFF", above_max, (int)sizeof(above_max));
  expect_reject_raw("0xF5 lead byte", lead_f5, (int)sizeof(lead_f5));
  expect_reject_raw("0xF7 lead byte", lead_f7, (int)sizeof(lead_f7));
  expect_reject_raw("surrogate", "\xED\xA0\x80", 3);
  expect_reject_raw("continuation as lead", "\x80", 1);
  expect_reject_raw("invalid continuation", "\xC2\x41", 2);
  expect_reject_raw("truncated sequence", "\xF0\x90\x80", 3);

  // Valid: multi-byte sequences at the edges of their ranges.
  static const char two_byte_min[] = {(char)0xC2, (char)0x80}; // U+0080
  static const char three_byte_min[] = {(char)0xE0, (char)0xA0,
                                        (char)0x80}; // U+0800
  static const char four_byte_min[] = {(char)0xF0, (char)0x90, (char)0x80,
                                       (char)0x80}; // U+10000
  static const char max_scalar[] = {(char)0xF4, (char)0x8F, (char)0xBF,
                                    (char)0xBF}; // U+10FFFF
  expect_accept_raw("U+0080", two_byte_min, (int)sizeof(two_byte_min));
  expect_accept_raw("U+0800", three_byte_min, (int)sizeof(three_byte_min));
  expect_accept_raw("U+10000", four_byte_min, (int)sizeof(four_byte_min));
  expect_accept_raw("U+10FFFF", max_scalar, (int)sizeof(max_scalar));
  expect_accept_raw("U+07FF", "\xDF\xBF", 2);
  expect_accept_raw("U+D7FF", "\xED\x9F\xBF", 3);
  expect_accept_raw("U+E000", "\xEE\x80\x80", 3);
  expect_accept_raw("U+FFFF", "\xEF\xBF\xBF", 3);

  toml_set_option(toml_default_option()); // restore defaults
}

static void test_escape_range(void) {
  printf("Running test_escape_range...\n");

  // \U escapes above U+10FFFF are not Unicode scalar values. This holds with
  // and without check_utf8: the document itself is valid UTF-8 (ASCII).
  const char *bad = "a = \"\\U00110000\"\n";
  const char *bad2 = "a = \"\\U001FFFFF\"\n";
  const char *good = "a = \"\\U0010FFFF\"\n";

  for (int pass = 0; pass < 2; pass++) {
    toml_option_t opt = toml_default_option();
    opt.check_utf8 = (pass == 1);
    toml_set_option(opt);

    toml_result_t r = toml_parse(bad, (int)strlen(bad));
    CHECK(!r.ok);
    toml_free(r);

    r = toml_parse(bad2, (int)strlen(bad2));
    CHECK(!r.ok);
    toml_free(r);

    r = toml_parse(good, (int)strlen(good));
    CHECK(r.ok);
    toml_datum_t v = toml_get(r.toptab, "a");
    CHECK(v.type == TOML_STRING);
    static const char max_scalar[] = {(char)0xF4, (char)0x8F, (char)0xBF,
                                      (char)0xBF};
    CHECK(v.u.str.len == (int)sizeof(max_scalar));
    CHECK(0 == memcmp(v.u.str.ptr, max_scalar, sizeof(max_scalar)));
    toml_free(r);
  }

  toml_set_option(toml_default_option());
}

int main(void) {
  test_raw_utf8();
  test_escape_range();
  printf("All tests passed.\n");
  return 0;
}
