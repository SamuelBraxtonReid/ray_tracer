#include <stdio.h>
#include <stdlib.h>

#if defined(__APPLE__) || defined(__linux__) || defined(__unix__)
#include <unistd.h>
#endif

#include "image.h"

/*
void dither(uint32_t width, uint32_t height, struct pixel *pixels)
{

}
*/

/*
void png(char *file_name, uint32_t width, uint32_t height, struct pixel *pixels)
{
  
}
*/

void ppm(char *file_name, uint32_t width, uint32_t height, struct pixel *pixels)
{
  uint64_t len = width * height;
  uint8_t *data = malloc(30 + len * 3);
  uint8_t *p = data + sprintf((char *)data, "P6\n%u %u\n255\n", width, height);
  for (uint64_t i = 0; i < len; i++) {
    for (int32_t j = 0; j < 3; j++) {
      int32_t v = pixels[i].color[j] * 256.0f;
      if (v > 255) {
        v = 255;
      } else if (v < 0) {
        v = 0;
      }
      p[j] = v;
    }
    p += 3;
  }
  FILE *f = fopen(file_name, "wb");
#if defined(__APPLE__) || defined(__linux__) || defined(__unix__)
  write(fileno(f), data, p - data);
#else
  fwrite(data, 1, p - data, f);
#endif
  fclose(f);
  free(data);
}
