#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "scene.h"
#include "matrix.h"
#include "parse.h"
#include "main.h"

/*
I want a function that measures how much space needs to be allocated and one that assumes that it has enough
Can these be the same function?

Also, having scene files in scene files brings up the question of recursion. Should be seriously considered.

OBJ and REC will only ever have the file name and nothing else on the line
The rule is that if there is more than 1 quotation mark, the stuff between the first and last quotation mark is treated as the file name
Otherwise, the first contiguous chunk of non-whitespace (after OBJ or REC) is treated as the file name

*/

struct object *create_scene(char *file_name, int32_t *length)
{
  *length = measure_scene(file_name);
  struct object *objects = malloc((*length) * sizeof(*objects));
  struct affine a;
  struct properties p;
  for (int32_t i = 0; i < 3; i++) {
    for (int32_t j = 0; j < 4; j++) {
      a.elements[i][j] = (i == j);
    }
    p.color[i] = 1.0f;
    p.brightness[i] = 0.0f;
  }
  p.sigma = 0.0f;
  p.opacity = 1.0f;
  p.index = 1.0f;
  p.volume_id = 0;
  p.group_id = 0;
  p.volume = NULL;
  a.winding = 0;
  *length = parse_scene(file_name, objects, 0, &a, &p);
  process_triangles(objects, *length);
  return objects;
}

char *get_file_name(char *line)
{
  while (isspace(line[0])) {
    line++;
  }
  if (line[0] == '\0') {
    return 0;
  }
  if (line[0] == '"') {
    int32_t last_quote = 0;
    for (int32_t i = 1; line[i]; i++) {
      if (line[i] == '"') {
        last_quote = i;
      }
    }
    if (last_quote > 1) {
      line[last_quote] = '\0';
      line++;
      return line;
    }
  }
  int32_t i;
  for (i = 0; line[i] && !isspace(line[i]); i++);
  line[i] = '\0';
  return line; 
}

