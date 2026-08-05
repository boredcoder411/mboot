#include "libc.h"

#define __NR_exit 1
#define __NR_read 3
#define __NR_write 4
#define __NR_open 5
#define __NR_close 6
#define __NR_exec 11

uint8_t read_key(void) {
  uint8_t key;
  asm volatile("int $0x80" : "=a"(key) : "a"(253));
  return key;
}

void sleep_ms(uint32_t ms) { asm volatile("int $0x80" : : "a"(252), "b"(ms)); }

int has_key(void) {
  int result;
  asm volatile("int $0x80" : "=a"(result) : "a"(251));
  return result;
}

ssize_t read(int fd, void *buf, size_t count) {
  ssize_t ret;
  asm volatile("int $0x80"
               : "=a"(ret)
               : "0"(__NR_read), "b"(fd), "c"(buf), "d"(count));
  return ret;
}

ssize_t write(int fd, const void *buf, size_t count) {
  ssize_t ret;
  asm volatile("int $0x80"
               : "=a"(ret)
               : "0"(__NR_write), "b"(fd), "c"(buf), "d"(count));
  return ret;
}

void exit(int code) {
  asm volatile("int $0x80" : : "a"(__NR_exit), "b"(code));
  while (1) {
    asm volatile("hlt");
  }
}

int open(const char *pathname, int flags) {
  int ret;
  asm volatile("int $0x80"
               : "=a"(ret)
               : "0"(__NR_open), "b"(pathname), "c"(flags), "d"(0));
  return ret;
}

int close(int fd) {
  int ret;
  asm volatile("int $0x80" : "=a"(ret) : "0"(__NR_close), "b"(fd));
  return ret;
}

void *memcpy(void *dst, const void *src, uint32_t size) {
  uint8_t *d = (uint8_t *)dst;
  const uint8_t *s = (const uint8_t *)src;
  for (uint32_t i = 0; i < size; i++) {
    d[i] = s[i];
  }
  return dst;
}

void *memset(void *ptr, int value, uint32_t size) {
  uint8_t *p = (uint8_t *)ptr;
  for (uint32_t i = 0; i < size; i++) {
    p[i] = (uint8_t)value;
  }
  return ptr;
}

char *strcpy(char *dst, const char *src) {
  char *d = dst;
  while (*src) {
    *d++ = *src++;
  }
  *d = '\0';
  return dst;
}

uint32_t strlen(const char *str) {
  uint32_t len = 0;
  while (str[len])
    len++;
  return len;
}

int strcmp(const char *str1, const char *str2) {
  while (*str1 && *str1 == *str2) {
    str1++;
    str2++;
  }
  return (int)*str1 - (int)*str2;
}

int abs(int x) { return x < 0 ? -x : x; }

static uint32_t seed = 0x12345678;

int rand(void) {
  seed = seed * 1103515245 + 12345;
  return (int)((seed >> 16) & 0x7fff);
}

void srand(uint32_t s) { seed = s; }

static uint8_t heap[65536];
static uint32_t heap_used = 0;

void *malloc(size_t size) {
  if (heap_used + size >= sizeof(heap)) {
    return 0;
  }
  void *ptr = &heap[heap_used];
  heap_used += size;
  return ptr;
}

void *calloc(size_t num, size_t size) {
  size_t total = num * size;
  void *ptr = malloc(total);
  if (ptr) {
    memset(ptr, 0, total);
  }
  return ptr;
}

void *realloc(void *ptr, size_t size) {
  if (!ptr) return malloc(size);
  void *new_ptr = malloc(size);
  if (new_ptr) {
    memcpy(new_ptr, ptr, size < heap_used ? size : heap_used);
  }
  return new_ptr;
}

void free(void *ptr) { (void)ptr; }

/* ===== errno ===== */
int errno;

