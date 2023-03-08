#ifndef MAIN_H
#define MAIN_H

#define X 0
#define Y 1
#define Z 2

//#define R 0
//#define G 1
//#define B 2

#include <stdint.h>

struct ray {
  double pos[3];
  double vec[3];
  int32_t kx;
  int32_t ky;
  int32_t kz;
  double Sx;
  double Sy;
  double Sz;
  double color[3];
  struct object *excluded;
  
  //int32_t excluded; 
};

struct triangle {
  double normal[3];
  double points[3][3];
  // access like points[0...3][X...Z]
};

struct sphere {
  double center[3];
  double radius;
};

struct box {
  double sides[3][2];
  // access like sides[X...Z][0...1]
};

struct group {
  int32_t id;
  int32_t status;
  float opacity;
  float index;
};

// volume id is sequential
struct volume {
  int32_t id;
  int32_t status;
  float opacity;
  float index;
  struct group *group;
};

struct object {
  union object_contents {
    struct triangle triangle;
    struct sphere sphere;
    // struct circle c
  } contents;

  float color[3]; // color
  float brightness[3]; // brightness
  float sigma; // standard deviation of scattering
  struct volume *volume;

  int32_t type; // data type of contents
  int32_t data; // additional information
  int32_t id;
};

struct bounding_box {
  struct box box;
  struct object *object;
};

/*
struct triangle_box { // rename 'bounding box'
  struct box b;
  struct triangle *t; 
};
*/

struct node {
  /*
  // later plan:
  double divide;
  int32_t axis;
  int32_t children;
  */
  double divide;
  int32_t axis;
  union node_contents {
    struct node *children;
    struct object *objects;
  } contents;
};

struct root {
  struct box box;
  struct node node;
};

struct pixel {
  double color[3];
};

struct affine {
  double elements[3][4];
  int32_t winding;
};

struct properties {
  float color[3];
  float brightness[3];
  float sigma;
  float opacity;
  float index;
  int32_t volume_id;
  int32_t group_id;
  struct volume *volume;
  // ...
};

#endif
