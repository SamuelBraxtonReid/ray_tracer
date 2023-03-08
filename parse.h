#ifndef PARSE_H
#define PARSE_H

#include <stdint.h>

#include "main.h"

struct object *old_parse_obj(char *file_name, int32_t *p_len);

void process_triangles(struct object *objects, int32_t len);

int32_t get_total();

void partition(struct box *box, struct object *objects, int32_t len, struct node *node);

void free_partition(struct node *node);

void print_tree(struct node *node);

int32_t sub_bound(struct box *b, struct object *o, struct box *out);

int32_t sphere_sub_bound(struct box *b, struct sphere *s, struct box *out);

int32_t triangle_sub_bound(struct box *b, struct triangle *t, struct box *out);

void generate_bounding_box(struct object *objects, int32_t len, struct box *box);

//int32_t sphere_sub_bound(struct box *b, struct sphere *s, struct box *out);

void sort(double *list, int32_t size);

void create_sphere(struct sphere *s, double x, double y, double z, double r);

#endif
