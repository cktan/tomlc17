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

// A freshly created pool has its magic set and one small page ready to go.
static void test_create() {
  printf("Running test_create...\n");
  pool_t *pool = pool_create();
  CHECK(pool != NULL);
  CHECK(pool->magic == POOL_MAGIC);
  CHECK(pool->small != NULL);
  CHECK(pool->small->magic == PAGE_MAGIC);
  CHECK(pool->small->top == 0);
  CHECK(pool->small->max == PAGE_SMALL_SIZE);
  CHECK(pool->large == NULL);
  pool_destroy(pool);
}

// Small allocations are carved out of pool->small without changing pages,
// and each allocation gets distinct, non-overlapping memory.
static void test_small_alloc_no_growth() {
  printf("Running test_small_alloc_no_growth...\n");
  pool_t *pool = pool_create();
  page_t *page = pool->small;
  char *a = pool_alloc(pool, 10);
  char *b = pool_alloc(pool, 20);
  CHECK(a != NULL && b != NULL);
  CHECK(pool->small == page); // same page, no growth
  CHECK(pool->small->top == 30);
  CHECK(b == a + 10); // contiguous bump allocation
  memset(a, 'a', 10);
  memset(b, 'b', 20);
  CHECK(a[9] == 'a' && b[19] == 'b');
  pool_destroy(pool);
}

// An allocation exactly at the large threshold stays in the small page.
static void test_boundary_stays_small() {
  printf("Running test_boundary_stays_small...\n");
  pool_t *pool = pool_create();
  page_t *page = pool->small;
  char *p = pool_alloc(pool, PAGE_LARGE_THRESHOLD);
  CHECK(p != NULL);
  CHECK(pool->small == page);
  CHECK(pool->large == NULL);
  CHECK(pool->small->top == PAGE_LARGE_THRESHOLD);
  pool_destroy(pool);
}

// An allocation one byte over the large threshold gets its own exact-fit
// large page, and never touches the small list.
static void test_boundary_goes_large() {
  printf("Running test_boundary_goes_large...\n");
  pool_t *pool = pool_create();
  page_t *small_page = pool->small;
  int n = PAGE_LARGE_THRESHOLD + 1;
  char *p = pool_alloc(pool, n);
  CHECK(p != NULL);
  CHECK(pool->small == small_page); // untouched
  CHECK(pool->small->top == 0);
  CHECK(pool->large != NULL);
  CHECK(pool->large->magic == PAGE_MAGIC);
  CHECK(pool->large->max == n);
  CHECK(pool->large->top == n); // handed out in full immediately
  memset(p, 'x', n);
  CHECK(p[n - 1] == 'x');
  pool_destroy(pool);
}

// When a small allocation doesn't fit in the current page, a fresh page is
// linked in front and the old page (with its data) is left intact, not
// reused.
static void test_small_page_growth() {
  printf("Running test_small_page_growth...\n");
  pool_t *pool = pool_create();
  page_t *first_page = pool->small;

  // Fill the first page with under-threshold (still "small") chunks,
  // leaving less than one more chunk's worth of room.
  int chunk = PAGE_LARGE_THRESHOLD - 100; // 924 bytes, well under threshold
  int used = 0;
  char *a = NULL;
  while (used + chunk <= PAGE_SMALL_SIZE) {
    a = pool_alloc(pool, chunk);
    CHECK(a != NULL);
    CHECK(pool->small == first_page); // still fits in the first page
    used += chunk;
  }
  memset(a, 'a', chunk);
  CHECK(first_page->top == used);

  // Doesn't fit in the remaining space: forces a new page.
  char *b = pool_alloc(pool, chunk);
  CHECK(b != NULL);
  CHECK(pool->small != first_page);
  CHECK(pool->small->next == first_page);
  CHECK(pool->small->top == chunk);

  // Old page's data and leftover space are untouched, just abandoned.
  CHECK(first_page->top == used);
  CHECK(a[0] == 'a' && a[chunk - 1] == 'a');
  pool_destroy(pool);
}

// Multiple large allocations each get their own page, linked in a list.
static void test_multiple_large_allocs() {
  printf("Running test_multiple_large_allocs...\n");
  pool_t *pool = pool_create();
  int n1 = PAGE_LARGE_THRESHOLD + 1;
  int n2 = PAGE_LARGE_THRESHOLD + 100;
  char *p1 = pool_alloc(pool, n1);
  char *p2 = pool_alloc(pool, n2);
  CHECK(p1 != NULL && p2 != NULL);
  CHECK(pool->large->max == n2); // most recent large alloc is the head
  CHECK(pool->large->next != NULL);
  CHECK(pool->large->next->max == n1);
  CHECK(pool->large->next->next == NULL);
  pool_destroy(pool);
}

// End-to-end: a real parse exercises pool_alloc through parse_norm /
// dedup_source and must not corrupt the pool's magic.
static void test_parse_uses_pool() {
  printf("Running test_parse_uses_pool...\n");
  const char *doc = "s = \"hello world\"\nname = \"x\"\n";
  toml_result_t r = toml_parse(doc, (int)strlen(doc));
  CHECK(r.ok);
  pool_t *pool = (pool_t *)r.__internal;
  CHECK(pool->magic == POOL_MAGIC);
  toml_datum_t v = toml_get(r.toptab, "s");
  CHECK(v.type == TOML_STRING);
  CHECK(0 == strcmp(v.u.s, "hello world"));
  toml_free(r);
}

int main() {
  test_create();
  test_small_alloc_no_growth();
  test_boundary_stays_small();
  test_boundary_goes_large();
  test_small_page_growth();
  test_multiple_large_allocs();
  test_parse_uses_pool();
  printf("All tests completed.\n");
  return 0;
}
