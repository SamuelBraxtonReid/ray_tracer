#ifndef COLORS_H
#define COLORS_H

#include <stdint.h>

#include "main.h"

void color(struct object *objects, int32_t len, double r, double g, double b);

void random_color(struct object *objects, int32_t len);

void tropical(struct object *objects, int32_t len);

void heat(struct object *objects, int32_t len);

void greyscale(struct object *objects, int32_t len);

void grayscale(struct object *objects, int32_t len);

#endif
