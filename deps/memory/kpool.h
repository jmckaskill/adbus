#pragma once

#include <stdlib.h>

typedef struct kpool_t kpool_t;

void* kp_alloc(kpool_t* p, size_t size);
void* kp_calloc(kpool_t* p, size_t size);

kpool_t* kp_new(kpool_t* parent);
void kp_clear(kpool_t* p);
void kp_free(kpool_t* p);

typedef void (*kp_free_function_t)(void*);
void kp_register(kpool_t* p, kp_free_function_t func, void* data);

#define NEW_ARRAY(TYPE, NUM) ((TYPE*) calloc(NUM, sizeof(TYPE)))
#define NEW(TYPE) ((TYPE*) calloc(1, sizeof(TYPE)))
#define PNEW(POOL, TYPE) ((TYPE*) kp_calloc(POOL, sizeof(TYPE)))
