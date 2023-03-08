#include <stdlib.h>

struct list {
  int32_t byte_count; // current number of bytes in buffer 
  int32_t byte_offset; // current number of bytes in use
  int32_t element_size; // size of single element in bytes
  int32_t element_count; // byte_count / element_size
  int32_t element_offset; // byte_offset / element_size
  void *buffer;
}

struct list *create(int32_t element_count, int32_t element_size)
{
  struct list *list = malloc(sizeof(*list));
  element_count += (element_count == 0);
  list->byte_count = element_count * element_size; 
  list->byte_offset = 0;
  list->element_size = element_size;
  list->element_count = element_count;
  list->element_offset = 0;
  list->buffer = malloc(l->byte_count);
  return l;
}

int32_t push(struct list *list, void *element)
{
  if (list->element_offset == list->element_count) {
    list->byte_count <<= 1; 
    list->buffer = realloc(list->buffer, list->byte_count);
  }
  memcpy((char *) list->buffer + list->byte_offset, element, list->element_size);
  list->byte_offset += list->element_size; 
  int32_t position = list->element_offset;
  list->element_offset++;
  return position;
}

void pop(struct list *list)
{
  if (list->element_offset) {
    list->byte_offset -= list->element_size;
    list->element_offset--;
  }
}

void *next(struct list *list)
{
  if (list->element_offset == list->element_count) {
    list->byte_count <<= 1; 
    list->buffer = realloc(list->buffer, list->byte_count);
  }
  int32_t offset = list->byte_offset;
  list->byte_offset += list->element_size;
  list->element_offset++;
  return (char *) list->buffer + offset;
}

