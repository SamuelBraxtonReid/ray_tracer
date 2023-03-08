#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "parse.h"
#include "matrix.h"
//#include "my_malloc.h"

struct object *old_parse_obj(char *file_name, int32_t *p_len)
{
  FILE *f = fopen(file_name, "r");
  char s[256];
  int32_t vc = 0;
  int32_t fc = 0;
  while (fgets(s, 256, f)) {
    if (s[0] == 'v') {
      vc++;
    } else if (s[0] == 'f') {
      fc++;
    }
  }
  *p_len = fc;
  rewind(f);
  double *vs = malloc(vc * 3 * sizeof(vs));
  int32_t *fs = malloc(fc * 3 * sizeof(fs));
  int32_t vi = 0;
  int32_t fi = 0;
  while (fgets(s, 256, f)) {
    if (s[0] == 'v') {
      char *start = s + 1;
      for (int32_t i = 0; i < 3; i++) {
        char *end;
        vs[vi++] = strtod(start, &end);
        start = end;
      }
    } else if (s[0] == 'f') {
      char *start = s + 1;
      for (int32_t i = 0; i < 3; i ++) {
        char *end;
        fs[fi++] = strtol(start, &end, 10);
        start = end;
      }
    }
  }
  fclose(f);
  struct object *triangles = malloc(fc * sizeof(*triangles));
  int32_t *fsp = fs;
  double *vsp = vs - 3;
  for (int32_t i = 0; i < fc; i++) {
    struct triangle *t = &(triangles[i].contents.triangle);
    memcpy(t->points[0], vsp + (fsp[0] * 3), sizeof(t->points[0]));
    memcpy(t->points[1], vsp + (fsp[1] * 3), sizeof(t->points[1]));
    memcpy(t->points[2], vsp + (fsp[2] * 3), sizeof(t->points[2]));
    triangles[i].type = 1;
    fsp += 3;
  }
  free(fs);
  free(vs);
  return triangles;
}

void process_triangles(struct object *objects, int32_t len)
{
  for (int32_t i = 0; i < len; i++) {
    if (objects[i].type == 1) {
      struct triangle *t = &(objects[i].contents.triangle);
      double v12[] = {t->points[1][X] - t->points[0][X], t->points[1][Y] - t->points[0][Y], t->points[1][Z] - t->points[0][Z]};
      //double v23[] = {t->points[2][X] - t->points[1][X], t->points[2][Y] - t->points[1][Y], t->points[2][Z] - t->points[1][Z]};
      double v31[] = {t->points[0][X] - t->points[2][X], t->points[0][Y] - t->points[2][Y], t->points[0][Z] - t->points[2][Z]};
      double *n = t->normal;
      n[X] = v12[Z] * v31[Y] - v12[Y] * v31[Z];
      n[Y] = v12[X] * v31[Z] - v12[Z] * v31[X];
      n[Z] = v12[Y] * v31[X] - v12[X] * v31[Y];
      double im = 1.0f / sqrt(n[X] * n[X] + n[Y] * n[Y] + n[Z] * n[Z]);
      n[X] *= im; 
      n[Y] *= im; 
      n[Z] *= im; 
      double m2 = n[0] * n[0] + n[1] * n[1] + n[2] * n[2];
      printf("%d\n", (m2 > 1.0f) - (m2 < 1.0f));
      /*
      double m = n[0] * n[0] + n[1] * n[1] + n[2] * n[2];
      while (m != 1.0f) {
        m = sqrt(m);
        n[X] /= m;
        n[Y] /= m;
        n[Z] /= m;
        m = n[0] * n[0] + n[1] * n[1] + n[2] * n[2];
      }
      */
      objects[i].data = -1;
      double total = n[X] + n[Y] + n[Z];
      for (int32_t j = 0; j < 3; j++) {
        if (n[j] == total) {
          objects[i].data = j; 
          break;
        }
      }
    }
  }
}

struct b_e {
  double value;
  int32_t b_or_e;
};

