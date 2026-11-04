/*
 * Tests for the tomlc99 compat layer, covering the API surface that
 * toml_cat does not exercise. Every check here also holds for the
 * original tomlc99, so this file compiles and passes against either.
 */
#include "toml.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void check_failed(const char *expr, int line) {
  printf("FAILED line %d: %s\n", line, expr);
  exit(1);
}

#define CHECK(x) ((x) ? (void)0 : check_failed(#x, __LINE__))

/* Counting allocator, installed via toml_set_memutil(). */
static int g_nalloc;

static void *count_malloc(size_t n) {
  g_nalloc++;
  return malloc(n);
}

static void count_free(void *p) {
  if (p) {
    g_nalloc--;
  }
  free(p);
}

static toml_table_t *parse(const char *src) {
  char errbuf[200];
  // toml_parse() takes a non-const char*
  char *buf = malloc(strlen(src) + 1);
  strcpy(buf, src);
  toml_table_t *tab = toml_parse(buf, errbuf, sizeof(errbuf));
  free(buf);
  if (!tab) {
    printf("parse error: %s\n", errbuf);
    exit(1);
  }
  return tab;
}

static void test_structure(void) {
  toml_table_t *root = parse("k1 = 1\n"
                             "t1 = {x = 1}\n"
                             "a1 = [1, 2]\n"
                             "k2 = 'two'\n"
                             "[t2]\n"
                             "[[aot]]\n"
                             "y = 1\n"
                             "[[aot]]\n");

  CHECK(toml_table_key(root) == NULL);
  CHECK(toml_table_nkval(root) == 2);
  CHECK(toml_table_narr(root) == 2);
  CHECK(toml_table_ntab(root) == 2);

  // key-values first, then arrays, then tables; each in source order
  const char *expect[] = {"k1", "k2", "a1", "aot", "t1", "t2"};
  for (int i = 0; i < 6; i++) {
    CHECK(0 == strcmp(toml_key_in(root, i), expect[i]));
  }
  CHECK(toml_key_in(root, 6) == NULL);

  CHECK(toml_key_exists(root, "k1"));
  CHECK(toml_key_exists(root, "aot"));
  CHECK(toml_key_exists(root, "t2"));
  CHECK(!toml_key_exists(root, "nope"));

  // lookups are by kind
  CHECK(toml_table_in(root, "t1") != NULL);
  CHECK(toml_table_in(root, "k1") == NULL);
  CHECK(toml_table_in(root, "a1") == NULL);
  CHECK(toml_array_in(root, "a1") != NULL);
  CHECK(toml_array_in(root, "t1") == NULL);
  CHECK(!toml_int_in(root, "t1").ok);
  CHECK(!toml_int_in(root, "a1").ok);

  toml_table_t *t1 = toml_table_in(root, "t1");
  CHECK(0 == strcmp(toml_table_key(t1), "t1"));
  CHECK(toml_int_in(t1, "x").ok && toml_int_in(t1, "x").u.i == 1);

  toml_array_t *aot = toml_array_in(root, "aot");
  CHECK(0 == strcmp(toml_array_key(aot), "aot"));
  CHECK(toml_array_kind(aot) == 't');
  CHECK(toml_array_type(aot) == 0);
  CHECK(toml_array_nelem(aot) == 2);
  CHECK(0 == strcmp(toml_table_key(toml_table_at(aot, 0)), "__anon__"));
  CHECK(toml_int_in(toml_table_at(aot, 0), "y").u.i == 1);
  CHECK(toml_table_nkval(toml_table_at(aot, 1)) == 0);
  CHECK(toml_table_at(aot, 2) == NULL);
  CHECK(toml_table_at(aot, -1) == NULL);
  CHECK(toml_array_at(aot, 0) == NULL);

  CHECK(toml_table_key(NULL) == NULL);
  CHECK(toml_array_key(NULL) == NULL);

  toml_free(root);
}

