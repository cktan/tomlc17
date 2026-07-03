#include "../../src/tomlc17.c"

static void failed() {
  printf("FAILED\n");
  exit(1);
}

#define CHECK(x)                                                               \
  if (x)                                                                       \
    ;                                                                          \
  else                                                                         \
    failed()

static bool equiv(const char *doc1, const char *doc2) {
  toml_result_t r1 = toml_parse(doc1, strlen(doc1));
  toml_result_t r2 = toml_parse(doc2, strlen(doc2));
  CHECK(r1.ok);
  CHECK(r2.ok);
  bool eq = toml_equiv(&r1, &r2);
  toml_free(r1);
  toml_free(r2);
  return eq;
}

// Tables are unordered maps: same keys in a different order are equivalent.
static void test_key_reorder() {
  printf("Running test_key_reorder...\n");
  CHECK(equiv("a = 1\nb = 2\n", "b = 2\na = 1\n"));
}

static void test_nested_reorder() {
  printf("Running test_nested_reorder...\n");
  const char *doc1 = "[t]\n"
                     "x = 1\n"
                     "y = 2\n";
  const char *doc2 = "[t]\n"
                     "y = 2\n"
                     "x = 1\n";
  CHECK(equiv(doc1, doc2));
}

// Keyvals and subtables may appear in any relative order.
static void test_keyval_and_subtable_reorder() {
  printf("Running test_keyval_and_subtable_reorder...\n");
  const char *doc1 = "[t]\n"
                     "a = 1\n"
                     "[t.sub]\n"
                     "b = 2\n";
  const char *doc2 = "[t.sub]\n"
                     "b = 2\n"
                     "[t]\n"
                     "a = 1\n";
  CHECK(equiv(doc1, doc2));
}

static void test_different_value() {
  printf("Running test_different_value...\n");
  CHECK(!equiv("a = 1\nb = 2\n", "a = 1\nb = 3\n"));
}

static void test_missing_key() {
  printf("Running test_missing_key...\n");
  CHECK(!equiv("a = 1\nb = 2\n", "a = 1\n"));
}

// Same size, different key names.
static void test_different_key() {
  printf("Running test_different_key...\n");
  CHECK(!equiv("a = 1\nb = 2\n", "a = 1\nc = 2\n"));
}

// Arrays stay order-sensitive.
static void test_array_order_matters() {
  printf("Running test_array_order_matters...\n");
  CHECK(equiv("x = [1, 2, 3]\n", "x = [1, 2, 3]\n"));
  CHECK(!equiv("x = [1, 2, 3]\n", "x = [3, 2, 1]\n"));
}

int main() {
  test_key_reorder();
  test_nested_reorder();
  test_keyval_and_subtable_reorder();
  test_different_value();
  test_missing_key();
  test_different_key();
  test_array_order_matters();

  printf("All tests completed.\n");
  return 0;
}