int comp_b_e(const void *va, const void *vb)
{
  struct b_e *a = (struct b_e *)va;
  struct b_e *b = (struct b_e *)vb;
  if (a->value == b->value) {
    return a->b_or_e - b->b_or_e;
  }
  return a->value < b->value ? -1 : 1;
}

int32_t total;

int32_t get_total()
{
  int32_t t = total;
  total = 0;
  return t;
}

void partition(struct box *box, struct object *objects, int32_t len, struct node *node)
{
  // compute triangle bounding boxes
  struct bounding_box *contents = malloc(len * sizeof(*contents)); 
  for (int32_t i = 0; i < len; i++) {
    contents[i].object = objects + i;
    //contents[i].t = triangles + i;
    if (!sub_bound(box, objects + i, &(contents[i].box))) {
      printf("ERROR1: %d\n", (objects + i)->type);
    }
  }

  // pare box if necessary
  struct box min_box = contents[0].box;
  for (int32_t i = 0; i < len; i++) {
    for (int32_t j = 0; j < 3; j++) {
      if (contents[i].box.sides[j][0] < min_box.sides[j][0]) {
        min_box.sides[j][0] = contents[i].box.sides[j][0];
      }
      if (contents[i].box.sides[j][1] > min_box.sides[j][1]) {
        min_box.sides[j][1] = contents[i].box.sides[j][1];
      }
    }
  }
  double whd[6];
  for (int32_t i = 0; i < 3; i++) {
    whd[i] = box->sides[i][1] - box->sides[i][0];
    whd[i + 3] = whd[i];
  }
  for (;;) {
    double best_score = 0.0f;
    int32_t best_axis;
    int32_t best_side;
    for (int32_t i = 0; i < 3; i++) {
      double s1 = (min_box.sides[i][0] - box->sides[i][0]);
      double s2 = (box->sides[i][1] - min_box.sides[i][1]); 
      double v = whd[i + 1] + whd[i + 2];
      if (s1 > s2) {
        s1 *= v;
        if (s1 > best_score) {
          best_score = s1;
          best_axis = i;
          best_side = 0;
        }
      } else {
        s2 *= v;
        if (s2 > best_score) {
          best_score = s2;
          best_axis = i;
          best_side = 1;
        }
      }
    }
    double d = whd[0] * (whd[1] + whd[2]) + whd[1] * whd[2];
    if (best_score * 5.0f > d) {
      // so if the value is higher, we cut off the higher side
      node->axis = best_axis + (best_side + 1) * 4;
      node->divide = min_box.sides[best_axis][best_side];
      box->sides[best_axis][best_side] = node->divide;
      whd[best_axis] = box->sides[best_axis][1] - box->sides[best_axis][0];
      whd[best_axis + 3] = whd[best_axis];
      struct node *new_node = malloc(sizeof(struct node));
      //struct node *new_node = my_malloc(sizeof(struct node));
      //total++;
      node->contents.children = new_node;
      node = new_node;
    } else {
      break;
    }
  }

  /*
  for (int32_t i = 0; i < len; i++) {
    struct box test_box; 
    if (!sub_bound(box, objects + i, &test_box)) {
      printf("ERROR2: %d\n", (objects + i)->type);
    }
  }
  */
   
  /*
  for (int32_t i = 0; i < len; i++) {
    struct triangle *t = &(objects[i].contents.triangle);
    for (int32_t j = 0; j < 3; j++) {
      for (int32_t k = 0; k < 3; k++) {
        if (t->points[j][k] < box->sides[k][0] ||
            t->points[j][k] > box->sides[k][1]) {
          
          printf("not in box:\n");
          for (int32_t l = 0; l < 3; l++) {
            printf("p%d: ", l);
            for (int32_t m = 0; m < 3; m++) {
              printf("%f ", t->points[l][m]);
            }
            printf("\n");
          }
          char *xyz = "xyz";
          for (int32_t l = 0; l < 3; l++) {
            printf("%c0: %0.3f, %c1: %0.3f\n", xyz[l], box->sides[l][0], xyz[l], box->sides[l][1]);
          }
          printf("\n");
        }
      }
    }
  }
  */
  struct b_e *b_and_e = malloc(2 * len * sizeof(*b_and_e));
  double mid[3];
  int32_t min[3] = {len, len, len};
  int32_t found = 0;
  for (int32_t i = 0; i < 3; i++) {
    int32_t lowest = len * 2;
    for (int32_t j = 0; j < len; j++) {
      b_and_e[j].b_or_e = 0;
      b_and_e[j].value = contents[j].box.sides[i][0];
      b_and_e[j + len].b_or_e = 1;
      b_and_e[j + len].value = contents[j].box.sides[i][1];
    }
    qsort(b_and_e, len * 2, sizeof(*b_and_e), comp_b_e);
    int32_t l = 1;
    int32_t r = len;
    for (int32_t k = 1; k < 2 * len; k++) {
      if (b_and_e[k].b_or_e == 0) {
        if (b_and_e[k - 1].b_or_e == 1) {
          int32_t max = l > r ? l : r;
          if (max < len && max <= min[i]) {
            if (max < min[i]) {
              found = 1;
              min[i] = max;
              lowest = l + r;
              mid[i] = (b_and_e[k - 1].value + b_and_e[k].value) * 0.5f;
            } else if (l + r < lowest) {
              found = 1;
              lowest = l + r;
              mid[i] = (b_and_e[k - 1].value + b_and_e[k].value) * 0.5f;
            }
          }
        }
        l++;
        if (l > min[i]) {
          break;
        }
      } else {
        r--;
      }
    }
  }
  free(b_and_e);

  if (!found) {
    node->axis = -len;
    //node->contents.triangles = malloc(len * sizeof(struct triangle));
    node->contents.objects = malloc(len * sizeof(struct object));
    //node->contents.triangles = my_malloc(len * sizeof(struct triangle));
    for (int32_t i = 0; i < len; i++) {
      //node->contents.triangles[i] = *(contents[i].t);
      node->contents.objects[i] = *(contents[i].object);
    }
    /*
    double volume = 1.0f;
    for (int32_t i = 0; i < 3; i++) {
      volume *= fabs(box->sides[i][1] - box->sides[i][0]);
    }
    printf("leaf volume: %f\n", volume);
    */
    return;
  }

  int32_t minima[3];
  int32_t mindex = 0;
  int32_t minimum = len - 1;
  for (int32_t i = 0; i < 3; i++) {
    if (min[i] <= minimum) {
      if (min[i] < minimum) {
        minimum = min[i];
        minima[0] = i;
        mindex = 1;
      } else {
        minima[mindex++] = i;
      }
    }
  }

  node->axis = minima[random() % mindex];
  //printf("%d\n", node->axis);
  node->divide = mid[node->axis];
  node->contents.children = malloc(2 * sizeof(struct node));
  //node->contents.children = my_malloc(2 * sizeof(struct node));
  total += 2;
  struct object *values = malloc(minimum * sizeof(*values));

  struct box b1 = *box;
  b1.sides[node->axis][1] = node->divide;
  int32_t count = 0;
  for (int32_t i = 0; i < len; i++) {
    if (contents[i].box.sides[node->axis][0] < node->divide) {
      values[count++] = *(contents[i].object);
    }
  }
  partition(&b1, values, count, node->contents.children);

  struct box b2 = *box;
  b2.sides[node->axis][0] = node->divide;
  count = 0;
  for (int32_t i = 0; i < len; i++) {
    if (contents[i].box.sides[node->axis][1] > node->divide) {
      values[count++] = *(contents[i].object);
    }
  }
  partition(&b2, values, count, node->contents.children + 1);

  free(values);
  free(contents);
}