static void test_arrays(void) {
  toml_table_t *root =
      parse("i = [1, 2]\n"
            "d = [1.5]\n"
            "b = [true]\n"
            "s = ['x']\n"
            "tm = [07:32:00]\n"
            "dt = [1979-05-27]\n"
            "ts = [1979-05-27T07:32:00Z, 1979-05-27T07:32:00]\n"
            "mixed = [1, 'x']\n"
            "empty = []\n"
            "aa = [[1], [2, 3]]\n"
            "ta = [{x = 1}, {x = 2}]\n"
            "m = [1, [2], {x = 3}]\n");

  struct {
    const char *key;
    char kind, type;
  } expect[] = {
      {"i", 'v', 'i'},  {"d", 'v', 'd'},  {"b", 'v', 'b'},  {"s", 'v', 's'},
      {"tm", 'v', 't'}, {"dt", 'v', 'D'}, {"ts", 'v', 'T'}, {"mixed", 'v', 'm'},
      {"empty", 0, 0},  {"aa", 'a', 0},   {"ta", 't', 0},   {"m", 'm', 0},
  };
  for (size_t i = 0; i < sizeof(expect) / sizeof(expect[0]); i++) {
    toml_array_t *arr = toml_array_in(root, expect[i].key);
    CHECK(arr != NULL);
    CHECK(toml_array_kind(arr) == expect[i].kind);
    CHECK(toml_array_type(arr) == expect[i].type);
  }

  toml_array_t *aa = toml_array_in(root, "aa");
  toml_array_t *sub = toml_array_at(aa, 1);
  CHECK(sub != NULL && toml_array_key(sub) == NULL);
  CHECK(toml_array_nelem(sub) == 2 && toml_int_at(sub, 1).u.i == 3);

  toml_array_t *ta = toml_array_in(root, "ta");
  CHECK(toml_table_key(toml_table_at(ta, 0)) == NULL);
  CHECK(toml_int_in(toml_table_at(ta, 1), "x").u.i == 2);

  // a mixed array still gives access to each element by its own kind
  toml_array_t *m = toml_array_in(root, "m");
  CHECK(toml_int_at(m, 0).ok && toml_int_at(m, 0).u.i == 1);
  CHECK(toml_array_at(m, 1) != NULL && toml_table_at(m, 1) == NULL);
  CHECK(toml_table_at(m, 2) != NULL && !toml_int_at(m, 2).ok);
  CHECK(!toml_int_at(m, 3).ok);

  toml_array_t *mixed = toml_array_in(root, "mixed");
  toml_datum_t s = toml_string_at(mixed, 1);
  CHECK(s.ok && 0 == strcmp(s.u.s, "x"));
  count_free(s.u.s);
  CHECK(!toml_string_at(mixed, 0).ok);

  toml_free(root);
}

static void test_scalars(void) {
  toml_table_t *root = parse("s = \"a\\tb\"\n"
                             "i = -42\n"
                             "d = 2.5\n"
                             "b = false\n"
                             "big = 9_000\n");
  toml_datum_t d;

  d = toml_string_in(root, "s");
  CHECK(d.ok && 0 == strcmp(d.u.s, "a\tb"));
  count_free(d.u.s);

  d = toml_int_in(root, "i");
  CHECK(d.ok && d.u.i == -42);
  d = toml_double_in(root, "d");
  CHECK(d.ok && d.u.d == 2.5);
  d = toml_bool_in(root, "b");
  CHECK(d.ok && d.u.b == 0);

  // a decimal integer is also readable as a double
  d = toml_double_in(root, "big");
  CHECK(d.ok && d.u.d == 9000.0);

  // wrong types and missing keys
  CHECK(!toml_int_in(root, "d").ok);
  CHECK(!toml_int_in(root, "s").ok);
  CHECK(!toml_bool_in(root, "i").ok);
  CHECK(!toml_string_in(root, "i").ok);
  CHECK(!toml_double_in(root, "s").ok);
  CHECK(!toml_timestamp_in(root, "s").ok);
  CHECK(!toml_int_in(root, "nope").ok);

  toml_free(root);
}

