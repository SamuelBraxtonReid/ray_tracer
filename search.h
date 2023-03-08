#ifndef SEARCH_H
#define SEARCH_H

#include "main.h"

struct ray *search(struct node *node, struct box *box, double enter, double exit, struct ray *ray, struct ray *result);

double watertight(struct ray *r, struct triangle *t);

double moller_trumbore(struct ray *r, struct triangle *t);

#endif
