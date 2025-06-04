# C++20 Accessors in tomlc17

## Usage

Convenient routines are provided to facilitate extraction of values from parsed result. Notably:

- provides direct access to values in subtables,
- utilizes C++ `std::optional` construct that throws exception on bad access,
- represents time and date using C++ `std::chrono` objects, and
- converts datum arrays into integer or string vectors.

Here is a simple example:

```c++
/*
 * Parse the config file simple.toml:
 *
 * [server]
 * host = "www.example.com"
 * port = [8080, 8181, 8282]
 *
 */
#include "../src/tomlcpp.hpp"
#include <iostream>
#include <vector>

using std::cout;

static void error(const char *msg) {
  fprintf(stderr, "ERROR: %s\n", msg);
  exit(1);
}

int main() {
  // Parse the toml file
  toml_result_t result = toml_parse_file_ex("simple.toml");

  // Check for parse error
  if (!result.ok) {
    error(result.errmsg);
  }

  // Extract values
  toml::Datum toptab(result.toptab);
  std::string host;
  std::vector<int64_t> port;

  try {
    host = *toptab.get({"server", "host"})->as_str();
  } catch (const std::bad_optional_access &ex) {
    error("missing or invalid 'server.host' property in config");
  }

  try {
    port = *toptab.get({"server", "port"})->as_intvec();
  } catch (const std::bad_optional_access &ex) {
    error("missing or invalid 'server.port' property in config");
  }


  // Print values
  cout << "server.host = " << host << "\n";
  cout << "server.port = [";
  for (size_t i = 0; i < port.size(); i++) {
    cout << (i ? ", " : "") << port[i];
  }
  cout << "]\n";

  // Done!
  toml_free(result);
  return 0;
}
```