/* ===== ctype ===== */
int isalnum(int c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'); }
int isalpha(int c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
int iscntrl(int c) { return (c >= 0 && c <= 31) || c == 127; }
int isdigit(int c) { return c >= '0' && c <= '9'; }
int isgraph(int c) { return c > 32 && c < 127; }
int islower(int c) { return c >= 'a' && c <= 'z'; }
int isprint(int c) { return c >= 32 && c < 127; }
int ispunct(int c) { return isgraph(c) && !isalnum(c); }
int isspace(int c) { return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r'; }
int isupper(int c) { return c >= 'A' && c <= 'Z'; }
int isxdigit(int c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
int tolower(int c) { return (c >= 'A' && c <= 'Z') ? c + 32 : c; }
int toupper(int c) { return (c >= 'a' && c <= 'z') ? c - 32 : c; }

/* ===== string.h ===== */
void *memmove(void *dest, const void *src, size_t n) {
  uint8_t *d = (uint8_t *)dest;
  const uint8_t *s = (const uint8_t *)src;
  if (d < s) {
    for (size_t i = 0; i < n; i++) d[i] = s[i];
  } else if (d > s) {
    for (size_t i = n; i > 0; i--) d[i - 1] = s[i - 1];
  }
  return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const uint8_t *p1 = (const uint8_t *)s1;
  const uint8_t *p2 = (const uint8_t *)s2;
  for (size_t i = 0; i < n; i++) {
    if (p1[i] != p2[i]) return (int)p1[i] - (int)p2[i];
  }
  return 0;
}

void *memchr(const void *s, int c, size_t n) {
  const uint8_t *p = (const uint8_t *)s;
  for (size_t i = 0; i < n; i++) {
    if (p[i] == (uint8_t)c) return (void *)(p + i);
  }
  return NULL;
}

size_t strnlen(const char *s, size_t maxlen) {
  size_t n = 0;
  while (n < maxlen && s[n]) n++;
  return n;
}

char *stpcpy(char *dest, const char *src) {
  while (*src) *dest++ = *src++;
  *dest = '\0';
  return dest;
}

char *strncpy(char *dest, const char *src, size_t n) {
  size_t i;
  for (i = 0; i < n && src[i]; i++) dest[i] = src[i];
  for (; i < n; i++) dest[i] = '\0';
  return dest;
}

char *strcat(char *dest, const char *src) {
  char *d = dest;
  while (*d) d++;
  while (*src) *d++ = *src++;
  *d = '\0';
  return dest;
}

char *strchr(const char *s, int c) {
  while (*s) {
    if (*s == (char)c) return (char *)s;
    s++;
  }
  if (c == '\0') return (char *)s;
  return NULL;
}

char *strrchr(const char *s, int c) {
  const char *last = NULL;
  while (*s) {
    if (*s == (char)c) last = s;
    s++;
  }
  if (c == '\0') return (char *)s;
  return (char *)last;
}

char *strstr(const char *haystack, const char *needle) {
  if (!*needle) return (char *)haystack;
  while (*haystack) {
    const char *h = haystack;
    const char *n = needle;
    while (*h && *n && *h == *n) { h++; n++; }
    if (!*n) return (char *)haystack;
    haystack++;
  }
  return NULL;
}

char *strpbrk(const char *s, const char *accept) {
  while (*s) {
    const char *a = accept;
    while (*a) {
      if (*s == *a) return (char *)s;
      a++;
    }
    s++;
  }
  return NULL;
}

char *strsep(char **stringp, const char *delim) {
  char *start = *stringp;
  if (!start) return NULL;
  char *end = start;
  while (*end) {
    const char *d = delim;
    while (*d) {
      if (*end == *d) {
        *end = '\0';
        *stringp = end + 1;
        return start;
      }
      d++;
    }
    end++;
  }
  *stringp = NULL;
  return start;
}

size_t strspn(const char *s, const char *accept) {
  size_t count = 0;
  while (s[count]) {
    int found = 0;
    for (const char *a = accept; *a; a++) {
      if (s[count] == *a) { found = 1; break; }
    }
    if (!found) break;
    count++;
  }
  return count;
}

size_t strcspn(const char *s, const char *reject) {
  size_t count = 0;
  while (s[count]) {
    for (const char *r = reject; *r; r++) {
      if (s[count] == *r) return count;
    }
    count++;
  }
  return count;
}

int strcasecmp(const char *s1, const char *s2) {
  while (*s1 && tolower(*s1) == tolower(*s2)) { s1++; s2++; }
  return tolower(*s1) - tolower(*s2);
}

int strncmp(const char *s1, const char *s2, size_t n) {
  for (size_t i = 0; i < n; i++) {
    if (s1[i] != s2[i]) return (int)s1[i] - (int)s2[i];
    if (!s1[i]) return 0;
  }
  return 0;
}

int strncasecmp(const char *s1, const char *s2, size_t n) {
  for (size_t i = 0; i < n; i++) {
    if (!s1[i] && !s2[i]) return 0;
    int c1 = tolower(s1[i]);
    int c2 = tolower(s2[i]);
    if (c1 != c2) return c1 - c2;
    if (!s1[i]) return 0;
  }
  return 0;
}

int strcoll(const char *s1, const char *s2) {
  return strcmp(s1, s2);
}

char *strerror(int errnum) {
  switch (errnum) {
    case EDOM: return "Domain error";
    case ERANGE: return "Range error";
    case EINVAL: return "Invalid argument";
    case ENOENT: return "No such file or directory";
    case EACCES: return "Permission denied";
    case ENOMEM: return "Out of memory";
    default: return "Unknown error";
  }
}

/* ===== stdio.h ===== */
static FILE __stdin = { 0, 0, 0, _IONBF, NULL, 0, 0, 0, -1, 0 };
static FILE __stdout = { 1, 0, 0, _IONBF, NULL, 0, 0, 0, -1, 0 };
static FILE __stderr = { 2, 0, 0, _IONBF, NULL, 0, 0, 0, -1, 0 };

FILE *stdin = &__stdin;
FILE *stdout = &__stdout;
FILE *stderr = &__stderr;

static FILE __files[FOPEN_MAX];

static int __filemode_to_flags(const char *mode) {
  int flags = 0;
  if (mode[0] == 'r') {
    flags = 0;
    if (strchr(mode, '+')) flags = 2;
  } else if (mode[0] == 'w') {
    flags = 1;
    if (strchr(mode, '+')) flags = 2;
  } else if (mode[0] == 'a') {
    flags = 1;
    if (strchr(mode, '+')) flags = 2;
  }
  return flags;
}

FILE *fopen(const char *path, const char *mode) {
  int fd = open(path, __filemode_to_flags(mode));
  if (fd < 0) return NULL;
  for (int i = 0; i < FOPEN_MAX; i++) {
    if (__files[i].fd == 0 && !__files[i].flags) {
      __files[i].fd = fd;
      __files[i].eof = 0;
      __files[i].error = 0;
      __files[i].bufmode = _IONBF;
      __files[i].buf = NULL;
      __files[i].bufsize = 0;
      __files[i].bufpos = 0;
      __files[i].buflen = 0;
      __files[i].ungetc = -1;
      __files[i].flags = 1;
      return &__files[i];
    }
  }
  return NULL;
}

FILE *freopen(const char *path, const char *mode, FILE *stream) {
  if (stream->fd > 2) close(stream->fd);
  if (path) {
    int fd = open(path, __filemode_to_flags(mode));
    if (fd < 0) return NULL;
    stream->fd = fd;
  }
  stream->eof = 0;
  stream->error = 0;
  stream->ungetc = -1;
  return stream;
}

int fclose(FILE *stream) {
  if (stream->fd > 2) {
    close(stream->fd);
  }
  stream->fd = 0;
  stream->flags = 0;
  return 0;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
  size_t total = size * nmemb;
  size_t count = 0;
  uint8_t *p = (uint8_t *)ptr;

  if (stream->ungetc >= 0) {
    p[0] = (uint8_t)stream->ungetc;
    stream->ungetc = -1;
    count = 1;
    if (count >= total) return count / size;
  }

  ssize_t ret = read(stream->fd, p + count, total - count);
  if (ret < 0) {
    stream->error = 1;
    return count / size;
  }
  count += (size_t)ret;
  if (ret == 0) stream->eof = 1;
  return count / size;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
  size_t total = size * nmemb;
  ssize_t ret = write(stream->fd, ptr, total);
  if (ret < 0) {
    stream->error = 1;
    return 0;
  }
  return (size_t)ret / size;
}

int fflush(FILE *stream) {
  (void)stream;
  return 0;
}

int fseek(FILE *stream, long offset, int whence) {
  (void)stream;
  (void)offset;
  (void)whence;
  return -1;
}

long ftell(FILE *stream) {
  (void)stream;
  return -1;
}

int feof(FILE *stream) {
  return stream->eof;
}

int ferror(FILE *stream) {
  return stream->error;
}

void clearerr(FILE *stream) {
  stream->eof = 0;
  stream->error = 0;
}

int fputs(const char *s, FILE *stream) {
  size_t len = strlen(s);
  if (fwrite(s, 1, len, stream) == len) return 0;
  return EOF;
}

char *fgets(char *s, int size, FILE *stream) {
  int i = 0;
  while (i < size - 1) {
    int c = getc(stream);
    if (c == EOF) {
      if (i == 0) return NULL;
      break;
    }
    s[i++] = (char)c;
    if (c == '\n') break;
  }
  s[i] = '\0';
  return s;
}

int getc(FILE *stream) {
  if (stream->ungetc >= 0) {
    int c = stream->ungetc;
    stream->ungetc = -1;
    return c;
  }
  uint8_t ch;
  ssize_t ret = read(stream->fd, &ch, 1);
  if (ret <= 0) {
    if (ret == 0) stream->eof = 1;
    return EOF;
  }
  return ch;
}

int ungetc(int c, FILE *stream) {
  if (stream->ungetc >= 0) return EOF;
  stream->ungetc = c;
  stream->eof = 0;
  return c;
}

int setvbuf(FILE *stream, char *buf, int mode, size_t size) {
  stream->buf = buf;
  stream->bufmode = mode;
  stream->bufsize = size;
  return 0;
}

FILE *tmpfile(void) { return NULL; }
char *tmpnam(char *s) {
  static char name[] = "/tmpXXXXXX";
  if (s) { strcpy(s, name); return s; }
  return name;
}

/* ===== printf family ===== */
static void __print_char(char **str, size_t *pos, size_t max, char c) {
  if (str && *str && *pos < max) {
    (*str)[(*pos)++] = c;
  }
}

static void __print_str(char **str, size_t *pos, size_t max, const char *s) {
  while (*s) __print_char(str, pos, max, *s++);
}

static void __print_int(char **str, size_t *pos, size_t max, int val, int base, int uppercase) {
  char buf[32];
  int i = 0;
  unsigned int uval;
  if (val < 0 && base == 10) {
    __print_char(str, pos, max, '-');
    uval = (unsigned int)(-val);
  } else {
    uval = (unsigned int)val;
  }
  if (uval == 0) {
    buf[i++] = '0';
  } else {
    while (uval > 0) {
      int d = uval % base;
      buf[i++] = (d < 10) ? ('0' + d) : (uppercase ? 'A' : 'a') + d - 10;
      uval /= base;
    }
  }
  while (i > 0) __print_char(str, pos, max, buf[--i]);
}

static void __print_unsigned(char **str, size_t *pos, size_t max, unsigned long val, int base, int uppercase) {
  char buf[64];
  int i = 0;
  if (val == 0) {
    buf[i++] = '0';
  } else {
    while (val > 0) {
      int d = val % base;
      buf[i++] = (d < 10) ? ('0' + d) : (uppercase ? 'A' : 'a') + d - 10;
      val /= base;
    }
  }
  while (i > 0) __print_char(str, pos, max, buf[--i]);
}

static void __print_double_f(char **str, size_t *pos, size_t max, double val, int precision) {
  if (precision < 0) precision = 6;
  size_t start = *pos;
  if (val < 0) {
    __print_char(str, pos, max, '-');
    val = -val;
  }
  unsigned long long int_part = (unsigned long long)val;
  __print_unsigned(str, pos, max, int_part, 10, 0);
  if (precision > 0) {
    __print_char(str, pos, max, '.');
    double frac = val - (double)int_part;
    for (int i = 0; i < precision; i++) {
      frac *= 10.0;
      int d = (int)frac;
      __print_char(str, pos, max, '0' + d);
      frac -= d;
    }
  }
}

static void __print_double_g(char **str, size_t *pos, size_t max, double val, int precision) {
  if (precision < 0) precision = 6;
  size_t start = *pos;
  if (val < 0) {
    __print_char(str, pos, max, '-');
    start = *pos;
    val = -val;
  }
  unsigned long long int_part = (unsigned long long)val;
  __print_unsigned(str, pos, max, int_part, 10, 0);
  double frac = val - (double)int_part;
  size_t before_frac = *pos;
  size_t dot_pos = (size_t)-1;
  int any_nonzero = 0;
  if (precision > 0) {
    __print_char(str, pos, max, '.');
    dot_pos = before_frac;
    for (int i = 0; i < precision; i++) {
      frac *= 10.0;
      int d = (int)frac;
      __print_char(str, pos, max, '0' + d);
      if (d != 0) any_nonzero = 1;
      frac -= d;
    }
  }
  if (!any_nonzero && dot_pos != (size_t)-1) {
    *pos = dot_pos;
  } else if (any_nonzero) {
    char *base = str && *str ? *str : NULL;
    size_t end = *pos;
    while (end > dot_pos + 1 && base && base[end - 1] == '0')
      end--;
    *pos = end;
  }
}

static int __vfprintf(FILE *stream, const char *format, va_list ap) {
  char buf[4096];
  char *buf_ptr = buf;
  size_t pos = 0;
  while (*format) {
    if (*format != '%') {
      buf[pos++] = *format++;
      if (pos >= sizeof(buf) - 8) {
        fwrite(buf, 1, pos, stream);
        pos = 0;
      }
      continue;
    }
    format++;
    int width = 0;
    int precision = -1;
    int long_flag = 0;

    if (*format == '-') { format++; }
    if (*format == '0') { format++; }
    while (*format >= '0' && *format <= '9') { width = width * 10 + (*format - '0'); format++; }
    if (*format == '.') { format++; precision = 0; while (*format >= '0' && *format <= '9') { precision = precision * 10 + (*format - '0'); format++; } }
    if (*format == 'l') { long_flag = 1; format++; }
    if (*format == 'z') { format++; }

    switch (*format) {
      case 'd': case 'i': {
        int val = va_arg(ap, int);
        __print_int(&buf_ptr, &pos, sizeof(buf), val, 10, 0);
        break;
      }
      case 'u': {
        unsigned long val = long_flag ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int);
        __print_unsigned(&buf_ptr, &pos, sizeof(buf), val, 10, 0);
        break;
      }
      case 'x': {
        unsigned long val = long_flag ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int);
        __print_unsigned(&buf_ptr, &pos, sizeof(buf), val, 16, 0);
        break;
      }
      case 'X': {
        unsigned long val = long_flag ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int);
        __print_unsigned(&buf_ptr, &pos, sizeof(buf), val, 16, 1);
        break;
      }
      case 'p': {
        unsigned long val = (unsigned long)va_arg(ap, void *);
        __print_str(&buf_ptr, &pos, sizeof(buf), "0x");
        __print_unsigned(&buf_ptr, &pos, sizeof(buf), val, 16, 0);
        break;
      }
      case 's': {
        const char *s = va_arg(ap, const char *);
        if (!s) s = "(null)";
        __print_str(&buf_ptr, &pos, sizeof(buf), s);
        break;
      }
      case 'c': {
        char c = (char)va_arg(ap, int);
        __print_char(&buf_ptr, &pos, sizeof(buf), c);
        break;
      }
      case 'f': {
        double val = va_arg(ap, double);
        __print_double_f(&buf_ptr, &pos, sizeof(buf), val, precision);
        break;
      }
      case 'g': case 'G': {
        double val = va_arg(ap, double);
        __print_double_g(&buf_ptr, &pos, sizeof(buf), val, precision);
        break;
      }
      case '%': {
        __print_char(&buf_ptr, &pos, sizeof(buf), '%');
        break;
      }
      default:
        break;
    }
    format++;
  }
  if (pos > 0) fwrite(buf, 1, pos, stream);
  return 0;
}

int fprintf(FILE *stream, const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  int ret = __vfprintf(stream, format, ap);
  va_end(ap);
  return ret;
}

int printf(const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  int ret = __vfprintf(stdout, format, ap);
  va_end(ap);
  return ret;
}

int vsnprintf(char *str, size_t size, const char *format, va_list ap) {
  size_t pos = 0;
  while (*format) {
    if (*format != '%') {
      __print_char(&str, &pos, size, *format++);
      continue;
    }
    format++;
    int width = 0;
    int precision = -1;
    int long_flag = 0;

    if (*format == '-') { format++; }
    if (*format == '0') { format++; }
    while (*format >= '0' && *format <= '9') { width = width * 10 + (*format - '0'); format++; }
    if (*format == '.') { format++; precision = 0; while (*format >= '0' && *format <= '9') { precision = precision * 10 + (*format - '0'); format++; } }
    if (*format == 'l') { long_flag = 1; format++; }
    if (*format == 'z') { format++; }

    switch (*format) {
      case 'd': case 'i': {
        int val = long_flag ? va_arg(ap, long) : va_arg(ap, int);
        __print_int(&str, &pos, size, val, 10, 0);
        break;
      }
      case 'u': {
        unsigned long val = long_flag ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int);
        __print_unsigned(&str, &pos, size, val, 10, 0);
        break;
      }
      case 'x': {
        unsigned long val = long_flag ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int);
        __print_unsigned(&str, &pos, size, val, 16, 0);
        break;
      }
      case 'X': {
        unsigned long val = long_flag ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int);
        __print_unsigned(&str, &pos, size, val, 16, 1);
        break;
      }
      case 'p': {
        unsigned long val = (unsigned long)va_arg(ap, void *);
        __print_str(&str, &pos, size, "0x");
        __print_unsigned(&str, &pos, size, val, 16, 0);
        break;
      }
      case 's': {
        const char *s = va_arg(ap, const char *);
        if (!s) s = "(null)";
        __print_str(&str, &pos, size, s);
        break;
      }
      case 'c': {
        char c = (char)va_arg(ap, int);
        __print_char(&str, &pos, size, c);
        break;
      }
      case 'f': {
        double val = va_arg(ap, double);
        __print_double_f(&str, &pos, size, val, precision);
        break;
      }
      case 'e': case 'E':
      case 'g': case 'G': {
        double val = va_arg(ap, double);
        __print_double_g(&str, &pos, size, val, precision);
        break;
      }
      case '%': {
        __print_char(&str, &pos, size, '%');
        break;
      }
      default:
        break;
    }
    format++;
  }
  if (str && pos < size) str[pos] = '\0';
  else if (str && size > 0) str[size - 1] = '\0';
  return (int)pos;
}

