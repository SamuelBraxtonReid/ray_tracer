#ifndef LIST_H
#define LIST_H

struct list {
  int32_t byte_count; // current number of bytes in buffer 
  int32_t byte_offset; // current number of bytes in use
  int32_t element_size; // size of single element in bytes
  int32_t element_count; // byte_count / element_size
  int32_t element_offset; // byte_offset / element_size
  void *buffer;
}

struct list *create(int32_t element_count, int32_t element_size);

int32_t push(struct list *list, void *element);

void pop(struct list *list);

void *next(struct list *list);

#endif
