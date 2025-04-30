#include "tomlc17.c"
#include <inttypes.h>
#include <stdlib.h>

const char **g_argv = 0;
int g_argc = 0;

static void usage() {
  fprintf(stderr, "Usage: %s fname\n", g_argv[0]);
  exit(1);
}

static void printspecial(const char *p, int n) {
  for (int i = 0; i < n; i++, p++) {
    int ch = (*p == '\n') ? '_' : *p;
    putchar(ch);
  }
}

static void printtok(char *content, const token_t tok) {
  // clang-format off
#define CASESTR(x) case x: s = #x; break
  // clang-format on
  char *s = 0;
  switch (tok.toktyp) {
    CASESTR(DOT);
    CASESTR(EQUAL);
    CASESTR(COMMA);
    CASESTR(LBRACK);
    CASESTR(RBRACK);
    CASESTR(LBRACE);
    CASESTR(RBRACE);
    CASESTR(STRING);
    CASESTR(MLSTRING);
    CASESTR(LITSTRING);
    CASESTR(MLLITSTRING);
    CASESTR(TIME);
    CASESTR(DATE);
    CASESTR(DATETIME);
    CASESTR(DATETIMETZ);
    CASESTR(INTEGER);
    CASESTR(FLOAT);
    CASESTR(BOOL);
    CASESTR(LIT);
    CASESTR(ENDL);
    CASESTR(FIN);
  default:
    s = "UNKNOWN";
    break;
  }
  printf("%s %ld %d ", s, tok.str.ptr - content, tok.str.len);
  printspecial(tok.str.ptr, tok.str.len);

  switch (tok.toktyp) {
  case INTEGER:
    printf(" %" PRId64, tok.u.int64);
    break;
  case FLOAT:
    printf(" %.16g%s", tok.u.fp64,
           (tok.u.fp64 - (int64_t)tok.u.fp64 == 0) ? ".0" : "");
    break;
  case BOOL:
    printf(" %s", tok.u.b1 ? "true" : "false");
    break;
  default:
    break;
  }
  printf("\n");
}

static char *readfile(const char *fname, int *ret_len) {
  FILE *fp = fopen(fname, "r");
  if (!fp) {
    perror("fopen");
    exit(1);
  }

  // Seek to the end of the file to determine its size
  if (fseek(fp, 0, SEEK_END) != 0) {
    perror("fseek");
    exit(1);
  }

  long file_size = ftell(fp);
  if (file_size == -1) {
    perror("ftell");
    exit(1);
  }
  rewind(fp); // Go back to the beginning of the file

  // Allocate memory for the file content, plus one for the null terminator
  char *content = malloc(file_size + 1);
  if (!content) {
    perror("out of memory");
    exit(1);
  }

  // Read the file into the buffer
  size_t read_size = fread(content, 1, file_size, fp);
  if (read_size != (size_t)file_size) {
    perror("fread");
    exit(1);
  }
  content[file_size] = '\0'; // Null-terminate the string
  fclose(fp);

  *ret_len = file_size;
  return content;
}

int main(int argc, const char *argv[]) {
  g_argc = argc;
  g_argv = argv;
  if (argc != 2) {
    usage();
  }
  int len;
  char *content = readfile(argv[1], &len);
  char errbuf[200];

  scanner_t scanner;
  scanner_t *sp = &scanner;
  scan_init(sp, content, len, errbuf, sizeof(errbuf));

  for (;;) {
    (void)scan_key; // silent compiler
    token_t tok;
    if (scan_value(sp, &tok)) {
      return -1;
    }
    if (tok.toktyp == FIN) {
      break;
    }
    printtok(content, tok);
  }

  if (sp->errmsg) {
    printf("ERROR: %s (line %d)\n", sp->errmsg, sp->lineno);
    exit(1);
  }

  free(content);
  return 0;
}