void free_partition(struct node *node)
{
  if (node->axis >= 0) {
    free_partition(node->contents.children);
    if (node->axis < 3) {
      free_partition(node->contents.children + 1);
    }
    free(node->contents.children);
  } else {
    free(node->contents.objects);
  }
}

void print_tree(struct node *node)
{
  // this is broken: need to account for new tree structure (and other stuff as well)
  /*
  static int32_t depth = 0;
  if (node->axis >= 0) {
    char xyz[] = "xyz";
    for (int32_t i = 0; i < depth; i++) {
      printf(" ");
    }
    printf("%c %f:\n", xyz[node->axis], node->divide);
    depth++;
    print_tree(node->contents.children);
    printf("\n");
    print_tree(node->contents.children + 1);
    depth--;
  } else {
    for (int32_t i = 0; i > node->axis; i--) {
      for (int32_t j = 0; j < depth; j++) {
        printf(" ");
      }
      for (int32_t j = 0; j < 3; j++) {
        printf("(");
        for (int32_t k = 0; k < 3; k++) {
          printf("%f ", node->contents.triangles[-i].points[j][k]);
        }
        printf(") ");
        printf("%f ", node->contents.triangles[-i].color[j]);
      }
      printf("\n");
    }
  }
  */
}

int32_t sub_bound(struct box *b, struct object *o, struct box *out)
{
  if (o->type == 0) { // sphere
    //printf("sphere\n");
    return sphere_sub_bound(b, &(o->contents.sphere), out);
  } else if (o->type == 1) { // triangle
    return triangle_sub_bound(b, &(o->contents.triangle), out);
  }
  return 0;
}

