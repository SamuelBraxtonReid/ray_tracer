#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <string.h>

#include "main.h"
#include "parse.h"
#include "search.h"
#include "image.h"
#include "timer.h"
#include "colors.h"
#include "scene.h"
#include "matrix.h"

int main(int argc, char **argv)
{
  double start_Time = get_time();
  double Time = start_Time;

  uint64_t utime = time(NULL);
  printf("seed: %llu\n", utime);
  srandom(utime);

  int32_t width = 3024;
  int32_t height = 1964;
  int32_t sub_pixels = 1;
  int32_t bounces = 1000000;
  {
    int32_t *args[] = {0, &width, &height, &sub_pixels, &bounces};
    for (int32_t i = 1; i < argc; i++) {
      int32_t v = strtol(argv[i], NULL, 10);
      *args[i] = v > 0 ? v : *args[i];
    }
  }
  printf("%d x %d, %d, %d\n", width, height, sub_pixels, bounces);

  char *scene_file = "glass_box.txt";
  int32_t len;
  struct object *objects = create_scene(scene_file, &len);
  printf("%s: %d objects\n", scene_file, len);
  struct root root;
  generate_bounding_box(objects, len, &(root.box));
  partition(&(root.box), objects, len, &(root.node));
  printf("total nodes: %d\n", get_total());
  printf("parse: %f\n", get_time() - Time);
  Time = get_time(); 

  //double pos[] = {0.0f, 3.0f, 5.5f};
  //double pos[] = {0.0f, 5.0f, 4.5f};
  //double pos[] = {0.0f, 5.25f, 5.0f}; // desktop
  //double pos[] = {2.0f, 2.0f, 2.0f}; // new desktop
  double pos[] = {0.0f, 0.0f, 2.0f};
  //double pos[] = {0.0f, -2.0f, 0.0f}; // volume test
  //double pos[] = {-2.0f, 0.0f, 0.0f};
  //double pos[] = {-2.0f, 5.25f, 10.0f};
  //double look_at[] = {pos[0], pos[1], pos[2] - 1.0f};
  //double look_at[] = {0.0f, 2.4f, -1.5f};
  //double look_at[] = {0.0f, 1.8f, -1.5f};
  double look_at[] = {0, 0, 0};
  double above[] = {pos[0], pos[1] + 1.0f, pos[2]};
  double right[3];
  double up[3];
  double forward[3];
  basis(pos, above, look_at, right, up, forward);
  printf("right: %f %f %f\n", right[0], right[1], right[2]); 
  printf("up: %f %f %f\n", up[0], up[1], up[2]); 
  printf("forward: %f %f %f\n", forward[0], forward[1], forward[2]); 

  double fov_horizontal = M_PI * 0.5f;
  double scale = tan(fov_horizontal * 0.5f);
  int32_t sample_width = width * sub_pixels; 
  int32_t sample_height = height * sub_pixels;
   
  struct pixel *pixels = calloc(width * height, sizeof(*pixels));
  int32_t maximum_bounce = 0;
  int64_t total_bounce = 0;
  for (int32_t py = height - 1; py >= 0; py--) {
    if (py % 10 == 0) {
      printf("row: %d\n", py);
    }
    for (int32_t px = 0; px < width; px++) {
      double color[] = {0.0f, 0.0f, 0.0f};
      for (int32_t k = 0; k < sub_pixels; k++) {
        for (int32_t l = 0; l < sub_pixels; l++) {
          // new
          int32_t x = px * sub_pixels + k;
          int32_t y = py * sub_pixels + l;
          double xv = scale * (2 * x - (sample_width - 1)) / (sample_width - 1);  
          double yv = scale * (2 * y - (sample_height - 1)) / (sample_width - 1);
          double vec[] = {forward[0], forward[1], forward[2]};
          for (int32_t i = 0; i < 3; i++) {
            vec[i] += right[i] * xv;
            vec[i] += up[i] * yv;
          }
          double m = sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
          vec[0] /= m;
          vec[1] /= m;
          vec[2] /= m;

          // old
          struct ray result;
          for (int32_t i = 0; i < 3; i++) {
            result.vec[i] = vec[i];
            result.pos[i] = pos[i];
            result.color[i] = 1.0f;
          }
          result.excluded = 0;
         
          double best = INFINITY;
         
          int32_t broken = 0;
          int32_t bounce;
          int32_t light = 0;

          for (bounce = 0; bounce < bounces; bounce++) {
            struct ray r;
            for (int32_t i = 0; i < 3; i++) {
              r.vec[i] = result.vec[i];
              r.pos[i] = result.pos[i];
            }
            double avec[] = {fabs(r.vec[0]), fabs(r.vec[1]), fabs(r.vec[2])};
            double maximum = avec[0];
            r.kz = 0;
            for (int32_t i = 1; i < 3; i++) {
              if (avec[i] > maximum) {
                maximum = avec[i];
                r.kz = i;
              }
            }
            int32_t mod3[] = {1, 2, 0};
            r.kx = mod3[r.kz];
            r.ky = mod3[r.kx];
            
            if (r.vec[r.kz] < 0.0f) {
              int32_t t = r.kx;
              r.kx = r.ky;
              r.ky = t;
            }
            r.Sz = 1.0f / r.vec[r.kz];
            r.Sx = r.vec[r.kx] * r.Sz;
            r.Sy = r.vec[r.ky] * r.Sz;
           
            struct node *np = &(root.node);
            struct box box = root.box; 
            int32_t valid = 1;
            int32_t count = 0;
            double enter[3];
            double exit[3];
            for (int32_t k = 0; k < 3; k++) {
              if (r.vec[k]) {
                double t1 = (box.sides[k][0] - r.pos[k]) / r.vec[k];
                double t2 = (box.sides[k][1] - r.pos[k]) / r.vec[k];
                if (t1 < t2) {
                  enter[count] = t1;
                  exit[count] = t2;
                } else {
                  enter[count] = t2;
                  exit[count] = t1;
                }
                count++;
              } else if ((r.pos[k] - box.sides[k][0]) *
                         (r.pos[k] - box.sides[k][1]) > 0.0f) {
                valid = 0;
                break;
              }
            }
           
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
                double mag_ray = safe_magnitude(r.vec); 
                if (search(np, &box, last_enter, first_exit, &r, &result)) {
                  double dx = result.pos[0] - r.pos[0]; 
                  double dy = result.pos[1] - r.pos[1]; 
                  double dz = result.pos[2] - r.pos[2]; 
                  best = dx * dx + dy * dy + dz * dz;
                  if (result.excluded &&
                      (result.excluded->brightness[0] ||
                      result.excluded->brightness[1] ||
                      result.excluded->brightness[2])) {
                    light = 1;
                    break;        
                  }
                  double mag_result = safe_magnitude(result.vec);
                  if (fabs(mag_ray - mag_result) > 0.01) {
                    printf("ray, result: %f %f\n", mag_ray, mag_result);
                  }
                } else {
                  broken = 1;
                  break;
                }
              } else {
                broken = 1;
                break;
              }
            } else {
              broken = 1;
              break;
            }
          }
          if (bounce > maximum_bounce) {
            maximum_bounce = bounce;
          }
          total_bounce += bounce;
          if (bounce == bounces) {
            if (result.excluded->volume) {
              result.excluded->volume->status = 1;
            }
            //color[1] += 6.0f * sub_pixels * sub_pixels;
            printf("MANY\n");
            continue;
          }

          double vm = sqrt(result.vec[0] * result.vec[0] +
                           result.vec[1] * result.vec[1] + 
                           result.vec[2] * result.vec[2]);
          if (vm > 1.25 || vm < 0.8) {
            //printf("%p\n", (void *) result.excluded);
            //printf("color: %f %f %f\n", result.color[0], result.color[1], result.color[2]);
            printf("ERROR\n");
            color[0] = 6;
            color[1] = 0;
            color[2] = 6;
            continue;
          }
         
          if (best != INFINITY && light) {
            color[0] += result.color[0];
            color[1] += result.color[1];
            color[2] += result.color[2];
            //int32_t coord = py * input + px;
            /*
            int32_t coord = j * width + i;
            struct pixel *p = pixels + coord;
            p->color[0] += result.color[0];
            p->color[1] += result.color[1];
            p->color[2] += result.color[2];
            */
          } else {
            // quick and dirty sky?
            /*
            color[0] += result.color[0] * fabs(result.vec[0]);
            color[1] += result.color[1] * fabs(result.vec[1]);
            color[2] += result.color[2] * fabs(result.vec[2]);
            */
            //printf("HERE\n");
            if (result.vec[2] < 0.0f) {
              double T = (6.0f - result.pos[2]) / result.vec[2];
              int32_t x = result.vec[0] * T;
              x -= (result.vec[0] < 0.0f);
              int32_t y = result.vec[1] * T;
              y -= (result.vec[1] < 0.0f);
              if ((x + y) & 1) {
                color[0] += result.color[0];
                color[1] += result.color[1];
                color[2] += result.color[2];
              }
            }
            //if (result.vec[1] > 0.8f) {
              // intersection with a plane that is 6 units up
              /*
              double T = (6.0f - result.pos[1]) / result.vec[1];
              int32_t x = result.vec[0] * T;
              x -= (result.vec[0] < 0.0f);
              int32_t z = result.vec[2] * T;
              z -= (result.vec[2] < 0.0f);
              if ((x + z) & 1) {
                color[0] += result.color[0];
                color[1] += result.color[1];
                color[2] += result.color[2];
              }
              */
              //color[0] += result.color[0];
              //color[1] += result.color[1];
              //color[2] += result.color[2];
            //}
            
            //double dot = result.vec[0] + result.vec[1] + result.vec[2];
            //if (dot > 0.0f) {
              // three color
              /*
              double maximum = result.vec[0];
              int32_t best_index = 0;
              for (int32_t index = 1; index < 3; index++) {
                if (result.vec[index] > maximum) {
                  maximum = result.vec[index];
                  best_index = index;
                }
              }
              color[best_index] += result.color[best_index];
              */
              //color[0] += result.vec[0] * result.color[0];
              //color[1] += result.vec[1] * result.color[1];
              //color[2] += result.vec[2] * result.color[2];
            //}
            /*
            if (result.vec[1] >= 0.0f) {
              color[0] += result.color[0] * fabs(result.vec[0]);
              color[1] += result.color[1] * fabs(result.vec[1]);
              color[2] += result.color[2] * fabs(result.vec[2]);
              //printf("%f %f %f\n", color[0], color[1], color[2]);
              //color[0] += result.color[0];
              //color[1] += result.color[1];
              //color[2] += result.color[2];
            } else {
              // quick and dirty hell dimension?
              //color[0] += result.color[0];
            }
            */
          }
          // end old



        }
      }
      int32_t coord = ((height - 1) - py) * width + px;
      struct pixel *p = pixels + coord;
      p->color[0] = color[0];
      p->color[1] = color[1];
      p->color[2] = color[2];
    }
  }
  
  //free_partition(&(root.node));
  printf("render: %f\n", get_time() - Time);
  Time = get_time();

  printf("maximum bounce: %d\n", maximum_bounce);
  printf("average bounce: %f\n", (double) total_bounce / ((double) width * height * sub_pixels * sub_pixels));

  double maximum = 0.0f;
  double minimum = INFINITY;
  for (int32_t i = 0; i < width * height; i++) {
    for (int32_t j = 0; j < 3; j++) {
      if (pixels[i].color[j] < minimum) {
        minimum = pixels[i].color[j];
      } else if (pixels[i].color[j] > maximum) {
        maximum = pixels[i].color[j];
      }
    }
  }
  int32_t invert = 0;
  if (invert) {
    for (int32_t i = 0; i < width * height; i++) {
      for (int32_t j = 0; j < 3; j++) {
        pixels[i].color[j] = maximum - (pixels[i].color[j] - minimum);
      }
    }
  }
  if (maximum) {
    for (int32_t i = 0; i < width * height; i++) {
       pixels[i].color[0] /= maximum;
       pixels[i].color[1] /= maximum;
       pixels[i].color[2] /= maximum;
    }
  } else {
    for (int32_t i = 0; i < width * height; i++) {
       pixels[i].color[0] = 1.0f;
       pixels[i].color[1] = 0.0f;
       pixels[i].color[2] = 0.0f;
    }
  }
  //printf("Fucking maximum: %f\n", maximum);
  ppm("desktop.ppm", width, height, pixels);
  free(pixels);
  printf("write: %f\n", get_time() - Time);
  Time = get_time();
  printf("total: %f\n", Time - start_Time);

  free(objects);

  return 0;
}
