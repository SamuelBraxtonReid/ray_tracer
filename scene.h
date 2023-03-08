#ifndef SCENE_H
#define SCENE_H

#include <stdint.h>

#include "main.h"

struct object *create_scene(char *file_name, int32_t *length);

char *get_file_name(char *line);

int32_t parse_scene(char *file_name, struct object *objects, int32_t index, struct affine *affine, struct properties *properties);

int32_t parse_obj(char *file_name, struct object *objects, int32_t index, struct affine *affine, struct properties *properties);

int32_t measure_scene(char *file_name);

int32_t measure_obj(char *file_name);

#endif