int snprintf(char *str, size_t size, const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  int ret = vsnprintf(str, size, format, ap);
  va_end(ap);
  return ret;
}

int sprintf(char *str, const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  int ret = vsnprintf(str, (size_t)-1, format, ap);
  va_end(ap);
  return ret;
}

int vfprintf(FILE *stream, const char *format, va_list ap) {
  return __vfprintf(stream, format, ap);
}

/* ===== stdlib.h ===== */
void abort(void) {
  exit(1);
  while (1) { asm volatile("hlt"); }
}

int system(const char *command) {
  int ret;
  asm volatile("int $0x80" : "=a"(ret) : "0"(__NR_exec), "b"(command));
  return ret < 0 ? -1 : 0;
}

char *getenv(const char *name) {
  (void)name;
  return NULL;
}

int atoi(const char *nptr) {
  return (int)strtol(nptr, NULL, 10);
}

long atol(const char *nptr) {
  return strtol(nptr, NULL, 10);
}

double atof(const char *nptr) {
  return strtod(nptr, NULL);
}

long int strtol(const char *nptr, char **endptr, int base) {
  long result = 0;
  int sign = 1;
  while (isspace(*nptr)) nptr++;
  if (*nptr == '-') { sign = -1; nptr++; }
  else if (*nptr == '+') nptr++;
  if (base == 0) {
    if (*nptr == '0') { nptr++; base = (*nptr == 'x' || *nptr == 'X') ? 16 : 8; if (base == 16) nptr++; }
    else base = 10;
  } else if (base == 16) {
    if (*nptr == '0' && (nptr[1] == 'x' || nptr[1] == 'X')) nptr += 2;
  }
  while (*nptr) {
    int digit;
    if (*nptr >= '0' && *nptr <= '9') digit = *nptr - '0';
    else if (*nptr >= 'a' && *nptr <= 'f') digit = *nptr - 'a' + 10;
    else if (*nptr >= 'A' && *nptr <= 'F') digit = *nptr - 'A' + 10;
    else break;
    if (digit >= base) break;
    result = result * base + digit;
    nptr++;
  }
  if (endptr) *endptr = (char *)nptr;
  return result * sign;
}

