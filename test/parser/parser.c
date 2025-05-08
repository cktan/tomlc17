#include "../../src/tomlc17.c"
#include <inttypes.h>

const char **g_argv = 0;
int g_argc = 0;

static void usage() {
  fprintf(stderr, "Usage: %s [fname]\n", g_argv[0]);
  exit(1);
}

// Print a string in json. Properly escape special chars.
static void print_string(const char *s, int len) {
  printf("{\"type\": \"string\", \"value\": \"");
  for (int i = 0; i < len; i++) {
    int ch = s[i];
    if (isprint(ch) && ch != '\"' && ch != '\\') {
      putchar(ch);
      continue;
    }
    switch (ch) {
    case '"':
      putchar('\\');
      putchar('"');
      break; // Escape double quotes
    case '\\':
      putchar('\\');
      putchar('\\');
      break; // Escape backslash
    case '\b':
      putchar('\\');
      putchar('b');
      break; // Escape backspace
    case '\f':
      putchar('\\');
      putchar('f');
      break; // Escape formfeed
    case '\n':
      putchar('\\');
      putchar('n');
      break; // Escape newline
    case '\r':
      putchar('\\');
      putchar('r');
      break; // Escape carriage return
    case '\t':
      putchar('\\');
      putchar('t');
      break; // Escape tab
    default:
      if (0 <= ch && ch < ' ') {
        printf("\\u%04x", ch);
      } else {
        putchar(ch);
      }
      break;
    }
  }
  printf("\"}");
}

// Print a key
static void print_key(const char *s, int len) {
  putchar('"');
  for (int i = 0; i < len; i++) {
    int ch = s[i];
    if (isprint(ch) && ch != '\"' && ch != '\\') {
      putchar(ch);
      continue;
    }
    switch (ch) {
    case '"':
      putchar('\\');
      putchar('"');
      break; // Escape double quotes
    case '\\':
      putchar('\\');
      putchar('\\');
      break; // Escape backslash
    case '\b':
      putchar('\\');
      putchar('b');
      break; // Escape backspace
    case '\f':
      putchar('\\');
      putchar('f');
      break; // Escape formfeed
    case '\n':
      putchar('\\');
      putchar('n');
      break; // Escape newline
    case '\r':
      putchar('\\');
      putchar('r');
      break; // Escape carriage return
    case '\t':
      putchar('\\');
      putchar('t');
      break; // Escape tab
    default:
      if (0 <= ch && ch < ' ') {
        printf("\\u%04x", ch);
      } else {
        putchar(ch);
      }
      break;
    }
  }
  putchar('"');
}

// Print a DATE datum
static void print_date(toml_datum_t datum) {
  printf("{\"type\": \"date-local\", \"value\": \"%04d-%02d-%02d\"}",
         datum.u.ts.year, datum.u.ts.month, datum.u.ts.day);
}

// Print a TIME datum
static void print_time(toml_datum_t datum) {
  char fracstr[20];
  fracstr[0] = fracstr[1] = '\0';
  if (datum.u.ts.usec) {
    double f = datum.u.ts.usec / 1000000.0;
    snprintf(fracstr, sizeof(fracstr), "%.6f", f);
    fracstr[5] = '\0'; // millisec precision
  }
  printf("{\"type\": \"time-local\", \"value\": \"%02d:%02d:%02d%s\"}",
         datum.u.ts.hour, datum.u.ts.minute, datum.u.ts.second, fracstr + 1);
}

// Print a DATETIME datum
static void print_datetime(toml_datum_t datum) {
  char fracstr[20];
  fracstr[0] = fracstr[1] = '\0';
  if (datum.u.ts.usec) {
    double f = datum.u.ts.usec / 1000000.0;
    snprintf(fracstr, sizeof(fracstr), "%.6f", f);
    fracstr[5] = '\0'; // millisec precision
  }
  printf("{\"type\": \"datetime-local\", \"value\": \"%04d-%02d-%02d "
         "%02d:%02d:%02d%s\"}",
         datum.u.ts.year, datum.u.ts.month, datum.u.ts.day, datum.u.ts.hour,
         datum.u.ts.minute, datum.u.ts.second, fracstr + 1);
}

