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

// A plain array overridden by an array of tables must be fully replaced,
// not have table elements appended onto the plain values.
static void test_plain_array_overridden_by_array_of_tables() {
  printf("Running test_plain_array_overridden_by_array_of_tables...\n");
  const char *doc1 = "arr = [1, 2, 3]";
  const char *doc2 = "[[arr]]\n"
                     "x = 1";
  const char *expected = "[[arr]]\n"
                         "x = 1";
  check(doc1, doc2, expected);
}

// The reverse direction: an array of tables overridden by a plain array
// must also be fully replaced, not have plain values appended onto the
// table elements.
static void test_array_of_tables_overridden_by_plain_array() {
  printf("Running test_array_of_tables_overridden_by_plain_array...\n");
  const char *doc1 = "[[arr]]\n"
                     "x = 1";
  const char *doc2 = "arr = [9, 8]";
  const char *expected = "arr = [9, 8]";
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

// Merged result must own its keys: tomlc17 requires each result be freed
// independently, so reading the merged tree after freeing the inputs must
// not touch the freed input pools.
static void test_keys_outlive_inputs() {
  printf("Running test_keys_outlive_inputs...\n");
  const char *doc1 = "[owner]\n"
                     "name = \"Alice\"\n";
  const char *doc2 = "[owner]\n"
                     "organization = \"ACME\"\n";
  toml_result_t r1 = toml_parse(doc1, strlen(doc1));
  toml_result_t r2 = toml_parse(doc2, strlen(doc2));
  toml_result_t merged = toml_merge(&r1, &r2);
  CHECK(merged.ok);

  toml_free(r1);
  toml_free(r2);

  toml_datum_t owner = toml_get(merged.toptab, "owner");
  toml_datum_t name = toml_get(owner, "name");
  toml_datum_t org = toml_get(owner, "organization");
  CHECK(name.type == TOML_STRING && 0 == strcmp(name.u.str.ptr, "Alice"));
  CHECK(org.type == TOML_STRING && 0 == strcmp(org.u.str.ptr, "ACME"));

  toml_free(merged);
}

int main() {
  test_simple_merge();
  test_overwrite_values();
  test_nested_tables();
  test_array_merging();
  test_deep_merge();
  test_complex_merge();
  test_array_of_tables();
  test_plain_array_overridden_by_array_of_tables();
  test_array_of_tables_overridden_by_plain_array();
  test_type_conflicts();
  test_empty_documents();
  test_keys_outlive_inputs();

  printf("All tests completed.\n");
  return 0;
}
