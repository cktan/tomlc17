#include "../../src/tomlc17.c"
#include <inttypes.h>

static void failed() {
  printf("FAILED\n");
  exit(1);
}

#define CHECK(x)                                                               \
  if (x)                                                                       \
    ;                                                                          \
  else                                                                         \
    failed()

static void check(const char *doc1, const char *doc2, const char *expected) {
  toml_result_t r1 = toml_parse(doc1, strlen(doc1));
  toml_result_t r2 = toml_parse(doc2, strlen(doc2));
  toml_result_t merged = toml_merge(&r1, &r2);
  toml_result_t exp = toml_parse(expected, strlen(expected));
  CHECK(toml_equiv(&merged, &exp));
  toml_free(r1);
  toml_free(r2);
  toml_free(merged);
  toml_free(exp);
}

// All test cases as separate functions
static void test_simple_merge() {
  printf("Running test_simple_merge...\n");
  const char *doc1 = "title = \"First\"";
  const char *doc2 = "version = \"1.0\"";
  const char *expected = "title = \"First\"\n"
                         "version = \"1.0\"";
  check(doc1, doc2, expected);
}

static void test_overwrite_values() {
  printf("Running test_overwrite_values...\n");
  const char *doc1 = "title = \"First\"\n"
                     "version = \"0.9\"";
  const char *doc2 = "version = \"1.0\"";
  const char *expected = "title = \"First\"\n"
                         "version = \"1.0\"";
  check(doc1, doc2, expected);
}

static void test_nested_tables() {
  printf("Running test_nested_tables...\n");
  const char *doc1 = "[owner]\n"
                     "name = \"Alice\"\n"
                     "dob = \"1979-05-27\"";
  const char *doc2 = "[owner]\n"
                     "organization = \"ACME\"";
  const char *expected = "[owner]\n"
                         "name = \"Alice\"\n"
                         "dob = \"1979-05-27\"\n"
                         "organization = \"ACME\"";
  check(doc1, doc2, expected);
}

static void test_array_merging() {
  printf("Running test_array_merging...\n");
  const char *doc1 = "dependencies = [\"lib1\", \"lib2\"]";
  const char *doc2 = "dependencies = [\"lib3\"]";
  const char *expected = "dependencies = [\"lib3\"]";
  check(doc1, doc2, expected);
}

static void test_deep_merge() {
  printf("Running test_deep_merge...\n");
  const char *doc1 = "[database]\n"
                     "server = \"localhost\"\n"
                     "ports = [8000, 8001]\n"
                     "connection_max = 5000";
  const char *doc2 = "[database]\n"
                     "ports = [8001, 8002]\n"
                     "enabled = true";
  const char *expected = "[database]\n"
                         "server = \"localhost\"\n"
                         "ports = [8001, 8002]\n"
                         "connection_max = 5000\n"
                         "enabled = true";
  check(doc1, doc2, expected);
}

static void test_complex_merge() {
  printf("Running test_complex_merge...\n");
  const char *doc1 = "title = \"TOML Example\"\n"
                     "\n"
                     "[owner]\n"
                     "name = \"Tom Preston-Werner\"\n"
                     "dob = 1979-05-27T07:32:00Z\n"
                     "\n"
                     "[database]\n"
                     "server = \"192.168.1.1\"\n"
                     "ports = [8001, 8001, 8002]\n"
                     "connection_max = 5000\n"
                     "enabled = true";

  const char *doc2 = "title = \"Updated TOML Example\"\n"
                     "\n"
                     "[owner]\n"
                     "organization = \"GitHub\"\n"
                     "\n"
                     "[database]\n"
                     "ports = [9000]\n"
                     "enabled = false\n"
                     "\n"
                     "[clients]\n"
                     "data = [[\"gamma\", \"delta\"], [1, 2]]";

  const char *expected = "title = \"Updated TOML Example\"\n"
                         "\n"
                         "[owner]\n"
                         "name = \"Tom Preston-Werner\"\n"
                         "dob = 1979-05-27T07:32:00Z\n"
                         "organization = \"GitHub\"\n"
                         "\n"
                         "[database]\n"
                         "server = \"192.168.1.1\"\n"
                         "ports = [9000]\n"
                         "connection_max = 5000\n"
                         "enabled = false\n"
                         "\n"
                         "[clients]\n"
                         "data = [[\"gamma\", \"delta\"], [1, 2]]";
  check(doc1, doc2, expected);
}

static void test_array_of_tables() {
  printf("Running test_array_of_tables...\n");
  const char *doc1 = "[[products]]\n"
                     "name = \"Hammer\"\n"
                     "sku = 738594937";
  const char *doc2 = "[[products]]\n"
                     "color = \"red\"";
  const char *expected = "[[products]]\n"
                     "name = \"Hammer\"\n"
                     "sku = 738594937\n"
    "[[products]]\n"
                         "color = \"red\"";
  check(doc1, doc2, expected);
}

static void test_type_conflicts() {
  printf("Running test_type_conflicts...\n");
  const char *doc1 = "value = 42";
  const char *doc2 = "value = \"forty-two\"";
  const char *expected = "value = \"forty-two\"";
  check(doc1, doc2, expected);
}

static void test_empty_documents() {
  printf("Running test_empty_documents...\n");
  check("", "a = 1", "a = 1");
  check("a = 1", "", "a = 1");
  check("", "", "");
}

int main() {
  test_simple_merge();
  test_overwrite_values();
  test_nested_tables();
  test_array_merging();
  test_deep_merge();
  test_complex_merge();
  test_array_of_tables();
  test_type_conflicts();
  test_empty_documents();

  printf("All tests completed.\n");
  return 0;
}
