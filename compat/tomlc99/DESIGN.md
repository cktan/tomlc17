# DESIGN.md — tomlc99 compatibility layer

How `compat/tomlc99` puts tomlc99's API on top of tomlc17. Read the
top-level [`DESIGN.md`](../../DESIGN.md) first for tomlc17 itself.
For what the layer does *not* match, see [`README.md`](README.md).

## Layout

- `toml.h` — tomlc99's public header, copied verbatim. User code
  includes this, exactly as it did with tomlc99.
- `toml.c` — the whole layer, one translation unit. It contains all
  of tomlc17 (by `#include`) plus the tomlc99 API built over it.
- `test/` — tomlc99's own `toml_cat.c` and `stdex/` cases, and
  `unittest.c` for the rest of the API.

The build produces `libtoml.a` / `libtoml.so.1.0`, the same names
tomlc99 used, so switching is a relink.

## 1. Two APIs, one set of names

tomlc99 and tomlc17 both define `toml_parse`, `toml_parse_file`,
`toml_free` and `toml_datum_t`, with different signatures and
meanings. They cannot be declared in the same translation unit or
linked into the same program as-is.

The layer resolves this without touching tomlc17's sources. `toml.c`
starts by renaming every public tomlc17 name with the preprocessor,
then includes tomlc17's implementation directly:

```c
#define toml_datum_t toml17_datum_t
#define toml_parse   toml17_parse
#define toml_free    toml17_free
/* ... every other public tomlc17 name, and DATUM_ZERO ... */
#include "../../src/tomlc17.c"
#undef toml_datum_t
#undef toml_parse
/* ... */
#include "toml.h"          /* tomlc99's names, now free to use */
```

Because macros act on tokens, the rename reaches the declarations in
`tomlc17.h`, the definitions in `tomlc17.c`, and every call between
them. After the `#undef`s, the rest of `toml.c` sees tomlc99's
`toml_datum_t` and defines tomlc99's `toml_parse`/`toml_free`, while
still calling tomlc17 as `toml17_parse`/`toml17_free`.

Including the `.c` file (instead of linking `libtomlc17`) has two
side benefits:

- tomlc17's `static` helpers are in scope. The layer reuses
  `utf8_to_ucs`/`ucs_to_utf8` for tomlc99's
  `toml_utf8_to_ucs`/`toml_ucs_to_utf8`, and writes the `toml_option`
  allocator hook directly.
- The library is self-contained: users link only `libtoml`.

All public tomlc17 names are renamed, not just the four that clash.
Otherwise the library would also export `toml_get`, `toml_merge` and
so on, which could collide with helpers in tomlc99 users' own code.
The exported `toml_*` symbols are exactly tomlc99's API; the rest are
`toml17_*`.

(Making tomlc17's functions `static` instead does not work:
`tomlc17.h` defines `TOML_EXTERN` unconditionally, overriding any
earlier definition.)

## 2. Handles: the mirror tree

tomlc99 hands out opaque `toml_table_t *` and `toml_array_t *`
handles and answers questions tomlc17's tree cannot answer directly:

- **A node's own key** (`toml_table_key`, `toml_array_key`). tomlc17
  stores keys in the parent's `key[]` array; a child has no parent
  link and does not know its name.
- **Entries grouped by kind.** `toml_key_in(tab, i)` enumerates
  key-values first, then arrays, then tables, and
  `toml_table_nkval`/`narr`/`ntab` count each group. tomlc17 keeps one
  list in source order.
- **Array kind and type** (`toml_array_kind`, `toml_array_type`),
  which tomlc17 does not record.

So after a successful parse, `make_root` walks the tomlc17 tree once
and builds a parallel *mirror tree* of small nodes, one per table and
per array (scalars get none):

```c
struct toml_table_t {
  const char *key;          /* this table's own key, or NULL */
  const toml17_datum_t *d;  /* the tomlc17 TOML_TABLE datum it wraps */
  toml_result_t *res;       /* root only: the owning tomlc17 result */
  int nkval, narr, ntab;
  void **child;  /* child[i] wraps d->u.tab.value[i] if table/array */
  int *order;    /* indices into d->u.tab: key-values, arrays, tables */
};

struct toml_array_t {
  const char *key;          /* NULL for arrays nested in arrays */
  const toml17_datum_t *d;  /* the tomlc17 TOML_ARRAY datum it wraps */
  char kind, type;          /* tomlc99's kind/type codes */
  void **child;  /* child[i] wraps d->u.arr.elem[i] if table/array */
};
```

```
 handle (mirror)                 tomlc17 tree (owned by the result)
 ───────────────                 ──────────────────────────────────
 toml_table_t root ── d ──────▶  toptab   key[] = {"server","x"}
   child[0] ─┐                            value[0] = TABLE ─┐
   child[1] = NULL                        value[1] = INT64  │
             │                                              │
             ▼                                              ▼
 toml_table_t "server" ─ d ───────────────────────▶  TABLE {port = 8080}
```

Each mirror node only *points into* the tomlc17 tree: keys, strings
and values are never copied at build time. `mirror_table` and
`mirror_array` do the work:

- **Keys.** A table's or array's key is the parent's `key[i]`. Tables
  that are elements of an array get tomlc99's names: `"__anon__"` for
  each `[[header]]` table, and NULL for an inline `{...}` table. The
  two are told apart by tomlc17's `TOML_FLAG_INLINED` bit. The root
  table's key is NULL.
