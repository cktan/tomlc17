#include "../../src/tomlc17.c"
#include <locale.h>

static void failed(const char *msg) {
  printf("FAILED: %s\n", msg);
  exit(1);
}

#define CHECK(x)                                                               \
  if (x)                                                                       \
    ;                                                                          \
  else                                                                         \
    failed(#x)

// Float literals must parse even when LC_NUMERIC uses ',' (issue #57).
static void test_comma_locale(void) {
  printf("Running test_comma_locale...\n");
  if (!setlocale(LC_NUMERIC, "de_DE.UTF-8") &&
      !setlocale(LC_NUMERIC, "de_DE") &&
      !setlocale(LC_NUMERIC, "fr_FR.UTF-8") &&
      !setlocale(LC_NUMERIC, "fr_FR")) {
    printf("SKIPPED: no comma-decimal locale installed\n");
    return;
  }
  CHECK(0 == strcmp(localeconv()->decimal_point, ","));

  const char *doc = "a = 1.5\n"
                    "b = -2_000.25e-1\n"
                    "c = 6.02e23\n"
                    "d = 1e3\n";
  toml_result_t r = toml_parse(doc, (int)strlen(doc));
  if (!r.ok) {
    failed(r.errmsg);
  }
  CHECK(toml_get(r.toptab, "a").u.fp64 == 1.5);
  CHECK(toml_get(r.toptab, "b").u.fp64 == -200.025);
  CHECK(toml_get(r.toptab, "c").u.fp64 == 6.02e23);
  CHECK(toml_get(r.toptab, "d").u.fp64 == 1e3);
  toml_free(r);

  setlocale(LC_NUMERIC, "C");
}

int main(void) {
  test_comma_locale();
  printf("All tests passed.\n");
  return 0;
}
