#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "colors.h"

void color(struct object *objects, int32_t len, double r, double g, double b)
{
  for (int32_t i = 0; i < len; i++) {
    objects[i].color[0] = r;
    objects[i].color[1] = g;
    objects[i].color[2] = b;
  }
}

void random_color(struct object *objects, int32_t len)
{
  for (int32_t i = 0; i < len; i++) {
    objects[i].color[0] = (double) random() / RAND_MAX;
    objects[i].color[1] = (double) random() / RAND_MAX;
    objects[i].color[2] = (double) random() / RAND_MAX;
  } 
}

void tropical(struct object *objects, int32_t len)
{
  random_color(objects, len); 
  for (int32_t i = 0; i < len; i++) {
    objects[i].color[random() % 3] = 1.0f;
  }
}

void heat(struct object *objects, int32_t len)
{
  for (int32_t i = 0; i < len; i++) {
    double temperature = (double) random() * 3.0f / RAND_MAX ;
    for (int32_t j = 0; j < 3; j++) {
      double value = temperature - j;
      if (value < 0.0f) {
        value = 0.0f;
      } else if (value > 1.0f) {
        value = 1.0f;
      }
      objects[i].color[j] = value;
    }
  } 
}

void greyscale(struct object *objects, int32_t len)
{
  for (int32_t i = 0; i < len; i++) {
    double magnitude = (double) random() / RAND_MAX;
    objects[i].color[0] = magnitude;
    objects[i].color[1] = magnitude;
    objects[i].color[2] = magnitude;
  } 
}

void grayscale(struct object *objects, int32_t len)
{
  greyscale(objects, len);
}