unsigned long int strtoul(const char *nptr, char **endptr, int base) {
  unsigned long result = 0;
  while (isspace(*nptr)) nptr++;
  if (*nptr == '+') nptr++;
  if (base == 0) {
    if (*nptr == '0') { nptr++; base = (*nptr == 'x' || *nptr == 'X') ? 16 : 8; if (base == 16) nptr++; }
    else base = 10;
  } else if (base == 16) {
    if (*nptr == '0' && (nptr[1] == 'x' || nptr[1] == 'X')) nptr += 2;
  }
  while (*nptr) {
    int digit;
    if (*nptr >= '0' && *nptr <= '9') digit = *nptr - '0';
    else if (*nptr >= 'a' && *nptr <= 'f') digit = *nptr - 'a' + 10;
    else if (*nptr >= 'A' && *nptr <= 'F') digit = *nptr - 'A' + 10;
    else break;
    if (digit >= base) break;
    result = result * base + digit;
    nptr++;
  }
  if (endptr) *endptr = (char *)nptr;
  return result;
}

double strtod(const char *nptr, char **endptr) {
  double result = 0.0;
  int sign = 1;
  while (isspace(*nptr)) nptr++;
  if (*nptr == '-') { sign = -1; nptr++; }
  else if (*nptr == '+') nptr++;

  while (isdigit(*nptr)) {
    result = result * 10.0 + (*nptr - '0');
    nptr++;
  }
  if (*nptr == '.') {
    nptr++;
    double frac = 0.0;
    double div = 10.0;
    while (isdigit(*nptr)) {
      frac += (*nptr - '0') / div;
      div *= 10.0;
      nptr++;
    }
    result += frac;
  }
  if (*nptr == 'e' || *nptr == 'E') {
    nptr++;
    int exp_sign = 1;
    if (*nptr == '-') { exp_sign = -1; nptr++; }
    else if (*nptr == '+') nptr++;
    int exp = 0;
    while (isdigit(*nptr)) {
      exp = exp * 10 + (*nptr - '0');
      nptr++;
    }
    exp *= exp_sign;
    result *= __builtin_pow(10.0, (double)exp);
  }
  result *= sign;
  if (endptr) *endptr = (char *)nptr;
  return result;
}

