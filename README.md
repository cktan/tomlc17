# tomlc17

TOML v1.1 in c17.

* Compatible with C99.
* Compatible with C++.
* Implements [C++20 Accessors](README_CXX.md).
* Implements [TOML v1.1](https://toml.io/en/v1.1.0).
* Passes the [standard test suites](https://github.com/toml-lang/toml-test/).


## Usage

See [API.md](API.md) for the full API reference.

Parsing a toml document creates a tree data structure in memory that
reflects the document. Information can be extracted by navigating this
data structure.

Note: you can simply include `tomlc17.h` and `tomlc17.c` in your
projects without building the library.

The following is a simple example:

```c
/*
 * Parse the config file simple.toml:
 *
 * [server]
 * host = "www.example.com"
 * port = [8080, 8181, 8282]
 *
 */
#include "../src/tomlc17.h"
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

static void error(const char *msg, const char *msg1) {
  fprintf(stderr, "ERROR: %s%s\n", msg, msg1 ? msg1 : "");
  exit(1);
}

int main() {
  // Parse the toml file
  toml_result_t result = toml_parse_file_ex("simple.toml");

  // Check for parse error
  if (!result.ok) {
    error(result.errmsg, 0);
  }

  // Extract values
  toml_datum_t host = toml_seek(result.toptab, "server.host");
  toml_datum_t port = toml_seek(result.toptab, "server.port");

  // Print server.host
  if (host.type != TOML_STRING) {
    error("missing or invalid 'server.host' property in config", 0);
  }
  printf("server.host = %s\n", host.u.s);

  // Print server.port
  if (port.type != TOML_ARRAY) {
    error("missing or invalid 'server.port' property in config", 0);
  }
  printf("server.port = [");
  for (int i = 0; i < port.u.arr.size; i++) {
    toml_datum_t elem = port.u.arr.elem[i];
    if (elem.type != TOML_INT64) {
      error("server.port element not an integer", 0);
    }
    printf("%s%" PRId64, i ? ", " : "", elem.u.int64);
  }
  printf("]\n");

  // Done!
  toml_free(result);
  return 0;
}
```


## Building

For debug build:
```bash
export DEBUG=1
make
```

For release build:
```bash
unset DEBUG
make
```

## Running tests

We run the official `toml-test` as described
[here](https://github.com/toml-lang/toml-test). Refer to
[this
section](https://github.com/toml-lang/toml-test?tab=readme-ov-file#installation)
for prerequisites to run the tests.

The following command invokes the tests:

```bash
make test
```

As of May 7, 2025, all tests passed for TOML v1.0:

```
toml-test v0001-01-01 [/home/cktan/p/tomlc17/test/stdtest/driver]: using embedded tests
  valid tests: 185 passed,  0 failed
invalid tests: 371 passed,  0 failed
```

As of Dec 25, 2025, all tests passed for TOML v1.1:

```
toml-test v0001-01-01 [/home/cktan/p/tomlc17/test/stdtest/driver] [no encoder]
  valid tests: 214 passed,  0 failed
encoder tests: no encoder command given
invalid tests: 466 passed,  0 failed
```

## Installing

The install command will copy `tomlc17.h`, `tomlcpp.hpp` and `libtomlc17.a` to the `$prefix/include` and `$prefix/lib` directories.

```bash
unset DEBUG
make clean install prefix=/usr/local
```

## Observations (by Claude)

A TOML 1.1 parser in C17. One implementation file (`tomlc17.c`, ~2950
lines), a public header (`tomlc17.h`), and a C++20 wrapper
(`tomlcpp.hpp`). The parser uses a single-pass scanner +
recursive-descent parser, a memory pool for strings, and
heap-allocated arrays for tables/arrays.

## Code Quality Observations (by Claude)

- **Memory model is clean**: pool for strings (freed atomically), heap
  arrays for table/array structure (freed recursively by
  `datum_free`). No mixing.

- **`toml_free` on failed results is safe**: the bail paths destroy
  the pool and leave `__internal = NULL`; `FREE(NULL)` is a no-op.

- **`toml_free` takes by value**: callers' structs are not zeroed
  after free. Expected for a C API, and documented.

- **Commented-out error checks** at lines 1184 and 1205–1207 note v1.1
  relaxations (trailing commas in inline tables, newlines in inline
  tables). These are intentional and should stay.

- **Pool size (`len + 10`)**: adequate — normalized string sizes are
  always ≤ raw token sizes (escapes compress), so total pool usage ≤
  source length.

- **Test coverage**: 39 parser regression cases, separate
  scankey/scanvalue suites, merge tests, C++ wrapper tests, and the
  upstream `toml-test` validator (TOML 1.1 mode).
