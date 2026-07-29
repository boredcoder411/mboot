#pragma once

#include <stddef.h>
#include <stdarg.h>

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2

#define BUFSIZ 4096
#define EOF (-1)

#define FILENAME_MAX 256
#define FOPEN_MAX 16
#define TMP_MAX 10000
#define L_tmpnam 20

typedef struct {
  int fd;
  int eof;
  int error;
  int bufmode;
  char *buf;
  int bufsize;
  int bufpos;
  int buflen;
  int ungetc;
  int flags;
} FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

FILE *fopen(const char *path, const char *mode);
FILE *freopen(const char *path, const char *mode, FILE *stream);
int fclose(FILE *stream);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int fflush(FILE *stream);
int fseek(FILE *stream, long offset, int whence);
long ftell(FILE *stream);
int feof(FILE *stream);
int ferror(FILE *stream);
void clearerr(FILE *stream);
int fputs(const char *s, FILE *stream);
char *fgets(char *s, int size, FILE *stream);
int fprintf(FILE *stream, const char *format, ...);
int printf(const char *format, ...);
int sprintf(char *str, const char *format, ...);
int snprintf(char *str, size_t size, const char *format, ...);
int getc(FILE *stream);
int ungetc(int c, FILE *stream);
int setvbuf(FILE *stream, char *buf, int mode, size_t size);
FILE *tmpfile(void);
char *tmpnam(char *s);
int vfprintf(FILE *stream, const char *format, va_list ap);
int vsnprintf(char *str, size_t size, const char *format, va_list ap);
