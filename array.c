#include "array.h"
#include <stdlib.h>
#include <string.h>

static const int array_inc_size_=10;

/* Private functions */
static char* retrive_data_(array *arr) {
    return (char*)(arr) + sizeof(array);
}


/* APIs */
array* array_create(int element_size) {
    int size = sizeof(array)+array_inc_size_*element_size;
    array *arr = malloc( sizeof(array) );
    arr->size = 0;
    arr->cap = 0;
    arr->elesize = element_size;
    return arr;
}

array* array_append(array *arr, const void *element) {
    if( arr->cap-arr->size==0 ) {
        /* Extend */
        arr = realloc(arr, sizeof(array)+(arr->cap+array_inc_size_)*arr->elesize);
        arr->cap += array_inc_size_;
    }
    
    char *data = retrive_data_(arr);
    memcpy( data+arr->size*arr->elesize, element, arr->elesize );
    arr->size ++;
    return arr;
}

void* array_at(array *arr, int i) {
    if( i<0 || i>=arr->size ) 
        return 0;
    char *data = retrive_data_(arr);
    return data+i*arr->elesize;
}

void* array_front(array *arr) {
    if( arr->size==0 )
        return 0;
    return array_at(arr, 0);
}

void* array_tail(array *arr) {
    if( arr->size==0 )
        return 0;
    return array_at(arr, arr->size-1);
}

array* array_remove(array *arr, int i) {
    if( i>=arr->size ) {
        return arr;
    }
    char *data = retrive_data_(arr);
    memmove( data+i*arr->elesize, 
                    data+(i+1)*arr->elesize, 
                    (arr->size-i)*arr->elesize );
    arr->size --;
    return arr;
}

void array_destroy(array *arr) {
    free(arr);
}

