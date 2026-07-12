# tomlc17

[![C/C++ CI](https://github.com/cktan/tomlc17/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/cktan/tomlc17/actions/workflows/c-cpp.yml)
[![Latest release](https://img.shields.io/github/v/tag/cktan/tomlc17?label=release)](https://github.com/cktan/tomlc17/tags)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![TOML v1.1](https://img.shields.io/badge/TOML-v1.1-9c4221.svg)](https://toml.io/en/v1.0.0)

A lightweight, strictly compliant TOML v1.1 parser for C and C++.

## Overview

`tomlc17` parses TOML documents into an in-memory tree structure for
straightforward navigation. It is optimized for clean integration and
efficient execution, utilizing a single-pass scanner, a dedicated
string memory pool, and safe recursive teardowns.

* **Compliance:** Fully implements TOML v1.1 and passes the standard
`toml-test` validation suite.
* **Compatibility:** Written in C17. Fully compatible with C99 and C++.
* **Modern C++ Support:** Includes dedicated C++20 accessors (see
`README_CXX.md`).
* **Zero-Friction Integration:** Amalgamated design. Simply drop
`tomlc17.h` and `tomlc17.c` into your source tree, or build it as a
library.

## Quick Start

For complete API details, refer to [`API.md`](API.md).

### Example: Parsing & Extraction

Parsing a toml document creates a tree data structure in memory that
reflects the document. Information can be extracted by navigating this
data structure.


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

The install command will copy `tomlc17.h`, `tomlcpp.hpp` and
`libtomlc17.a` to the `$prefix/include` and `$prefix/lib` directories.

```bash
unset DEBUG
make clean install prefix=/usr/local
```

## Options

For information on configuring library options, such as setting custom memory 
allocators, see [`OPTIONS.md`](OPTIONS.md).