int32_t parse_scene(char *file_name, struct object *objects, int32_t index, struct affine *affine, struct properties *properties)
{
  FILE *scene = fopen(file_name, "r");
  if (!scene) {
    printf("{%s}: This file could not be opened.\n", file_name);
    return index;
  }
  struct affine a = *affine;
  struct properties p = *properties;
  char line[4096];
  while (fgets(line, sizeof(line), scene)) {
    if (line[0] == 'T') {
      char *l = line + 3;
      struct triangle *t = &(objects[index].contents.triangle);
      int32_t too_few = 0;
      for (int32_t i = 0; i < 3 && !too_few; i++) {
        for (int32_t j = 0; j < 3 && !too_few; j++) {
          char *end;
          t->points[i][j] = strtod(l, &end);
          too_few = (l == end);
          l = end;
        }
      }
      if (too_few) {
        printf("{%s}: too few values to define a triangle\n", file_name);
      } else {
        objects[index].type = 1;
        transform(objects + index, &a);
        for (int32_t i = 0; i < 3; i++) {
          objects[index].color[i] = p.color[i];
          objects[index].brightness[i] = p.brightness[i];
        }
        objects[index].sigma = p.sigma;
        objects[index].id = index;
        objects[index].volume = p.volume;
        index++;
      }
    } else if (line[0] == 'S') {
      if (line[1] == 'I') { // SIG
        char *l = line + 3;
        while (isspace(l[0])) {
          l++;
        }
        if (l[0] == 'I') {
          printf("INFINITY\n");
          p.sigma = INFINITY;
        } else {
          printf("NOT INFINITY\n");
          char *end;
          double sigma = strtod(l, &end);
          if (l == end) {
            printf("{%s}: incorrect sigma\n", file_name);
          } else {
            p.sigma = sigma;
          }
        }
      } else { // SPH
        char *l = line + 3;
        int32_t too_few = 0;
        double values[4];
        for (int32_t i = 0; i < 4 && !too_few; i++) {
          char *end;
          values[i] = strtod(l, &end);
          too_few = (l == end);
          l = end;
        }
        if (too_few) {
          printf("{%s}: too few values to define a sphere\n", file_name);
        } else {
          objects[index].type = 0;
          struct sphere *s = &(objects[index].contents.sphere);
          s->center[0] = values[0];
          s->center[1] = values[1];
          s->center[2] = values[2];
          s->radius = fabs(values[3]);
          transform(objects + index, &a);
          for (int32_t i = 0; i < 3; i++) {
            objects[index].color[i] = p.color[i];
            objects[index].brightness[i] = p.brightness[i];
          }
          objects[index].sigma = p.sigma;
          objects[index].id = index;
          objects[index].volume = p.volume;
          index++;
        }
      }
    } else if (line[0] == 'R') {
      char *l = line + 3;
      l = get_file_name(l);
      if (l) {
        index = parse_scene(l, objects, index, &a, &p);
      }
    } else if (line[0] == 'O') {
      char *l = line + 3;
      l = get_file_name(l);
      if (l) {
        index = parse_obj(l, objects, index, &a, &p);
      }
    } else if (line[0] == 'A') {
      char *l = line + 3;
      while (isspace(l[0])) {
        l++;
      }
      if (l[0] == 'I') {
        for (int32_t i = 0; i < 3; i++) {
          for (int32_t j = 0; j < 4; j++) {
            a.elements[i][j] = (i == j);
          }
        }
        a.winding = 0;
      } else {
        int32_t too_few = 0;
        struct affine temp;
        for (int32_t i = 0; i < 3 && !too_few; i++) {
          for (int32_t j = 0; j < 4 && !too_few; j++) {
            char *end;
            temp.elements[i][j] = strtod(l, &end);
            too_few = (l == end);
            l = end;
          }
        }
        if (too_few) {
          printf("{%s}: too few values to define a transformation\n", file_name);
        } else {
          a = temp;
          double det = 0.0f;
          det += a.elements[1][0] * (a.elements[0][2] * a.elements[2][1] - a.elements[0][1] * a.elements[2][2]);
          det += a.elements[1][1] * (a.elements[0][0] * a.elements[2][2] - a.elements[0][2] * a.elements[2][0]);
          det += a.elements[1][2] * (a.elements[0][1] * a.elements[2][0] - a.elements[0][0] * a.elements[2][1]); 
          printf("det: %f\n", det);
          a.winding = (det < 0.0f);
        }
      }
    } else if (line[0] == 'C') {
      char *l = line + 3;
      int32_t too_few = 0;
      struct properties previous = p;
      for (int32_t i = 0; i < 3 && !too_few; i++) {
        char *end;
        p.color[i] = strtod(l, &end);
        too_few = (l == end);
        l = end;
      }
      if (too_few) {
        p = previous;
        printf("{%s}: too few values to define a color\n", file_name);
      }
    } else if (line[0] == 'B') {
      char *l = line;
      while (l[0] && !isspace(l[0])) {
        l++;
      }
      int32_t too_few = 0;
      struct properties previous = p;
      for (int32_t i = 0; i < 3 && !too_few; i++) {
        char *end;
        p.brightness[i] = strtod(l, &end);
        p.color[i] = p.brightness[i];
        too_few = (l == end);
        l = end;
      }
      if (too_few) {
        p = previous;
        printf("{%s}: too few values to define a color\n", file_name);
      }
    } else if (line[0] == 'G') {
      // group of volumes: opacity, index 
      char *l = line;
      while (l[0] && !isspace(l[0])) {
        l++;
      }
      double values[2];
      int32_t too_few = 0;
      for (int32_t i = 0; i < 2 && !too_few; i++) {
        char *end;
        values[i] = strtod(l, &end);
        too_few = (l == end);
        l = end;
      }
      if (!too_few) {
        printf("GROUP\n");
        p.volume = malloc(sizeof(struct volume));
        p.volume->id = p.volume_id;
        p.volume_id++;
        p.volume->status = 1;
        p.volume->opacity = values[0];
        p.volume->index = values[1];
        p.opacity = values[0];
        p.index = values[1];
        p.volume->group = malloc(sizeof(struct group));
        p.volume->group->id = p.group_id;
        p.group_id++;
        p.volume->group->status = 1; 
        p.volume->group->opacity = values[0];
        p.volume->group->index = values[1];
      } else {
        p.volume = NULL;
        printf("ending group\n");
      }
    } else if (line[0] == 'V') {
      // volume
      if (p.volume && p.volume->group) {
        // add to existing group, ignore any specified volume properties
        struct volume *previous = p.volume;
        struct volume *new = malloc(sizeof(*new));
        *new = *(p.volume);
        p.volume = new;
        p.volume->id = p.volume_id; 
        p.volume_id++;
      } else {
        // create standalone volume
        char *l = line;
        while (l[0] && !isspace(l[0])) {
          l++;
        }
        double values[2];
        int32_t too_few = 0;
        for (int32_t i = 0; i < 2 && !too_few; i++) {
          char *end;
          values[i] = strtod(l, &end);
          too_few = (l == end);  
          l = end;
        }
        if (!too_few) {
          static int32_t group_id;
          p.volume = malloc(sizeof(struct volume));
          p.volume->id = p.volume_id;
          p.volume_id++;
          p.volume->status = 1;
          p.volume->opacity = values[0];
          p.volume->index = values[1];
          p.opacity = values[0];
          p.index = values[1];
          p.volume->group = NULL;
        } else {
          printf("could not create volume: ending group\n");
          p.volume = NULL;
        }
      }
    }
  }
  return index;
}