int32_t sphere_sub_bound(struct box *b, struct sphere *s, struct box *out)
{
  double extreme[54][3];
  int32_t index = 0;

  // extreme points of sphere
  double offset[] = {-s->radius, s->radius};
  for (int32_t i = 0; i < 3; i++) {
    for (int32_t j = 0; j < 2; j++) {
      extreme[index][0] = s->center[0];
      extreme[index][1] = s->center[1];
      extreme[index][2] = s->center[2];
      extreme[index][i] += offset[j];
      int32_t inside = 1;
      for (int32_t k = 0; k < 3; k++) {
        if (extreme[index][k] < b->sides[k][0] ||
            extreme[index][k] > b->sides[k][1]) {
          inside = 0;
          break;
        }
      }
      if (inside) {
        index++;
      }
    }
  }
  if (index == 6) {
    goto end;
  }

  struct box o;
  for (int32_t i = 0; i < 3; i++) {
    o.sides[i][0] = b->sides[i][0] - s->center[i];
    o.sides[i][1] = b->sides[i][1] - s->center[i];
  }

  // intersection of axis-aligned great circles with faces of box
  double r_squared = s->radius * s->radius;
  int32_t plus_1_mod_3[] = {1, 2, 0};
  for (int32_t i = 0; i < 3; i++) {
    int32_t i2 = plus_1_mod_3[i];
    int32_t i3 = plus_1_mod_3[i2];
    for (int32_t j = 0; j < 2; j++) {
      if (fabs(o.sides[i][j]) <= s->radius) {
        double v = sqrt(r_squared - (o.sides[i][j] * o.sides[i][j]));
        double p[4][2] = {{-v, 0.0f}, {v, 0.0f}, {0.0f, -v}, {0.0f, v}};
        for (int32_t k = 0; k < 4; k++) {
          extreme[index][i2] = p[k][0] + s->center[i2];
          if (extreme[index][i2] >= b->sides[i2][0] &&
              extreme[index][i2] <= b->sides[i2][1]) {
            extreme[index][i3] = p[k][1] + s->center[i3];
            if (extreme[index][i3] >= b->sides[i3][0] &&
                extreme[index][i3] <= b->sides[i3][1]) {
              extreme[index][i] = b->sides[i][j];
              index++;
            }
          }
        }
      }
    }
  }

  // intersection of box edges with sphere
  for (int32_t i = 0; i < 3; i++) {
    for (int32_t j = i + 1; j < 3; j++) {
      for (int32_t k = 0; k < 2; k++) {
        double av = o.sides[i][k];
        for (int32_t l = 0; l < 2; l++) {
          double bv = o.sides[j][l];
          double cv = r_squared - av * av - bv * bv;
          if (cv >= 0.0f) {
            cv = sqrt(cv);
            int32_t c_index = 3 - i - j;
            for (int32_t m = 0; m < 2; m++) {
              extreme[index][c_index] = (m ? cv : -cv) + s->center[c_index];
              if (extreme[index][c_index] >= b->sides[c_index][0] &&
                  extreme[index][c_index] <= b->sides[c_index][1]) {
                extreme[index][i] = b->sides[i][k];
                extreme[index][j] = b->sides[j][l];
                index++;
              }
            }
          }
        }
      }
    }
  }

  if (index == 0) {
    return 0;
  }

  end:
   
  for (int32_t i = 0; i < 3; i++) {
    out->sides[i][0] = extreme[0][i];
    out->sides[i][1] = extreme[0][i];
  }
  for (int32_t i = 1; i < index; i++) {
    for (int32_t j = 0; j < 3; j++) {
      if (extreme[i][j] < out->sides[j][0]) {
        out->sides[j][0] = extreme[i][j];
      } else if (extreme[i][j] > out->sides[j][1]) {
        out->sides[j][1] = extreme[i][j];
      }
    }
  }

  /*
  printf("out_box: (%f %f) (%f %f) (%f %f)\n", out->sides[0][0]
                                             , out->sides[0][1]
                                             , out->sides[1][0]
                                             , out->sides[1][1]
                                             , out->sides[2][0]
                                             , out->sides[2][1]);
*/

  return 1;
}

