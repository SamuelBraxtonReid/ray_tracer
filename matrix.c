#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "matrix.h"

void cross(double *out, double *in0, double *in1)
{
  double v[3];
  v[0] = in0[1] * in1[2] - in0[2] * in1[1];
  v[1] = in0[2] * in1[0] - in0[0] * in1[2]; 
  v[2] = in0[0] * in1[1] - in0[1] * in1[0];
  out[0] = v[0];
  out[1] = v[1];
  out[2] = v[2];
}

double safe_magnitude(double *vec) 
{
  double m = vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2];
  if (m == 0.0f || m == INFINITY) {
    double maximum = fabs(vec[0]);
    for (int32_t i = 1; i < 3; i++) {
      double v = fabs(vec[i]);
      if (v > maximum) {
        maximum = v;
      }
    }
    if (maximum == 0.0f || maximum == INFINITY) {
      return m;
    }
    double scaled[] = {vec[0] / maximum, vec[1] / maximum, vec[2] / maximum};
    double ms = sqrt(scaled[0] * scaled[0] + scaled[1] * scaled[1] + scaled[2] * scaled[2]);
    return ms * maximum;
  }
  return sqrt(m);
}

void safe_normalize(double *vec) {
  double m = safe_magnitude(vec);
  if (m) {
    vec[0] /= m;
    vec[1] /= m;
    vec[2] /= m;
    return;
  }
  for (;;) {
    vec[0] = random() * 2.0f / RAND_MAX - 1.0f;
    vec[1] = random() * 2.0f / RAND_MAX - 1.0f;
    vec[2] = random() * 2.0f / RAND_MAX - 1.0f;
    m = safe_magnitude(vec);
    if (m && m <= 1.0f) {
      vec[0] /= m;
      vec[1] /= m;
      vec[2] /= m;
      break;
    }
  }
}


// use 'pos', 'look_at', and 'above' to generate basis vectors for the camera ('forward', 'right', and 'up' (with origin at 'pos'))
void basis(double *pos, double *above, double *look_at, double *right, double *up, double *forward)
{
  int32_t u = 0;
  int32_t f = 0;
  for (int32_t i = 0; i < 3; i++) {
    up[i] = above[i] - pos[i];
    u |= (up[i] != 0.0f);
    forward[i] = look_at[i] - pos[i];
    f |= (forward[i] != 0.0f);
  }
  if (!u) up[1] = 1.0f;
  if (!f) forward[2] = -1.0f;
  double mu = safe_magnitude(up);
  double mf = safe_magnitude(forward);
  for (int32_t i = 0; i < 3; i++) {
    up[i] /= mu;
    forward[i] /= mf;
  }
  printf("up: %f %f %f\n", up[0], up[1], up[2]);
  printf("forward: %f %f %f\n", forward[0], forward[1], forward[2]);
  for (;;) {
    cross(right, forward, up);
    printf("right: %f %f %f\n", right[0], right[1], right[2]);
    if (right[0] || right[1] || right[2]) break;
    for (;;) {
      up[0] = random() * 2.0f / RAND_MAX - 1.0f;
      up[1] = random() * 2.0f / RAND_MAX - 1.0f;
      up[2] = random() * 2.0f / RAND_MAX - 1.0f;
      double m = safe_magnitude(up);
      if (m && m <= 1.0f) {
        up[0] /= m;
        up[1] /= m;
        up[2] /= m;
        break;
      }
    }
  }
  for (int32_t i = 0; i < 3; i++) {
    safe_normalize(right);
    cross(up, right, forward);
    safe_normalize(up);
    cross(right, forward, up);
  }
  printf("\n");
}

void transform(struct object *object, struct affine *affine)
{
  if (object->type == 0) {
    struct sphere *s = &(object->contents.sphere);
    double input[3] = {s->center[0], s->center[1], s->center[2]};
    for (int32_t i = 0; i < 3; i++) {
      s->center[i] = affine->elements[i][3];
      for (int32_t j = 0; j < 3; j++) {
        s->center[i] += affine->elements[i][j] * input[j];
      }
    }
    // replace this with something better
    //double average_scale = (affine->elements[0][0] + affine->elements[1][1] + affine->elements[2][2]) / 3.0f;
    //s->radius *= average_scale;
  } else if (object->type == 1) {
    struct triangle *t = &(object->contents.triangle);
    for (int32_t i = 0; i < 3; i++) {
      double input[3] = {t->points[i][0], t->points[i][1], t->points[i][2]};
      for (int32_t j = 0; j < 3; j++) {
        t->points[i][j] = affine->elements[j][3];
        for (int32_t k = 0; k < 3; k++) {
          t->points[i][j] += affine->elements[j][k] * input[k];
        }
      }
    }
    if (affine->winding) {
      for (int32_t i = 0; i < 3; i++) {
        double temp = t->points[0][i];
        t->points[0][i] = t->points[1][i];
        t->points[1][i] = temp;
      }
    }
  }
}

void invert(double *m, double *mi)
{
  double det = 0.0f;
  det += m[4] * (m[0] * m[8] - m[2] * m[6]);
  det += m[5] * (m[1] * m[6] - m[0] * m[7]); 
  det += m[3] * (m[2] * m[7] - m[1] * m[8]);
  mi[0] = (m[4] * m[8] - m[5] * m[7]) / det;
  mi[1] = (m[2] * m[7] - m[1] * m[8]) / det;
  mi[2] = (m[1] * m[5] - m[2] * m[4]) / det;
  mi[3] = (m[5] * m[6] - m[3] * m[8]) / det;
  mi[4] = (m[0] * m[8] - m[2] * m[6]) / det;
  mi[5] = (m[2] * m[3] - m[0] * m[5]) / det;
  mi[6] = (m[3] * m[7] - m[4] * m[6]) / det;
  mi[7] = (m[1] * m[6] - m[0] * m[7]) / det; 
  mi[8] = (m[0] * m[4] - m[1] * m[3]) / det;
}

void multiply(double *a, double *b, double *c)
{
  // column major
  for (int32_t i = 0; i < 9; i += 3) {
    for (int32_t j = 0; j < 3; j++) {
      c[i + j] = a[i] * b[j] + a[i + 1] * b[j + 3] + a[i + 2] * b[j + 6];
    }
  }

  /*
  // row major
  for (int32_t i = 0; i < 9; i += 3) {
    for (int32_t j = 0; j < 3; j++) {
      c[i + j] = a[j] * b[i] + a[j + 3] * b[i + 1] + a[j + 6] * b[i + 2];
    }
  }
  */
}

void print_m(double *m)
{
  for (int32_t i = 0; i < 9; i += 3) {
    for (int32_t j = 0; j < 3; j++) {
      printf("%f ", m[i + j]);
    }
    printf("\n");
  } 
}
