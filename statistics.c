#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#include "statistics.h"

// this is the Marsaglia polar method (implementation from Wikipedia)
// it is supposed to be slower than the Ziggurat algorithm, 
// but said algorithm looks somewhat annoying to implement.
double random_gaussian(double stdDev) {
  static double spare;
  static int32_t hasSpare;
  if (hasSpare) {
    hasSpare = 0;
    return spare * stdDev;
  } else {
    double u, v, s, m = 2.0f / RAND_MAX;
    do {
      u = random() * m - 1.0f;
      v = random() * m - 1.0f;
      s = u * u + v * v;
    } while (s >= 1.0 || s == 0.0);
    s = sqrt(-2.0 * log(s) / s);
    spare = v * s;
    hasSpare = 1;
    return stdDev * u * s;
  }
}

// this code needs to be slightly different:
// the approach that makes intuitive sense to me is the one in which
// 0) we pass in the incoming ray, calculate its outgoing direction assuming no scattering
// 1) rotate the result in a random direction in accordance with the scattering value (sigma)
// 2) if the result now points into the object (has negative dot with normal vector)
//    recalculate the outgoing vector assuming no scattering and repeat from step 1. 
// The above is not exactly how this works now, but it should be how it works in future
void scatter(double sigma, double *normal, double *ray)
{
  {
    // if the incoming ray is going generally the same direction as the normal, the normal should be reversed
    // this is a quick fix and maybe this problem should be addressed more cleanly elsewhere
    double dot = normal[0] * ray[0] + normal[1] * ray[1] + normal[2] * ray[2];
    if (dot > 0.0f) {
      normal[0] *= -1.0f;
      normal[1] *= -1.0f;
      normal[2] *= -1.0f;
    }
  }
  if (sigma) {
    if (sigma == INFINITY) {
      // random scattering
      double r[3]; 
      double half = RAND_MAX * 0.5f;
      for (;;) {
        r[0] = (double) random() / half - 1.0f;
        r[1] = (double) random() / half - 1.0f;
        r[2] = (double) random() / half - 1.0f;
        double r2 = r[0] * r[0] + r[1] * r[1] + r[2] * r[2];
        if (r2 && r2 <= 1.0f) {
          double dot = r[0] * normal[0] + r[1] * normal[1] + r[2] * normal[2];
          if (dot > 0.0f) {
            double inverse = 1.0f / sqrt(r2);
            ray[0] = r[0] * inverse;
            ray[1] = r[1] * inverse;
            ray[2] = r[2] * inverse;
            return;
          }
        }
      }
    } else {
      // partial scattering
      double r[3]; 
      double half = RAND_MAX * 0.5f;
      for (;;) {
        double cosine = ray[0] * normal[0] + ray[1] * normal[1] + ray[2] * normal[2];
        ray[0] = ray[0] - 2 * cosine * normal[0];
        ray[1] = ray[1] - 2 * cosine * normal[1];
        ray[2] = ray[2] - 2 * cosine * normal[2];

        r[0] = (double) random() / half - 1.0f;
        r[1] = (double) random() / half - 1.0f;
        r[2] = (double) random() / half - 1.0f;
        double r2 = r[0] * r[0] + r[1] * r[1] + r[2] * r[2];
        if (r2 && r2 <= 1.0f) {
          double dot = r[0] * normal[0] + r[1] * normal[1] + r[2] * normal[2];
          if (dot < 0.99f * sqrt(r2)) {
            r[0] -= normal[0] * dot;
            r[1] -= normal[1] * dot;
            r[2] -= normal[2] * dot;
            double m = 1.0f / sqrt(r[0] * r[0] + r[1] * r[1] + r[2] * r[2]);
            r[0] *= m;
            r[1] *= m;
            r[2] *= m;

            double theta = random_gaussian(sigma);
            double c = cos(theta);
            double v = 1.0f - c;
            double s = sin(theta);
            double R[3][3];

            R[0][0] = (r[0] * r[0] * v) + c;
            R[0][1] = (r[0] * r[1] * v) - (r[2] * s);
            R[0][2] = (r[0] * r[2] * v) + (r[1] * s);
            R[1][0] = (r[1] * r[0] * v) + (r[2] * s);
            R[1][1] = (r[1] * r[1] * v) + c;
            R[1][2] = (r[1] * r[2] * v) - (r[0] * s);
            R[2][0] = (r[2] * r[0] * v) - (r[1] * s);
            R[2][1] = (r[2] * r[1] * v) + (r[0] * s);
            R[2][2] = (r[2] * r[2] * v) + c;

            double t[3] = {0.0f, 0.0f, 0.0f};
            for (int32_t i = 0; i < 3; i++) {
              for (int32_t j = 0; j < 3; j++) {
                t[i] += R[i][j] * ray[j];
              }
            }
            ray[0] = t[0];
            ray[1] = t[1];
            ray[2] = t[2];
            double dot = ray[0] * normal[0] + ray[1] * normal[1] + ray[2] * normal[2];
            if (dot > 0.0f) {
              break;
            }

          }
        }
      }
    }
  } else {
    // no scattering
    double dot = ray[0] * normal[0] + ray[1] * normal[1] + ray[2] * normal[2];
    ray[0] = ray[0] - 2 * dot * normal[0];
    ray[1] = ray[1] - 2 * dot * normal[1];
    ray[2] = ray[2] - 2 * dot * normal[2];
  }
  // was previously getting bad error amplification out of this function
  double inverse = 1.0f / sqrt(ray[0] * ray[0] + ray[1] * ray[1] + ray[2] * ray[2]);
  ray[0] *= inverse;
  ray[1] *= inverse;
  ray[2] *= inverse;
}