int32_t triangle_sub_bound(struct box *b, struct triangle *t, struct box *out)
{
  double extreme[33][3];
  int32_t index = 0;

  // vertices of triangle contained by box
  for (int32_t i = 0; i < 3; i++) {
    double *point = t->points[i];
    if (point[0] >= b->sides[X][0] && point[0] <= b->sides[X][1] &&
        point[1] >= b->sides[Y][0] && point[1] <= b->sides[Y][1] &&
        point[2] >= b->sides[Z][0] && point[2] <= b->sides[Z][1]) {
      extreme[index][0] = point[0];
      extreme[index][1] = point[1];
      extreme[index][2] = point[2];
      index++;
    }
  }
  if (index == 3) {
    goto end;
  }

  // intersection of triangle edges with box
  double v1_2[] = {t->points[1][X] - t->points[0][X], t->points[1][Y] - t->points[0][Y], t->points[1][Z] - t->points[0][Z]};
  double v2_3[] = {t->points[2][X] - t->points[1][X], t->points[2][Y] - t->points[1][Y], t->points[2][Z] - t->points[1][Z]};
  double v3_1[] = {t->points[0][X] - t->points[2][X], t->points[0][Y] - t->points[2][Y], t->points[0][Z] - t->points[2][Z]};
  double *edges[3] = {v1_2, v2_3, v3_1};
  for (int32_t i = 0; i < 3; i++) {
    double *edge = edges[i];
    double *point = t->points[i];
    for (int32_t j = 0; j < 3; j++) {
      if (edge[j]) {
        for (int32_t k = 0; k < 2; k++) {
          double side = b->sides[j][k];
          double f = (side - point[j]) / edge[j];
          if (f >= 0.0f && f <= 1.0f) {
            double rsc[3];
            rsc[0] = point[0] + edge[0] * f;
            rsc[1] = point[1] + edge[1] * f;
            rsc[2] = point[2] + edge[2] * f;
            rsc[j] = side;
            if (rsc[0] >= b->sides[X][0] && rsc[0] <= b->sides[X][1] &&
                rsc[1] >= b->sides[Y][0] && rsc[1] <= b->sides[Y][1] &&
                rsc[2] >= b->sides[Z][0] && rsc[2] <= b->sides[Z][1]) {
              extreme[index][0] = rsc[0];
              extreme[index][1] = rsc[1];
              extreme[index][2] = rsc[2];
              index++;
            }
          }
        }
      }
    }
  }

  // intersection of box edges wih triangle
  double n[3];
  n[X] = v1_2[Z] * v3_1[Y] - v1_2[Y] * v3_1[Z];
  n[Y] = v1_2[X] * v3_1[Z] - v1_2[Z] * v3_1[X];
  n[Z] = v1_2[Y] * v3_1[X] - v1_2[X] * v3_1[Y];
  double im = 1.0f / sqrt(n[X] * n[X] + n[Y] * n[Y] + n[Z] * n[Z]);
  n[X] *= im;
  n[Y] *= im;
  n[Z] *= im;
  double v = t->points[0][X] * n[X] + t->points[0][Y] * n[Y] + t->points[0][Z] * n[Z];
  for (int32_t i = 0; i < 3; i++) {
    if (n[i]) {
      int32_t i2 = (i + 1) % 3;
      int32_t i3 = (i + 2) % 3;
      for (int32_t j = 0; j < 2; j++) {
        for (int32_t k = 0; k < 2; k++) {
          double r = (v - b->sides[i2][j] * n[i2] - b->sides[i3][k] * n[i3]) / n[i];
          if (r >= b->sides[i][0] && r <= b->sides[i][1]) {

            double p1[] = {t->points[0][i2], t->points[0][i3]};
            double p2[] = {t->points[1][i2], t->points[1][i3]};
            double p3[] = {t->points[2][i2], t->points[2][i3]};
            double p[] = {b->sides[i2][j], b->sides[i3][k]};

            double denominator = ((p2[1] - p3[1]) * (p1[0] - p3[0]) + (p3[0] - p2[0]) * (p1[1] - p3[1]));
            double alpha = ((p2[1] - p3[1]) * (p[0] - p3[0]) + (p3[0] - p2[0]) * (p[1] - p3[1])) / denominator;
            if (alpha < 0.0f) {
              continue;
            }
            double beta = ((p3[1] - p1[1]) * (p[0] - p3[0]) + (p1[0] - p3[0]) * (p[1] - p3[1])) / denominator;
            if (beta < 0.0f) {
              continue;
            }
            double gamma = 1.0f - alpha - beta; 
            if (gamma < 0.0f) {
              continue;
            }

            extreme[index][i] = r;
            extreme[index][i2] = b->sides[i2][j];
            extreme[index][i3] = b->sides[i3][k];
            index++;
            
            /*
            double intersection[3];
            intersection[i] = r;
            intersection[i2] = b->sides[i2][j];
            intersection[i3] = b->sides[i3][k];

            double x = intersection[0] * t->t1[0] + intersection[1] * t->t1[1] + intersection[2] * t->t1[2] - t->x_offset;
            if (x < 0.0f || x > 1.0f) {
              continue;
            }
            double y = intersection[0] * t->t2[0] + intersection[1] * t->t2[1] + intersection[2] * t->t2[2] - t->y_offset;
            if (y < x || y > 1.0f) {
              continue;
            }

            extreme[index][0] = intersection[0];
            extreme[index][1] = intersection[1];
            extreme[index][2] = intersection[2];
            index++;
            */
          }
        }
      }
    }
  }

  if (index == 0) {
    return 0;
  }

  end:

  for (int32_t i = 0; i < 3; i++) {
    out->sides[i][0] = extreme[0][i];
    out->sides[i][1] = extreme[0][i];
  }
  for (int32_t i = 1; i < index; i++) {
    for (int32_t j = 0; j < 3; j++) {
      if (extreme[i][j] < out->sides[j][0]) {
        out->sides[j][0] = extreme[i][j];
      } else if (extreme[i][j] > out->sides[j][1]) {
        out->sides[j][1] = extreme[i][j];
      }
    }
  }

  return 1;
}

