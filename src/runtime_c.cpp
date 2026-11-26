// The C runtime that every natively compiled Moss program embeds.
//
// It is emitted verbatim at the top of the generated .c file, so a built
// program is a single self-contained source file with no external
// dependencies beyond the C standard library.
extern const char* const kRuntimePrelude;

const char* const kRuntimePrelude = R"MOSSC(
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { const char* p; int64_t n; } moss_str;

static moss_str moss_str_make(const char* p, int64_t n) {
  moss_str s;
  s.p = p;
  s.n = n;
  return s;
}

static moss_str moss_str_cat(moss_str a, moss_str b) {
  char* buf = (char*)malloc((size_t)(a.n + b.n + 1));
  if (!buf) { fputs("moss: out of memory\n", stderr); exit(1); }
  if (a.n) memcpy(buf, a.p, (size_t)a.n);
  if (b.n) memcpy(buf + a.n, b.p, (size_t)b.n);
  buf[a.n + b.n] = '\0';
  return moss_str_make(buf, a.n + b.n);
}

static bool moss_str_eq(moss_str a, moss_str b) {
  return a.n == b.n && (a.n == 0 || memcmp(a.p, b.p, (size_t)a.n) == 0);
}

static void moss_print_int(int64_t v) { printf("%lld\n", (long long)v); }
static void moss_print_bool(bool v) { printf("%s\n", v ? "true" : "false"); }
static void moss_print_str(moss_str s) {
  if (s.n) fwrite(s.p, 1, (size_t)s.n, stdout);
  putchar('\n');
}
static void moss_print_inline(moss_str s) {
  if (s.n) fwrite(s.p, 1, (size_t)s.n, stdout);
  fflush(stdout);
}

static void moss_panic(const char* msg) {
  fprintf(stderr, "moss: runtime error: %s\n", msg);
  exit(1);
}

static int64_t moss_div(int64_t a, int64_t b) {
  if (b == 0) moss_panic("division by zero");
  return a / b;
}
static int64_t moss_mod(int64_t a, int64_t b) {
  if (b == 0) moss_panic("division by zero");
  return a % b;
}

static int64_t moss_read_int(void) {
  long long v;
  if (scanf("%lld", &v) != 1) moss_panic("failed to read an int");
  return (int64_t)v;
}

static bool moss_read_bool(void) {
  char buf[16];
  if (scanf("%15s", buf) != 1) moss_panic("failed to read a bool");
  if (!strcmp(buf, "true") || !strcmp(buf, "1")) return true;
  if (!strcmp(buf, "false") || !strcmp(buf, "0")) return false;
  moss_panic("expected true/false");
  return false;
}

static bool moss_is_space(int c) {
  return c == ' ' || c == '\n' || c == '\t' || c == '\r';
}

static moss_str moss_read_str(void) {
  size_t cap = 64, n = 0;
  char* buf = (char*)malloc(cap);
  int c;
  if (!buf) moss_panic("out of memory");
  while ((c = getchar()) != EOF && moss_is_space(c)) { }
  if (c == EOF) moss_panic("failed to read a string");
  while (c != EOF && !moss_is_space(c)) {
    if (n + 1 >= cap) {
      cap *= 2;
      buf = (char*)realloc(buf, cap);
      if (!buf) moss_panic("out of memory");
    }
    buf[n++] = (char)c;
    c = getchar();
  }
  if (c != EOF) ungetc(c, stdin);
  buf[n] = '\0';
  return moss_str_make(buf, (int64_t)n);
}

static void moss_skip_sep(moss_str sep) {
  int64_t matched = 0;
  while (matched < sep.n) {
    int c = getchar();
    if (c == EOF) moss_panic("input ended while looking for a separator");
    if ((char)c == sep.p[matched]) ++matched;
    else matched = (sep.n > 1 && (char)c == sep.p[0]) ? 1 : 0;
  }
}

typedef struct { int64_t* data; int64_t len; } moss_arr_int;
typedef struct { bool* data; int64_t len; } moss_arr_bool;
typedef struct { moss_str* data; int64_t len; } moss_arr_str;

static int64_t moss_index(int64_t i, int64_t n) {
  if (i < 0 || i >= n) {
    char buf[96];
    snprintf(buf, sizeof buf, "array index %lld out of range (length %lld)",
             (long long)i, (long long)n);
    moss_panic(buf);
  }
  return i;
}

static moss_arr_int moss_arr_int_new(int64_t n) {
  moss_arr_int a;
  if (n < 0) moss_panic("negative array length");
  a.len = n;
  a.data = (int64_t*)calloc((size_t)(n ? n : 1), sizeof(int64_t));
  if (!a.data) moss_panic("out of memory");
  return a;
}

static moss_arr_bool moss_arr_bool_new(int64_t n) {
  moss_arr_bool a;
  if (n < 0) moss_panic("negative array length");
  a.len = n;
  a.data = (bool*)calloc((size_t)(n ? n : 1), sizeof(bool));
  if (!a.data) moss_panic("out of memory");
  return a;
}

static moss_arr_str moss_arr_str_new(int64_t n) {
  moss_arr_str a;
  if (n < 0) moss_panic("negative array length");
  a.len = n;
  a.data = (moss_str*)calloc((size_t)(n ? n : 1), sizeof(moss_str));
  if (!a.data) moss_panic("out of memory");
  return a;
}

// Accessors keep the array expression evaluated exactly once on the C side.
static int64_t moss_at_int(moss_arr_int a, int64_t i) {
  return a.data[moss_index(i, a.len)];
}
static void moss_set_int(moss_arr_int a, int64_t i, int64_t v) {
  a.data[moss_index(i, a.len)] = v;
}
static bool moss_at_bool(moss_arr_bool a, int64_t i) {
  return a.data[moss_index(i, a.len)];
}
static void moss_set_bool(moss_arr_bool a, int64_t i, bool v) {
  a.data[moss_index(i, a.len)] = v;
}
static moss_str moss_at_str(moss_arr_str a, int64_t i) {
  return a.data[moss_index(i, a.len)];
}
static void moss_set_str(moss_arr_str a, int64_t i, moss_str v) {
  a.data[moss_index(i, a.len)] = v;
}

static moss_str moss_join_args(int argc, char** argv) {
  moss_str s = moss_str_make("", 0);
  for (int i = 1; i < argc; ++i) {
    if (i > 1) s = moss_str_cat(s, moss_str_make(" ", 1));
    s = moss_str_cat(s, moss_str_make(argv[i], (int64_t)strlen(argv[i])));
  }
  return s;
}
)MOSSC";
