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
  toml::Result result = toml::parse_file_ex("simple.toml");

  // Check for parse error
  if (!result.ok()) {
    error(result.errmsg());
  }

  // Extract values
  std::string host;
  std::vector<int64_t> port;

  try {
    // use the Datum::get() method
    host = result.get({"server", "host"})->as_str().value();
  } catch (const std::bad_optional_access &ex) {
    error("missing or invalid 'server.host' property in config");
  }

  try {
    // use the Datum::seek() method
    port = result.seek("server.port")->as_intvec().value();
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

  // Done! No need to call toml_free(). Result::~Result() will handle
  // destruction.
  return 0;
}