void generate_bounding_box(struct object *objects, int32_t len, struct box *box)
{
  if (len) {
    if (objects[0].type == 0) {
      struct sphere *s = &(objects[0].contents.sphere); 
      for (int32_t i = 0; i < 3; i++) {
        box->sides[i][0] = s->center[i] - s->radius;
        box->sides[i][1] = s->center[i] + s->radius;
      }
    } else if (objects[0].type == 1) {
      for (int32_t i = 0; i < 3; i++) {
        box->sides[i][0] = objects[0].contents.triangle.points[0][i];
        box->sides[i][1] = objects[0].contents.triangle.points[0][i];
      }
    }
    for (int32_t i = 0; i < len; i++) {
      if (objects[i].type == 0) {
        struct sphere *s = &(objects[i].contents.sphere); 
        for (int32_t j = 0; j < 3; j++) {
          double small = s->center[j] - s->radius;
          double large = s->center[j] + s->radius; 
          if (small < box->sides[j][0]) {
            box->sides[j][0] = small;
          }
          if (large > box->sides[j][1]) {
            box->sides[j][1] = large;
          }
        }
      } else if (objects[i].type == 1) {
        struct triangle *t = &(objects[i].contents.triangle);
        for (int32_t j = 0; j < 3; j++) {
          for (int32_t k = 0; k < 3; k++) {
            if (t->points[j][k] < box->sides[k][0]) {
              box->sides[k][0] = t->points[j][k];
            } else if (t->points[j][k] > box->sides[k][1]) {
              box->sides[k][1] = t->points[j][k];
            }
          }
        }
      }
    }
  }
}

