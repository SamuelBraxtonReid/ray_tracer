#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "search.h"
#include "timer.h"
#include "parse.h"
#include "statistics.h"

//#define BACKFACE_CULLING

struct ray *search(struct node *node, struct box *box, double enter, double exit, struct ray *ray, struct ray *result)
{
  if (node->axis >= 0) {
    int32_t count = 1;
    int32_t order;
    double center;
    double old_exit;
    int32_t axis = node->axis & 0x3;
    if (ray->vec[axis]) {
      center = (node->divide - ray->pos[axis]) / ray->vec[axis];
      if (center < 0.0f) {
        order = ray->pos[axis] > node->divide;
      } else if (center > 0.0f) {
        if (center < enter) {
          order = ray->pos[axis] < node->divide;
        } else if (center <= exit) {
          count = 2;
          order = ray->pos[axis] > node->divide;
          old_exit = exit;
          exit = center;
        } else {
          order = ray->pos[axis] > node->divide;
        }
      } else {
        order = ray->vec[axis] > 0.0f;
      }
    } else {
      order = ray->pos[axis] > node->divide;
    }
    if (node->axis > 2) {
      int32_t discard = node->axis >> 3;
      if (order == discard) {
        if (count == 1) {
          return 0;
        }
        enter = center;
        exit = old_exit;
      }
      double last = box->sides[axis][discard];
      box->sides[axis][discard] = node->divide;
      struct ray *r = search(node->contents.children, box, enter, exit, ray, result);
      box->sides[axis][discard] = last;
      return r;
    }
    for (;;) {
      int32_t not_order = !order;
      double last = box->sides[axis][not_order];
      box->sides[axis][not_order] = node->divide;
      if (search(node->contents.children + order, box, enter, exit, ray, result)) {
        box->sides[axis][not_order] = last;
        return result;
      }
      box->sides[axis][not_order] = last;
      if (count == 1) {
        return 0;
      }
      count = 1;
      order = not_order;
      enter = center;
      exit = old_exit;
    }
  } else {
    int32_t object_count = -(node->axis);
    double best = INFINITY;
    struct object *best_object = 0;
    double sphere_normal[] = {0.0f, 0.0f, 0.0f};
    for (int32_t i = 0; i < object_count; i++) {
      struct object *o = node->contents.objects + i;
      
      /*
      struct box test_box;
      if (!sub_bound(box, o, &test_box)) {
        printf("not in box\n");
      }
      */

      if (o->type == 0) {
        if (o->volume == NULL &&
            result->excluded &&
            result->excluded->id == o->id) {
          continue;
        }
        struct sphere *s = &(o->contents.sphere);
        // the below makes the assumption that ray->vec is a unit vector
        double a[3] = {s->center[0] - ray->pos[0], s->center[1] - ray->pos[1], s->center[2] - ray->pos[2]};
        double dist = ray->vec[0] * a[0] + ray->vec[1] * a[1] + ray->vec[2] * a[2];
        double d = dist * dist - (a[0] * a[0] + a[1] * a[1] + a[2] * a[2]) + s->radius * s->radius;
        if (o->volume == NULL || o->volume->status >= 0) {
          // ray is not inside the sphere
          dist -= sqrt(d);
        } else {
          // ray is inside the sphere
          dist += sqrt(d);
        }
        if (dist > 0.0f && dist < best) { // I am not sure that this check is still correct after transparency stuff
          best = dist;
          result->pos[0] = ray->pos[0] + ray->vec[0] * dist;
          result->pos[1] = ray->pos[1] + ray->vec[1] * dist;
          result->pos[2] = ray->pos[2] + ray->vec[2] * dist;
          double n[3] = {(result->pos[0] - s->center[0]) / s->radius, 
                         (result->pos[1] - s->center[1]) / s->radius, 
                         (result->pos[2] - s->center[2]) / s->radius};
          sphere_normal[0] = n[0];
          sphere_normal[1] = n[1];
          sphere_normal[2] = n[2];
          best_object = o;
        }

      } else if (o->type == 1) {
        if (result->excluded &&
            result->excluded->id == o->id) {
          continue;
        }
        struct triangle *t = &(o->contents.triangle);
        double time = watertight(ray, t); 
        //double time_w = moller_trumbore(ray, t); 
        if (time >= 0.0f && time < best) {
          //if (o->volume && o->volume->status <= 0) {
             
          //}
          best = time;
          best_object = o;
        }
      }
    }
    
    int32_t in_box = (best >= enter && best <= exit); // consider if this check is entirely correct
    if (!in_box &&
        best_object && 
        best_object->type == 1 && 
        best_object->data > -1) {
      double pos[3];
      pos[0] = ray->pos[0] + ray->vec[0] * best;
      pos[1] = ray->pos[1] + ray->vec[1] * best;
      pos[2] = ray->pos[2] + ray->vec[2] * best;
      pos[best_object->data] = best_object->contents.triangle.points[0][best_object->data];
      in_box = 1;
      for (int32_t j = 0; j < 3; j++) {
        if (pos[j] < box->sides[j][0] ||
            pos[j] > box->sides[j][1]) {
          in_box = 0;
          break;
        }
      }
    }

    if (in_box) {
      for (int32_t i = 0; i < 3; i++) {
        result->vec[i] = ray->vec[i];
      } 
      double n[3];
      if (best_object->type == 0) {
        scatter(best_object->sigma, sphere_normal, result->vec);
        n[0] = sphere_normal[0];
        n[1] = sphere_normal[1];
        n[2] = sphere_normal[2];
      } else if (best_object->type == 1) {
        struct triangle *t = &(best_object->contents.triangle);
        result->pos[0] = ray->pos[0] + ray->vec[0] * best;
        result->pos[1] = ray->pos[1] + ray->vec[1] * best;
        result->pos[2] = ray->pos[2] + ray->vec[2] * best;
        scatter(best_object->sigma, t->normal, result->vec);
        n[0] = t->normal[0];
        n[1] = t->normal[1];
        n[2] = t->normal[2];
      }
      if (best_object->volume) {
        int32_t reflect = (double) random() / RAND_MAX < best_object->volume->opacity;
        if (!reflect) {
          double n1 = 1.0f;
          double n2 = best_object->volume->index;
          if (best_object->volume->status != 1) {
            n1 = n2;
            n2 = 1.0f;
          }
          double dot = result->vec[0] * n[0] + result->vec[1] * n[1] + result->vec[2] * n[2];
          double sin1 = sqrt(1 - dot * dot);
          double sin2 = n1 * sin1 / n2;
          if (sin2 < 1.0f) {
            double p[3] = {result->vec[0] - n[0] * dot,
                           result->vec[1] - n[1] * dot,
                           result->vec[2] - n[2] * dot};
            double cos = sqrt(1.0f - sin2 * sin2);
            double tan = sin2 / cos; 
            double mul = tan / sqrt(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]);
            double r[3] = {(p[0] * mul) - n[0], (p[1] * mul) - n[1], (p[2] * mul) - n[2]};
            double inv = 1.0f / sqrt(r[0] * r[0] + r[1] * r[1] + r[2] * r[2]); 
            result->vec[0] = r[0] * inv;
            result->vec[1] = r[1] * inv;
            result->vec[2] = r[2] * inv;
            best_object->volume->status *= -1;
          }
        }
      }
      result->color[0] *= best_object->color[0];
      result->color[1] *= best_object->color[1];
      result->color[2] *= best_object->color[2];
      if (best_object->brightness[0] ||
          best_object->brightness[1] ||
          best_object->brightness[2]) {
        if (result->excluded && result->excluded->volume) {
          result->excluded->volume->status = 1;
        }
      }
      result->excluded = best_object;
      return result;
    }
  }
  return 0;
}

