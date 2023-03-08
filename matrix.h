#ifndef MATRIX_H
#define MATRIX_H

#include "main.h"

/*
void cross(double *out, double *a, double *b);

double dot(double *a, double *b);

void normalize(double *v);

void look_at(double *m, double *pos, double *forward, double *up);
*/

double safe_magnitude(double *vec);

void safe_normalize(double *vec);

void basis(double *pos, double *above, double *look_at, double *right, double *up, double *forward);

void transform(struct object *object, struct affine *affine);

void invert(double *m, double *mi);

void multiply(double *a, double *b, double *c);

void print_m(double *m);

#endif
