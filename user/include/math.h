#pragma once

#define HUGE_VAL ((double)1e308)

#define FP_NAN 0
#define FP_INFINITE 1
#define FP_ZERO 2
#define FP_NORMAL 3
#define FP_SUBNORMAL 4

double acos(double x);
double asin(double x);
double atan2(double y, double x);
double cos(double x);
double cosh(double x);
double sin(double x);
double sinh(double x);
double tan(double x);
double tanh(double x);
double exp(double x);
double frexp(double x, int *exp);
double ldexp(double x, int exp);
double log(double x);
double log2(double x);
double log10(double x);
double pow(double x, double y);
double sqrt(double x);
double ceil(double x);
double fabs(double x);
double floor(double x);
double fmod(double x, double y);

int fpclassify(double x);
int isfinite(double x);
int isinf(double x);
int isnan(double x);
int isnormal(double x);

int ifloor(double x);
int iceil(double x);