static void test_timestamps(void) {
  toml_table_t *root = parse("odt = 1979-05-27T07:32:00.999999-07:30\n"
                             "utc = 1979-05-27T07:32:00Z\n"
                             "ldt = 1979-05-27T07:32:00\n"
                             "ld = 1979-05-27\n"
                             "lt = 00:32:00.5\n");
  toml_timestamp_t *ts;

  toml_datum_t d = toml_timestamp_in(root, "odt");
  CHECK(d.ok);
  ts = d.u.ts;
  CHECK(*ts->year == 1979 && *ts->month == 5 && *ts->day == 27);
  CHECK(*ts->hour == 7 && *ts->minute == 32 && *ts->second == 0);
  CHECK(ts->millisec && *ts->millisec == 999);
  CHECK(ts->z && 0 == strcmp(ts->z, "-07:30"));
  count_free(ts);

  d = toml_timestamp_in(root, "utc");
  CHECK(d.ok);
  ts = d.u.ts;
  CHECK(ts->z && 0 == strcmp(ts->z, "Z"));
  CHECK(ts->millisec == NULL);
  count_free(ts);

  d = toml_timestamp_in(root, "ldt");
  CHECK(d.ok);
  ts = d.u.ts;
  CHECK(ts->year && ts->hour && ts->z == NULL);
  count_free(ts);

  d = toml_timestamp_in(root, "ld");
  CHECK(d.ok);
  ts = d.u.ts;
  CHECK(*ts->year == 1979 && ts->hour == NULL && ts->minute == NULL);
  CHECK(ts->second == NULL && ts->millisec == NULL && ts->z == NULL);
  count_free(ts);

  d = toml_timestamp_in(root, "lt");
  CHECK(d.ok);
  ts = d.u.ts;
  CHECK(ts->year == NULL && ts->month == NULL && ts->day == NULL);
  CHECK(*ts->hour == 0 && *ts->minute == 32 && *ts->second == 0);
  CHECK(ts->millisec && *ts->millisec == 500);
  CHECK(ts->z == NULL);
  count_free(ts);

  toml_free(root);
}

static void test_errors(void) {
  char errbuf[200];
  char bad[] = "a = 1\nb = = 2\n";
  CHECK(toml_parse(bad, errbuf, sizeof(errbuf)) == NULL);
  CHECK(strstr(errbuf, "line 2") != NULL);

  // a zero-size errbuf is never written
  char untouched[1] = {'x'};
  CHECK(toml_parse(bad, untouched, 0) == NULL);
  CHECK(untouched[0] == 'x');

  // errbuf is cleared on success
  char good[] = "a = 1\n";
  strcpy(errbuf, "stale");
  toml_table_t *tab = toml_parse(good, errbuf, sizeof(errbuf));
  CHECK(tab && errbuf[0] == 0);
  toml_free(tab);

  toml_free(NULL);
}

static void test_parse_file(void) {
  FILE *fp = tmpfile();
  CHECK(fp != NULL);
  fputs("[server]\nport = 8080\n", fp);
  rewind(fp);
  char errbuf[200];
  toml_table_t *root = toml_parse_file(fp, errbuf, sizeof(errbuf));
  fclose(fp);
  CHECK(root != NULL);
  toml_table_t *server = toml_table_in(root, "server");
  CHECK(server && toml_int_in(server, "port").u.i == 8080);
  toml_free(root);
}

static void test_utf8(void) {
  struct {
    int64_t code;
    const char *utf8;
  } cases[] = {
      {0x41, "A"},
      {0x80, "\xc2\x80"},
      {0x7ff, "\xdf\xbf"},
      {0x800, "\xe0\xa0\x80"},
      {0xfffd, "\xef\xbf\xbd"},
      {0x10000, "\xf0\x90\x80\x80"},
      {0x10ffff, "\xf4\x8f\xbf\xbf"},
  };
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
    char buf[6];
    int64_t code;
    int n = strlen(cases[i].utf8);
    CHECK(toml_ucs_to_utf8(cases[i].code, buf) == n);
    CHECK(0 == memcmp(buf, cases[i].utf8, n));
    CHECK(toml_utf8_to_ucs(buf, n, &code) == n && code == cases[i].code);
  }
  int64_t code;
  CHECK(toml_utf8_to_ucs("\xc2", 1, &code) == -1); // truncated
  CHECK(toml_utf8_to_ucs("\x80", 1, &code) == -1); // stray continuation
  char buf[6];
  CHECK(toml_ucs_to_utf8(-1, buf) == -1);
}

int main(void) {
  // Must precede any parse. All allocations below go through it.
  toml_set_memutil(count_malloc, count_free);

  test_structure();
  test_arrays();
  test_scalars();
  test_timestamps();
  test_errors();
  test_parse_file();
  test_utf8();

  // Everything the library allocated has been released, and the
  // strings and timestamps it returned came from count_malloc.
  CHECK(g_nalloc == 0);

  printf("unittest: all passed\n");
  return 0;
}
