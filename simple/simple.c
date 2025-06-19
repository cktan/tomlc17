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
    printf("%s%d", i ? ", " : "", (int)elem.u.int64);
  }
  printf("]\n");

  // Done!
  toml_free(result);
  return 0;
}