// Print a DATETIMETZ datum
static void print_datetimetz(toml_datum_t datum) {
  char fracstr[20];
  char tzstr[20];
  fracstr[0] = fracstr[1] = tzstr[0] = '\0';
  if (datum.u.ts.usec) {
    double f = datum.u.ts.usec / 1000000.0;
    snprintf(fracstr, sizeof(fracstr), "%.6f", f);
    fracstr[5] = '\0'; // millisec precision
  }
  int tz = datum.u.ts.tz;
  char sign = tz < 0 ? '-' : '+';
  tz = tz < 0 ? -tz : tz;
  snprintf(tzstr, sizeof(tzstr), "%c%02d:%02d", sign, tz / 60, tz % 60);

  printf("{\"type\": \"datetime\", \"value\": \"%04d-%02d-%02d "
         "%02d:%02d:%02d%s%s\"}",
         datum.u.ts.year, datum.u.ts.month, datum.u.ts.day, datum.u.ts.hour,
         datum.u.ts.minute, datum.u.ts.second, fracstr + 1, tzstr);
}

// Print indent spaces 
int indent_level = 0;
static int indent() {
  for (int i = 0; i < indent_level * 2; i++) {
    putchar(' ');
  }
  return 0;
}

// Print datum tree recursively
static void print_datum(toml_datum_t datum) {
  switch (datum.type) {
  case TOML_STRING:
    print_string(datum.u.str.ptr, datum.u.str.len);
    break;
  case TOML_INT64:
    printf("{\"type\": \"integer\", \"value\": \"%" PRId64 "\"}",
           datum.u.int64);
    break;
  case TOML_FP64:
    printf("{\"type\": \"float\", \"value\": \"%.16g%s\"}", datum.u.fp64,
           (datum.u.fp64 - (int64_t)datum.u.fp64 == 0) ? ".0" : "");
    break;
  case TOML_BOOLEAN:
    printf("{\"type\": \"bool\", \"value\": \"%s\"}",
           datum.u.boolean ? "true" : "false");
    break;
  case TOML_DATE:
    print_date(datum);
    break;
  case TOML_TIME:
    print_time(datum);
    break;
  case TOML_DATETIME:
    print_datetime(datum);
    break;
  case TOML_DATETIMETZ:
    print_datetimetz(datum);
    break;
  case TOML_ARRAY:
    printf("[");
    for (int i = 0; i < datum.u.arr.size; i++) {
      printf("%s", i ? ", " : "");
      print_datum(datum.u.arr.elem[i]);
    }
    printf("]");
    break;
  case TOML_TABLE:
    printf("{\n");
    indent_level++;
    for (int i = 0; i < datum.u.tab.size; i++) {
      printf("%s", i ? ",\n" : "");
      indent();
      print_key(datum.u.tab.key[i], datum.u.tab.len[i]);
      putchar(':');
      putchar(' ');
      print_datum(datum.u.tab.value[i]);
    }
    printf("\n");
    indent_level--;
    indent(), printf("}");
    break;
  default:
    fprintf(stderr, "ERROR: unimplemented datum type %d\n", datum.type);
    abort();
  }
}

int main(int argc, const char *argv[]) {
  g_argc = argc;
  g_argv = argv;
  if (argc > 2) {
    usage();
  }

  toml_option_t opt = toml_default_option();
  opt.check_utf8 = 1;
  toml_set_option(opt);

  toml_result_t result;
  if (argc == 2) {
    result = toml_parse_file_ex(argv[1]);
  } else {
    result = toml_parse_file(stdin);
  }

  if (!result.ok) {
    printf("%s\n", result.errmsg);
    exit(1);
  }

  print_datum(result.toptab);
  printf("\n");

  toml_free(result);
  return 0;
}