int remove(const char *path) {
  (void)path;
  return -1;
}

int rename(const char *oldPath, const char *newPath) {
  (void)oldPath;
  (void)newPath;
  return -1;
}

typedef struct {
  char *base;
  size_t nmemb;
  size_t size;
  int (*compar)(const void *, const void *);
} qsort_ctx;

static void qsort_swap(char *a, char *b, size_t size) {
  char tmp;
  for (size_t i = 0; i < size; i++) {
    tmp = a[i];
    a[i] = b[i];
    b[i] = tmp;
  }
}

static void qsort_inner(char *base, size_t lo, size_t hi, size_t size, int (*compar)(const void *, const void *)) {
  if (lo >= hi) return;
  size_t mid = (lo + hi) / 2;
  qsort_swap(base + lo * size, base + mid * size, size);
  size_t last = lo;
  for (size_t i = lo + 1; i <= hi; i++) {
    if (compar(base + i * size, base + lo * size) < 0) {
      last++;
      qsort_swap(base + last * size, base + i * size, size);
    }
  }
  qsort_swap(base + lo * size, base + last * size, size);
  if (last > 0) qsort_inner(base, lo, last - 1, size, compar);
  qsort_inner(base, last + 1, hi, size, compar);
}

void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *)) {
  if (nmemb <= 1) return;
  qsort_inner((char *)base, 0, nmemb - 1, size, compar);
}

