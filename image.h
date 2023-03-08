#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h>

#include "main.h"

void dither(uint32_t width, uint32_t height, struct pixel *pixels);

void png(char *file_name, uint32_t width, uint32_t height, struct pixel *pixels);

void ppm(char *file_name, uint32_t width, uint32_t height, struct pixel *pixels);

#endif
