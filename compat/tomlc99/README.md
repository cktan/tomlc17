# tomlc99 compatibility layer

> **EXPERIMENTAL:** this layer is new and has not yet been used in
> production. Its API and behavior may change. Test your application
> thoroughly before relying on it.

A drop-in replacement for [tomlc99](https://github.com/cktan/tomlc99),
built on tomlc17. Code written against tomlc99 keeps its
`#include "toml.h"` and its calls unchanged; it is only recompiled and
linked against `libtoml` from this directory instead of tomlc99's.

## Building and installing

```bash
make -C compat/tomlc99                               # libtoml.a, libtoml.so.1.0
make -C compat/tomlc99 install prefix=/usr/local     # toml.h + libtoml
```

`toml.h` here is tomlc99's header, unchanged.

## What is supported

All of the tomlc99 API, except the deprecated raw API
(`toml_raw_in`, `toml_raw_at`, `toml_rtos`, `toml_rtob`, `toml_rtoi`,
`toml_rtod`, `toml_rtod_ex`, `toml_rtots`), which is not provided.
Programs that use it will fail to link.

## Differences from tomlc99

- **Parsing follows tomlc17**, which is strict TOML v1.1. Some
  documents tomlc99 accepted are now rejected, e.g. out-of-range dates
  and times (`1979-02-30`, `24:00:00`), malformed numbers (`0x`,
  `1e_2`), control characters in comments or strings, and tables
  extended through dotted keys.
  Some it rejected are now accepted: TOML v1.1 syntax (newlines and
  trailing commas in inline tables, times without seconds, `\x`
  escapes), and valid documents that tomlc99 rejected by mistake.
- **Error messages** come from tomlc17, e.g. `(line 3) duplicate key`
  rather than `line 3: key exists`. Some errors tomlc99 only reported
  when a bad value was read (`toml_*_in` returning `ok == 0`) are now
  reported by `toml_parse`.
- **Timestamps**: a zero UTC offset is always reported in `z` as `"Z"`,
  even if written as `+00:00`. `millisec` is NULL when the fraction of
  a second is zero, even if one was written (e.g. `07:32:00.000`).
- **`toml_double_in`/`_at`** accept any integer, including hex, octal
  and binary ones, which tomlc99 rejected.
- **`toml_utf8_to_ucs`/`toml_ucs_to_utf8`** handle code points up to
  U+10FFFF only (at most 4 bytes), and reject overlong encodings.
- **One API per program**: tomlc99's API and tomlc17's both define
  `toml_parse`, `toml_free` and `toml_datum_t`, so a program cannot use
  this layer and tomlc17's own API together.

As with tomlc99, `toml_set_memutil()` must be called before any parse,
and keys or strings containing U+0000 are cut short at the NUL.

## Tests

`make -C compat/tomlc99 test` builds tomlc99's own `toml_cat` program,
unchanged, against this layer and checks its output on tomlc99's
`stdex/` cases, then runs `test/unittest.c`. The unit test passes
against the original tomlc99 as well.