/* ===== math.h ===== */
double acos(double x) { return __builtin_acos(x); }
double asin(double x) { return __builtin_asin(x); }
double atan2(double y, double x) { return __builtin_atan2(y, x); }
double cos(double x) { return __builtin_cos(x); }
double cosh(double x) { return __builtin_cosh(x); }
double sin(double x) { return __builtin_sin(x); }
double sinh(double x) { return __builtin_sinh(x); }
double tan(double x) { return __builtin_tan(x); }
double tanh(double x) { return __builtin_tanh(x); }
double exp(double x) { return __builtin_exp(x); }
double frexp(double x, int *exp) {
  double sig;
  if (x == 0.0 || __builtin_isinf(x) || __builtin_isnan(x)) {
    *exp = 0;
    return x;
  }
  __asm__ __volatile__(
    "fldl %2\n"
    "fxtract\n"
    "fstpl %0\n"
    "fistpl %1\n"
    : "=m"(sig), "=m"(*exp)
    : "m"(x)
    : "st", "st(1)"
  );
  sig = sig / 2.0;
  *exp = *exp + 1;
  return sig;
}
double ldexp(double x, int exp) {
  double result;
  __asm__ __volatile__(
    "fildl %2\n"
    "fldl %1\n"
    "fscale\n"
    "fstpl %0\n"
    "fstp %%st(0)\n"
    : "=m"(result)
    : "m"(x), "m"(exp)
    : "st", "st(1)"
  );
  return result;
}
double log(double x) { return __builtin_log(x); }
double log2(double x) { return __builtin_log2(x); }
double log10(double x) { return __builtin_log10(x); }
double pow(double x, double y) { return __builtin_pow(x, y); }
double sqrt(double x) {
  double result;
  __asm__ __volatile__("fldl %1\n fsqrt\n fstpl %0" : "=m"(result) : "m"(x) : "st");
  return result;
}
double ceil(double x) {
  unsigned short cw, cw_new;
  double result;
  __asm__ __volatile__("fnstcw %0" : "=m"(cw));
  cw_new = (cw & ~0x0C00) | 0x0800;
  __asm__ __volatile__("fldcw %0" : : "m"(cw_new));
  __asm__ __volatile__("fldl %1\n frndint\n fstpl %0" : "=m"(result) : "m"(x) : "st");
  __asm__ __volatile__("fldcw %0" : : "m"(cw));
  return result;
}
double fabs(double x) {
  double result;
  __asm__ __volatile__("fldl %1\n fabs\n fstpl %0" : "=m"(result) : "m"(x) : "st");
  return result;
}
double floor(double x) {
  unsigned short cw, cw_new;
  double result;
  __asm__ __volatile__("fnstcw %0" : "=m"(cw));
  cw_new = (cw & ~0x0C00) | 0x0400;
  __asm__ __volatile__("fldcw %0" : : "m"(cw_new));
  __asm__ __volatile__("fldl %1\n frndint\n fstpl %0" : "=m"(result) : "m"(x) : "st");
  __asm__ __volatile__("fldcw %0" : : "m"(cw));
  return result;
}
double fmod(double x, double y) {
  double result;
  __asm__ __volatile__(
    "fldl %2\n"
    "fldl %1\n"
    "1: fprem\n"
    "fstsw %%ax\n"
    "sahf\n"
    "jp 1b\n"
    "fstpl %0\n"
    "fstp %%st(0)\n"
    : "=m"(result)
    : "m"(x), "m"(y)
    : "st", "st(1)", "ax"
  );
  return result;
}

