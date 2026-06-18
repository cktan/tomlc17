#include "../../src/tomlc17.c"

static void failed(const char *msg) {
  printf("FAILED: %s\n", msg);
  exit(1);
}

#define CHECK(x)                                                               \
  if (x)                                                                       \
    ;                                                                          \
  else                                                                         \
    failed(#x)

static void test_named_scalars(void) {
  printf("Running test_named_scalars...\n");
  const char *doc = "title = \"hi\"\n"
                    "[owner]\n"
                    "name = \"Alice\"\n"
                    "[[pkg]]\n"
                    "n = 1\n"
                    "nums = [1, 2, 3]\n"
                    "inl = { a = 1, b = 2 }\n";
  toml_result_t r = toml_parse_named(doc, (int)strlen(doc), "a.toml");
  CHECK(r.ok);
  CHECK(r.toptab.source && 0 == strcmp(r.toptab.source, "a.toml"));

  toml_datum_t title = toml_get(r.toptab, "title");
  CHECK(title.type == TOML_STRING);
  CHECK(title.source && 0 == strcmp(title.source, "a.toml"));

  toml_datum_t owner = toml_get(r.toptab, "owner");
  CHECK(owner.type == TOML_TABLE);
  CHECK(owner.source && 0 == strcmp(owner.source, "a.toml"));
  toml_datum_t oname = toml_get(owner, "name");
  CHECK(oname.source && 0 == strcmp(oname.source, "a.toml"));

  toml_datum_t pkg = toml_get(r.toptab, "pkg");
  CHECK(pkg.type == TOML_ARRAY && pkg.u.arr.size == 1);
  CHECK(pkg.u.arr.elem[0].source &&
        0 == strcmp(pkg.u.arr.elem[0].source, "a.toml"));

  // inline array: datum and elements carry source
  toml_datum_t nums = toml_get(pkg.u.arr.elem[0], "nums");
  CHECK(nums.type == TOML_ARRAY && nums.u.arr.size == 3);
  CHECK(nums.source && 0 == strcmp(nums.source, "a.toml"));
  CHECK(nums.u.arr.elem[1].source &&
        0 == strcmp(nums.u.arr.elem[1].source, "a.toml"));

  // inline table: datum and values carry source
  toml_datum_t inl = toml_get(pkg.u.arr.elem[0], "inl");
  CHECK(inl.type == TOML_TABLE && inl.u.tab.size == 2);
  CHECK(inl.source && 0 == strcmp(inl.source, "a.toml"));
  toml_datum_t inl_a = toml_get(inl, "a");
  CHECK(inl_a.type == TOML_INT64);
  CHECK(inl_a.source && 0 == strcmp(inl_a.source, "a.toml"));

  // interned: same pointer across datums of one document
  CHECK(title.source == oname.source);
  toml_free(r);
}

static void test_null_name(void) {
  printf("Running test_null_name...\n");
  const char *doc = "x = 1\n";
  toml_result_t r1 = toml_parse_named(doc, (int)strlen(doc), NULL);
  CHECK(r1.ok);
  CHECK(r1.toptab.source == NULL);
  CHECK(toml_get(r1.toptab, "x").source == NULL);
  toml_free(r1);

  toml_result_t r2 = toml_parse(doc, (int)strlen(doc));
  CHECK(r2.ok);
  CHECK(toml_get(r2.toptab, "x").source == NULL);
  toml_free(r2);
}

static void test_file_ex(void) {
  printf("Running test_file_ex...\n");
  const char *path = "src_test_tmp.toml";
  FILE *fp = fopen(path, "w");
  CHECK(fp != NULL);
  fputs("k = 42\n", fp);
  fclose(fp);

  toml_result_t r = toml_parse_file_ex(path);
  CHECK(r.ok);
  CHECK(r.toptab.source && 0 == strcmp(r.toptab.source, path));
  CHECK(toml_get(r.toptab, "k").source &&
        0 == strcmp(toml_get(r.toptab, "k").source, path));
  toml_free(r);
  remove(path);
}

static void test_merge_sources(void) {
  printf("Running test_merge_sources...\n");
  const char *a = "title = \"A\"\n"
                  "[owner]\n"
                  "name = \"Alice\"\n";
  const char *b = "version = \"1\"\n"
                  "[owner]\n"
                  "org = \"ACME\"\n";
  toml_result_t rA = toml_parse_named(a, (int)strlen(a), "a.toml");
  toml_result_t rB = toml_parse_named(b, (int)strlen(b), "b.toml");
  toml_result_t m = toml_merge(&rA, &rB);
  CHECK(m.ok);

  // free inputs first: merged result must be self-contained
  toml_free(rA);
  toml_free(rB);

  CHECK(0 == strcmp(toml_get(m.toptab, "title").source, "a.toml"));
  CHECK(0 == strcmp(toml_get(m.toptab, "version").source, "b.toml"));

  toml_datum_t owner = toml_get(m.toptab, "owner");
  toml_datum_t name = toml_get(owner, "name"); // from A
  toml_datum_t org = toml_get(owner, "org");   // from B
  CHECK(0 == strcmp(name.source, "a.toml"));
  CHECK(0 == strcmp(org.source, "b.toml"));

  // dedup: same-origin datums share a pointer
  CHECK(toml_get(m.toptab, "title").source == name.source);
  toml_free(m);
}

static void test_merge_array_of_tables_source(void) {
  printf("Running test_merge_array_of_tables_source...\n");
  const char *a = "[[pkg]]\n"
                  "name = \"left\"\n";
  const char *b = "[[pkg]]\n"
                  "name = \"right\"\n";
  toml_result_t rA = toml_parse_named(a, (int)strlen(a), "a.toml");
  toml_result_t rB = toml_parse_named(b, (int)strlen(b), "b.toml");
  toml_result_t m = toml_merge(&rA, &rB);
  CHECK(m.ok);

  // free inputs first: merged result must be self-contained
  toml_free(rA);
  toml_free(rB);

  toml_datum_t pkg = toml_get(m.toptab, "pkg");
  CHECK(pkg.type == TOML_ARRAY && pkg.u.arr.size == 2);
  // append semantics: A's element first, B's element appended
  CHECK(0 == strcmp(pkg.u.arr.elem[0].source, "a.toml"));
  CHECK(0 == strcmp(pkg.u.arr.elem[1].source, "b.toml"));
  CHECK(0 == strcmp(toml_get(pkg.u.arr.elem[0], "name").source, "a.toml"));
  CHECK(0 == strcmp(toml_get(pkg.u.arr.elem[1], "name").source, "b.toml"));
  toml_free(m);
}

int main(void) {
  test_named_scalars();
  test_null_name();
  test_file_ex();
  test_merge_sources();
  test_merge_array_of_tables_source();
  printf("OK\n");
  return 0;
}