int32_t parse_obj(char *file_name, struct object *objects, int32_t index, struct affine *affine, struct properties *properties)
{
  FILE *f = fopen(file_name, "r");
  if (!f) {
    printf("{%s}: This file could not be opened.\n", file_name);
    return index;
  }
  char s[4096];
  int32_t vc = 0;
  int32_t fc = 0;
  while (fgets(s, 4096, f)) {
    if (s[0] == 'v') {
      vc++;
    } else if (s[0] == 'f') {
      fc++;
    }
  }
  rewind(f);
  double *vs = malloc(vc * 3 * sizeof(vs));
  int32_t *fs = malloc(fc * 3 * sizeof(fs));
  int32_t vi = 0;
  int32_t fi = 0;
  while (fgets(s, 4096, f)) {
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
  int32_t *fsp = fs;
  double *vsp = vs - 3;
  for (int32_t i = 0; i < fc; i++) {
    struct triangle *t = &(objects[index].contents.triangle);
    objects[index].type = 1;
    memcpy(t->points[0], vsp + (fsp[0] * 3), sizeof(t->points[0]));
    memcpy(t->points[1], vsp + (fsp[1] * 3), sizeof(t->points[1]));
    memcpy(t->points[2], vsp + (fsp[2] * 3), sizeof(t->points[2]));
    transform(objects + index, affine);
    for (int32_t i = 0; i < 3; i++) {
      objects[index].color[i] = properties->color[i];
      objects[index].brightness[i] = properties->brightness[i];
    }
    objects[index].sigma = properties->sigma;
    //objects[index].opacity = properties->opacity;
    //objects[index].index = properties->index;
    objects[index].volume = properties->volume;
    fsp += 3;
    objects[index].id = index;
    index++;
  }
  free(fs);
  free(vs);
  return index;
}

int32_t measure_scene(char *file_name)
{
  FILE *scene = fopen(file_name, "r");
  if (!scene) {
    printf("{%s}: This file could not be opened.\n", file_name);
    return 0;
  }
  char line[4096];
  int32_t count = 0;
  while (fgets(line, sizeof(line), scene)) {
    if (line[0] == 'T' || line[0] == 'S') { // TRI and SPH
      count++; 
    } else if (line[0] == 'O' || line[0] == 'R') { // OBJ and REC
      char *l = line + 3; 
      l = get_file_name(l);
      if (!l) {
        continue;
      }
      if (line[0] == 'O') {
        count += measure_obj(l);
      } else if (line[0] == 'R') {
        count += measure_scene(l); 
      }
    }
  }
  fclose(scene);
  return count;
}

int32_t measure_obj(char *file_name)
{
  FILE *obj = fopen(file_name, "r");
  if (!obj) {
    printf("{%s}: This file could not be opened.\n", file_name);
    return 0;
  }
  char line[4096];
  int32_t count = 0;
  while (fgets(line, 4096, obj)) {
    count += (line[0] == 'f');
  }
  fclose(obj);
  return count;
}