int fpclassify(double x) { return __builtin_fpclassify(FP_NAN, FP_INFINITE, FP_NORMAL, FP_SUBNORMAL, FP_ZERO, x); }
int isfinite(double x) { return __builtin_isfinite(x); }
int isinf(double x) { return __builtin_isinf(x); }
int isnan(double x) { return __builtin_isnan(x); }
int isnormal(double x) { return __builtin_isnormal(x); }

int ifloor(double x) { return (int)floor(x); }
int iceil(double x) { return (int)ceil(x); }

/* ===== time.h ===== */
time_t time(time_t *tloc) {
  time_t t = 0;
  if (tloc) *tloc = t;
  return t;
}

clock_t clock(void) { return (clock_t)-1; }

double difftime(time_t t1, time_t t2) { return (double)(t1 - t2); }

struct tm *gmtime(const time_t *t) {
  (void)t;
  static struct tm tm;
  tm.tm_sec = 0;
  tm.tm_min = 0;
  tm.tm_hour = 0;
  tm.tm_mday = 1;
  tm.tm_mon = 0;
  tm.tm_year = 70;
  tm.tm_wday = 4;
  tm.tm_yday = 0;
  tm.tm_isdst = 0;
  return &tm;
}

struct tm *localtime(const time_t *time) {
  return gmtime(time);
}

time_t mktime(struct tm *tm) {
  (void)tm;
  return 0;
}

