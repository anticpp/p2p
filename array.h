#ifndef __ARRAY_H__
#define __ARRAY_H__

typedef struct {
    int size;
    int cap;
    int elesize; /* Element size */
} array;

array* array_create(int element_size);
array* array_append(array *arr, const void *element);
array* array_remove(array *arr, int i);
void* array_at(array *arr, int i);
void* array_front(array *arr);
void* array_tail(array *arr);
void array_destroy(array *arr);

#endif