- **Grouping.** `order[]` holds the table's indices in three passes —
  scalars, then arrays, then tables — so `toml_key_in(tab, i)` is
  `d->u.tab.key[order[i]]`.
- **Kind and type.** Computed from the element types as tomlc99 did:
  kind is `'v'`/`'a'`/`'t'`, or `'m'` once two kinds mix, or 0 for
  an empty array. For value arrays, type is the one-letter code of
  the first element (`i d b s t D T`), or `'m'` once two types mix.
  Both tomlc17 datetime types (local and offset) map to `'T'`.

Each node and its `child[]`/`order[]` arrays come from a single
allocation. The tree is built eagerly, so lookups never allocate and
`toml_table_in`/`toml_array_in`/`toml_table_at`/`toml_array_at` just
return `child[i]` after checking the datum's type.

## 3. Lifetime and ownership

`make_root` copies the tomlc17 `toml_result_t` into a heap block and
points the root mirror node at it (`root->res`), so the root's `d` is
`&res->toptab`. The result therefore stays put for as long as the
handle lives.

`toml_free(root)` tears down in this order: free the mirror tree
(recursively, child before parent), then `toml17_free(*res)` to
release tomlc17's tree and string pool, then the `res` block. As in
tomlc99, `toml_free` is for the root only, and every handle obtained
from that root becomes invalid.

## 4. Reading values

The scalar getters (`toml_string_in`, `toml_int_at`, ...) all go
through two lookup helpers and five converters:

- `value_in(tab, key)` and `value_at(arr, idx)` return the tomlc17
  datum, or NULL if the key or index is absent or names a table or
  array. Key lookup (`find_key`) compares by length and `memcmp`, as
  `toml_get` does.
- `get_string`, `get_bool`, `get_int`, `get_double`, `get_timestamp`
  turn a datum into a tomlc99 `toml_datum_t`, with `ok = 0` if the
  datum is NULL or of the wrong type.

tomlc99 parsed values lazily from raw text, so its getters' rules are
those of its `toml_rto*` converters. The converters reproduce those
rules from tomlc17's typed values:

- **Strings** are copied into a fresh `malloc`'d buffer (via the
  memutil allocator), because tomlc99 callers `free()` `u.s`.
- **Doubles** accept integers too, since tomlc99 ran `strtod` on the
  raw text and so read `42` as `42.0`.
- **Timestamps** are allocated the same way and filled the way
  tomlc99 did: date fields only for dates, time fields only for
  times, both for datetimes, each `int *` pointing into the struct's
  own `__buffer`. `millisec` is set when the fraction is non-zero.
  `z` is rebuilt from tomlc17's offset in minutes, as `"Z"` or
  `"+HH:MM"`.

## 5. Parsing and errors

`toml_parse` and `toml_parse_file` call `toml17_parse` /
`toml17_parse_file`, then `make_root`. On failure, tomlc17's
`errmsg` is copied into the caller's `errbuf` and NULL is returned.
As in tomlc99, `errbuf` is cleared first and never written when
`errbufsz <= 0`. tomlc99 took a NUL-terminated string; its length is
checked against `INT_MAX` because tomlc17's `toml_parse` takes an
`int` length.

All parsing rules are tomlc17's — the layer adds no compatibility
code to the parser.

## 6. Custom allocators

tomlc99's `toml_set_memutil` takes a `malloc` and a `free`. tomlc17's
allocator hook needs a `realloc` and a `free`, and a `realloc` cannot
be built from `malloc`/`free` without knowing each block's old size.

So `toml_set_memutil` installs `memutil_realloc`/`memutil_free` into
tomlc17's `toml_option`. These prefix every block with a small
header (`memhdr_t`, a union with `max_align_t` to keep the user
block aligned) that records its size. Growing a block allocates a new
one with the user's `malloc`, copies, and frees the old one with the
user's `free`.

The layer's own allocations — mirror nodes, the result block, and
the strings and timestamps returned to the user — use the user's
`malloc`/`free` directly, without a header, so the user can `free`
returned values with their own `free`, as with tomlc99.

When `toml_set_memutil` is never called, tomlc17 keeps using plain
`realloc`/`free` and there is no header overhead. As with tomlc99, it
must be called before any parse: a block allocated before the switch
would otherwise be freed by the wrong function.

## 7. What is not carried over

- **The deprecated raw API** (`toml_raw_in`, `toml_raw_at`,
  `toml_rto*`) is not implemented. tomlc17 does not keep each value's
  source text, so `toml_raw_in` would have to re-serialize values.
  Programs using it fail to link, rather than misbehave.
- **Mixing APIs.** One program cannot use this layer and tomlc17's
  own API together, because the two still export different functions
  named `toml_parse` and `toml_free`.

## Testing

- `test/toml_cat.c` is tomlc99's own program, unmodified. Building it
  against the layer shows that tomlc99 code compiles unchanged;
  `test/run.sh` compares its output on tomlc99's `stdex/` cases with
  the expected `*.res` files. For cases that expect an error, only
  the rejection is checked, because the message text comes from
  tomlc17.
- `test/unittest.c` covers what `toml_cat` does not: keys and names,
  array kind/type, group counts, all timestamp forms, error paths,
  the UTF-8 helpers, and `toml_set_memutil`, counting allocations to
  confirm every one is freed. It compiles and passes against the
  original tomlc99 too, which keeps its expectations honest.
