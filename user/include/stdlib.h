#pragma once

#include <stddef.h>

#define RAND_MAX 32767
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

void *malloc(size_t size);
void *calloc(size_t num, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);

void exit(int status);
void abort(void);
int system(const char *command);
char *getenv(const char *name);

int rand(void);
void srand(unsigned int seed);
int abs(int n);

double atof(const char *nptr);
int atoi(const char *nptr);
long atol(const char *nptr);

double strtod(const char *nptr, char **endptr);
long int strtol(const char *nptr, char **endptr, int base);
unsigned long int strtoul(const char *nptr, char **endptr, int base);

int remove(const char *path);
int rename(const char *oldPath, const char *newPath);

void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *));