size_t strftime(char *s, size_t max, const char *format, const struct tm *tm) {
  size_t pos = 0;
  while (*format && pos < max - 1) {
    if (*format != '%') {
      s[pos++] = *format++;
      continue;
    }
    format++;
    switch (*format) {
      case 'Y': {
        int y = tm->tm_year + 1900;
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", y);
        for (char *p = buf; *p && pos < max - 1; p++) s[pos++] = *p;
        break;
      }
      case 'm': s[pos++] = '0' + (tm->tm_mon + 1) / 10; if (pos < max - 1) s[pos++] = '0' + (tm->tm_mon + 1) % 10; break;
      case 'd': s[pos++] = '0' + tm->tm_mday / 10; if (pos < max - 1) s[pos++] = '0' + tm->tm_mday % 10; break;
      case 'H': s[pos++] = '0' + tm->tm_hour / 10; if (pos < max - 1) s[pos++] = '0' + tm->tm_hour % 10; break;
      case 'M': s[pos++] = '0' + tm->tm_min / 10; if (pos < max - 1) s[pos++] = '0' + tm->tm_min % 10; break;
      case 'S': s[pos++] = '0' + tm->tm_sec / 10; if (pos < max - 1) s[pos++] = '0' + tm->tm_sec % 10; break;
      case 's': {
        char buf[16];
        snprintf(buf, sizeof(buf), "0");
        for (char *p = buf; *p && pos < max - 1; p++) s[pos++] = *p;
        break;
      }
      default: s[pos++] = *format; break;
    }
    format++;
  }
  s[pos] = '\0';
  return pos;
}

/* ===== locale.h ===== */
static struct lconv __locale = {
  (char *)".",
  (char *)"",
  (char *)"",
  (char *)"",
  (char *)"",
  (char *)"",
  (char *)"",
  (char *)"",
  (char *)"",
  (char *)"",
  0, 0, 0, 0, 0, 0, 0, 0
};

char *setlocale(int category, const char *locale) {
  (void)category;
  (void)locale;
  return (char *)"C";
}

struct lconv *localeconv(void) {
  return &__locale;
}

/* ===== signal.h ===== */
void (*signal(int sig, void (*func)(int)))(int) {
  (void)sig;
  (void)func;
  return SIG_DFL;
}

int raise(int sig) {
  (void)sig;
  return -1;
}

/* ===== setjmp.h ===== */
int setjmp(jmp_buf env) {
  int ret;
  asm volatile(
    "movl %%ebx, (%0)\n"
    "movl %%esi, 4(%0)\n"
    "movl %%edi, 8(%0)\n"
    "movl %%ebp, 12(%0)\n"
    "movl %%esp, 16(%0)\n"
    "movl (%%esp), %%eax\n"
    "movl %%eax, 20(%0)\n"
    "xorl %%eax, %%eax\n"
    : "=r"(ret) : "0"(env) : "memory"
  );
  return 0;
}

void longjmp(jmp_buf env, int val) {
  asm volatile(
    "movl (%0), %%ebx\n"
    "movl 4(%0), %%esi\n"
    "movl 8(%0), %%edi\n"
    "movl 12(%0), %%ebp\n"
    "movl 16(%0), %%esp\n"
    "movl %1, %%eax\n"
    "jmp *20(%0)\n"
    :
    : "r"(env), "r"(val)
    : "memory"
  );
}

/* ===== assert.h ===== */
void __assert_fail(const char *expr, const char *file, int line) {
  fprintf(stderr, "Assertion failed: %s, file %s, line %d\n", expr, file, line);
  abort();
}

/* ===== compiler-rt (64-bit ops for 32-bit target) ===== */
typedef unsigned long long du_int;
typedef long long di_int;
typedef unsigned int su_int;

static du_int __udivmoddi4(du_int a, du_int b, du_int *rem) {
  du_int d = 0;
  du_int q = 0;
  du_int r = 0;
  unsigned int shift;
  for (shift = 0; shift < 64; shift++) {
    r = (r << 1) | (a >> 63);
    a = a << 1;
    q = q << 1;
    if (r >= b) {
      r -= b;
      q |= 1;
    }
  }
  if (rem) *rem = r;
  return q;
}

du_int __udivdi3(du_int a, du_int b) {
  return __udivmoddi4(a, b, NULL);
}

du_int __umoddi3(du_int a, du_int b) {
  du_int rem;
  __udivmoddi4(a, b, &rem);
  return rem;
}

di_int __divdi3(di_int a, di_int b) {
  int neg = 0;
  du_int ua, ub;
  if (a < 0) { ua = -(du_int)a; neg = 1; }
  else { ua = a; }
  if (b < 0) { ub = -(du_int)b; neg ^= 1; }
  else { ub = b; }
  du_int q = __udivmoddi4(ua, ub, NULL);
  return neg ? -(di_int)q : (di_int)q;
}

di_int __moddi3(di_int a, di_int b) {
  int neg = 0;
  du_int ua, ub, rem;
  if (a < 0) { ua = -(du_int)a; neg = 1; }
  else { ua = a; }
  if (b < 0) { ub = -(du_int)b; }
  else { ub = b; }
  __udivmoddi4(ua, ub, &rem);
  return neg ? -(di_int)rem : (di_int)rem;
}
