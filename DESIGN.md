# DESIGN.md

High-level tour of `tomlc17`'s implementation. Read this before making
structural changes to the parser, scanner, or memory model.

## Layout

- `src/tomlc17.h` — public API: `toml_datum_t` (the value tree node),
  `toml_result_t` (parse/merge output), `toml_parse*`/`toml_merge`/
  `toml_get`/`toml_seek`/`toml_free`, and the `toml_option_t` allocator
  hook.
- `src/tomlc17.c` — the entire implementation, single translation
  unit, ~3200 lines. Everything below lives here and is `static`
  unless noted.

The library has no build-time configuration and no dependencies
beyond the C standard library. A parse (or merge) produces one
self-contained `toml_result_t`; the only public teardown call is
`toml_free()`.

## Big components, in the order data flows through them

### 1. Scanner (`scanner_t`, `scan_*`)

A hand-written lexer over the raw source buffer (`scanner_t.cur` /
`.endp`, around `tomlc17.c:277`). `scan_next()` (`tomlc17.c:2890`) is
the entry point and dispatches to per-token-kind scanners:
`scan_string`, `scan_litstring`, `scan_multiline_string`/
`_litstring`, `scan_number`, `scan_float`, `scan_bool`,
`scan_timestamp`/`scan_time`, `scan_key`/`scan_value` (keymode vs.
value-mode tokenization differ, e.g. bare keys vs. literals).

Tokens (`token_t`, `tomlc17.c:289`) are lightweight: a `toktyp_t`, a
`span_t` (pointer + length into the *original* source buffer, never
copied), plus scanned-out fields for numbers/timestamps. String
tokens still point at raw source bytes at this stage — escape
processing happens later in `parse_norm`.

`scan_mark`/`scan_restore` snapshot and rewind scanner position,
used by the parser for lookahead (e.g. distinguishing `{` inline
table from other value forms).

### 2. Parser (`parser_t`, `parse_*`)

Recursive-descent over the token stream. `parser_t` (`tomlc17.c:347`)
holds the scanner, the in-progress document tree (`toptab`/`curtab`),
the error buffer, and the memory pool (`pool_t`, see below).

Entry point is `toml_parse_named()` (`tomlc17.c:1024`), which
allocates a `parser_t` on the stack, initializes the scanner and
pool, then drives:

- `parse_std_table_expr` / `parse_array_table_expr` — `[table]` and
  `[[array.of.tables]]` headers, which call `descend_keypart` to
  walk/create nested tables along a dotted key path.
- `parse_keyvalue_expr` — `key = value` lines.
- `parse_val` — dispatches a value token to `parse_inline_array`,
  `parse_inline_table`, or a `token_to_*` scalar conversion
  (`token_to_string`, `_int64`, `_fp64`, `_boolean`, `_timestamp`).
- `parse_norm` (`tomlc17.c:1780`) — un-escapes a string/literal token
  into pool-owned storage; this is the only place scanned text is
  copied rather than referenced in place.

The output tree is `toml_datum_t` (public type, `tomlc17.h:66`): a
tagged union node. Tables and arrays are built incrementally via
`tab_emplace`/`tab_add` (`tomlc17.c:377,431`) and `arr_emplace`
(`tomlc17.c:448`), which grow their parallel `key[]`/`len[]`/
`value[]` (table) or `elem[]` (array) arrays through the `cell_t`
allocator.

### 3. Memory model — two different allocators for two different jobs

**`cell_t`** (`tomlc17.c:179`) is a classic growable buffer: a small
header (`top`, `max`) prefixed to the data, `cell_realloc` grows it
by `size*1.3+100` when it overflows (`tomlc17.c:185`), `cell_free`
releases it. Used for anything that needs to *grow and be freed
individually*: a table's `key[]`/`len[]`/`value[]` arrays, an array's
`elem[]` array, and `srcmap_t`'s memo arrays during merge. These are
freed piecemeal in `datum_free` (`tomlc17.c:499`) as the tree is torn
down.

**`pool_t`** (`tomlc17.c:90`) is an arena for the *string bytes*
themselves (unescaped string/literal contents, table keys, source
names) — data that is written once and never resized or individually
freed. Both `pool_t` and `page_t` carry a `magic` field checked by
`assert()` in `pool_alloc`/`pool_destroy` (cleared on destroy) to catch
corruption, use-after-free, and double-destroy. A pool is two linked
lists of `page_t` (`tomlc17.c:82`):

- `pool->small` — 4KB (`PAGE_SMALL_SIZE`) pages, bump-allocated
  piecemeal via `pool_alloc`. When the current page doesn't have
  room, its leftover space is abandoned and a fresh page is linked in
  front — there's no back-search for space in older pages.
- `pool->large` — allocations over 1KB (`PAGE_LARGE_THRESHOLD`) each
  get their own exactly-sized page, used once.

This split exists so a single huge string (or a huge document overall)
never forces one giant up-front `malloc`: pages accumulate
incrementally as parsing/copying proceeds, and an oversized value gets
a right-sized page instead of inflating a shared arena. `pool_destroy`
walks both lists freeing every page, then frees the `pool_t` header.
There is no way to free an individual `pool_alloc`'d string; the pool
is torn down all at once, from `toml_free()` or an error path.

The pool is reached from outside this file only through
`toml_result_t.__internal` (`tomlc17.h:120`, opaque `void*`) — it's
set to the `pool_t*` at the end of a successful parse (`tomlc17.c:1135`)
or merge (`tomlc17.c:819`) and read back by `toml_free()` to destroy
it. This is the one place internal-knowledge
coupling crosses the `parser_t`/public-API boundary.

Both allocators go through the pluggable hook `toml_option.mem_realloc`/
`mem_free` (default `realloc`/`free`, `tomlc17.c:19`), settable via
`toml_set_option()`. That global is unsynchronized — set it once at
startup, not concurrently with parsing.

### 4. Merge (`toml_merge`)

Combines two already-parsed results into a third, independent one:
deep-copies `r1`'s tree via `datum_copy`, then folds `r2` into it via
`datum_merge` (arrays-of-tables append, tables recurse, everything
else overrides). Both operations pull all string bytes through
`pool_alloc` into a fresh pool created for the merge result —
`srcmap_t`/`dedup_source` (`tomlc17.c:520,531`) additionally
deduplicate repeated `source` filename pointers so the same filename
string isn't copied once per datum that references it. The two input
results remain independently owned; merge never frees or aliases into
their pools (`test/merge/test1.c:test_keys_outlive_inputs` guards this
invariant).

### 5. Equivalence (`toml_equiv` / `datum_equiv`)

Structural comparison of two trees (`tomlc17.c:694`), tables compared
as unordered maps (so key order doesn't matter), arrays compared
positionally (order does matter there).

## Threading

No locks anywhere. Each `pool_t`/`parser_t` is owned by exactly one
parse or merge call, so concurrent parses of independent inputs are
safe. The one shared, unsynchronized piece of global state is
`toml_option` (the allocator hook) — don't call `toml_set_option()`
concurrently with any parse/merge/free.