double watertight(struct ray *r, struct triangle *t)
{
  double A[] = {t->points[0][0] - r->pos[0], t->points[0][1] - r->pos[1], t->points[0][2] - r->pos[2]};
  double B[] = {t->points[1][0] - r->pos[0], t->points[1][1] - r->pos[1], t->points[1][2] - r->pos[2]};
  double C[] = {t->points[2][0] - r->pos[0], t->points[2][1] - r->pos[1], t->points[2][2] - r->pos[2]};
  double Ax = A[r->kx] - r->Sx * A[r->kz];
  double Ay = A[r->ky] - r->Sy * A[r->kz];
  double Bx = B[r->kx] - r->Sx * B[r->kz];
  double By = B[r->ky] - r->Sy * B[r->kz];
  double Cx = C[r->kx] - r->Sx * C[r->kz];
  double Cy = C[r->ky] - r->Sy * C[r->kz];
  double U = Cx * By - Cy * Bx;
  double V = Ax * Cy - Ay * Cx;
  double W = Bx * Ay - By * Ax;
#ifdef BACKFACE_CULLING
  if (U < 0.0f || V < 0.0f || W < 0.0f) {
    //if (U < 0.0f && V < 0.0f && W < 0.0f) {
      //printf("culled\n");
    //}
    return -1.0f;
  }
#else
  if ((U < 0.0f || V < 0.0f || W < 0.0f) &&
      (U > 0.0f || V > 0.0f || W > 0.0f)) {
    return -1.0f;
  }
#endif
  double det = U + V + W;
  if (det == 0.0f) {
    return -1.0f;
  }
  return (U * A[r->kz] + V * B[r->kz] + W * C[r->kz]) * r->Sz / det;
}

double moller_trumbore(struct ray *r, struct triangle *t)
{
  double *v0 = t->points[0];
  double *v1 = t->points[1];
  double *v2 = t->points[2];
  double edge1[] = {v1[0] - v0[0],
                    v1[1] - v0[1],
                    v1[2] - v0[2]};
  double edge2[] = {v2[0] - v0[0],
                    v2[1] - v0[1],
                    v2[2] - v0[2]};
  double h[] = {r->vec[1] * edge2[2] - r->vec[2] * edge2[1],
                r->vec[2] * edge2[0] - r->vec[0] * edge2[2],
                r->vec[0] * edge2[1] - r->vec[1] * edge2[0]};
  double a = edge1[0] * h[0] + edge1[1] * h[1] + edge1[2] * h[2];
  if (a == 0.0f) {
    return -1.0f;
  }
  double f = 1.0 / a;
  double s[] = {r->pos[0] - v0[0],
                r->pos[1] - v0[1],
                r->pos[2] - v0[2]};
  double u = f * (s[0] * h[0] + s[1] * h[1] + s[2] * h[2]);
  if (u < 0.0f || u > 1.0f) {
    return -1.0f;
  }
  double q[] = {s[1] * edge1[2] - s[2] * edge1[1],
                s[2] * edge1[0] - s[0] * edge1[2],
                s[0] * edge1[1] - s[1] * edge1[0]};
  double v = f * (r->vec[0] * q[0] + r->vec[1] * q[1] + r->vec[2] * q[2]);
  if (v < 0.0f || u + v > 1.0f) {
    return -1.0f;
  }
  return f * (edge2[0] * q[0] + edge2[1] * q[1] + edge2[2] * q[2]);
}