void sort(double *list, int32_t size)
{
  if (size < 22) {
    for (int32_t i = 1; i < size; i++) {
      for (int32_t j = i; j > 0 && list[j] < list[j - 1]; j--) {
        double t = list[j];
        list[j] = list[j - 1];
        list[j - 1] = t;
      }
    }
    return;
  }
  double p = list[random() % size];
  int32_t i = 0;
  int32_t j = size - 1;
  for (;;) {
    while (list[i] < p) {
      i++;
    }
    while (list[j] > p) {
      j--;
    }
    if (list[i] != list[j]) {
      double t = list[i];
      list[i] = list[j];
      list[j] = t;
    } else if (i == j) {
      sort(list, i++);
      sort(list + i, size - i);
      return;
    } else {
      (i ^ j) & 1 ? i++ : j--;
    }
  }
}

void create_sphere(struct sphere *s, double x, double y, double z, double r)
{
  s->center[0] = x; 
  s->center[1] = y; 
  s->center[2] = z; 
  s->radius = r;
}

/*
void ray_box_intersection(struct ray *ray, struct box *box, double *enter, double *exit)
{
  int32_t valid = 1;
  int32_t count = 0;
  double enteries[3];
  double exitits[3];
  for (int32_t i = 0; i < 3; i++) {
    if (ray.vec[i]) {
      double t1 = (box.sides[i][0] - ray.pos[i]) / ray.vec[i];
      double t2 = (box.sides[i][1] - ray.pos[i]) / ray.vec[i];
      if (t1 < t2) {
        enter[count] = t1;
        exit[count] = t2;
      } else {
        enter[count] = t2;
        exit[count] = t1;
      }
      count++;
    } else if ((ray.pos[i] - box.sides[i][0]) *
               (ray.pos[i] - box.sides[i][1]) > 0.0f) {
      valid = 0;
      breai;
    }
  }

  struct ray result;
  result.color[0] = 1.0f;
  result.color[1] = 1.0f;
  result.color[2] = 1.0f;
  if (valid) {
    double last_enter = enter[0];
    double first_exit = exit[0];
    for (int32_t k = 1; k < count; k++) {
      if (enter[k] > last_enter) {
        last_enter = enter[k];
      }
      if (exit[k] < first_exit) {
        first_exit = exit[k];
      }
    }
    if (last_enter <= first_exit) {
      for (int32_t k = 0; k < 2; k++) {
        if (search(np, &box, last_enter, first_exit, &r, &result)) {
          //double dx = result.pos[0] - r.pos[0]; 
          //double dy = result.pos[1] - r.pos[1]; 
          //double dz = result.pos[2] - r.pos[2]; 
          best = 1.0f;
          //best = dx * dx + dy * dy + dz * dz;
          r = result;
        } else {
          break;
        }
  
}
*/

/*
int32_t sphere_sub_bound(struct box *b, struct sphere *s, struct box *out)
{

}
*/

