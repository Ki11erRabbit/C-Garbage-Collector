#ifndef GARBAGECOLLECTOR_GC_H
#define GARBAGECOLLECTOR_GC_H

#include <pthread.h>
#include <stdlib.h>
#include <setjmp.h>
#include "map.h"
typedef struct Pointer {
    void *ptr;
    char marked;
    char size;
} Pointer;

typedef map_t(struct Pointer) map_pointer_t;
static map_pointer_t map;

static void *stackPointer = NULL;
static void *oldStackPointer = NULL;
static void *minptr = NULL;
static void *maxptr = NULL;
static int flipped = 0;
static int run_gc = 0;
static int ended = 0;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int rc_main(int argc, char** argv);

#define main(...) \
  main(int argc, char** argv) { \
    setupHeap(); \
    long stk = (long)NULL;            \
    pthread_t gcThread;         \
    pthread_create(&gcThread,NULL,garbageCollector,(void *)&stk);\
    int return_val = rc_main(argc, argv);                        \
    signal_end();              \
    pthread_join(gcThread,NULL);              \
    return return_val;\
  }; \
  int rc_main(int argc, char** argv)

void *garbageCollector(void *stackEnd);
void signal_end();
void updateStackPointer();
void setupHeap();
void gc_mark_item(void *a);

void *allocate(size_t size);
#define malloc(size) allocate(size)
void *reallocate(void *ptr, size_t size);
#define realloc(ptr, size) reallocate(ptr, size)
void *continuousAllocate(size_t num, size_t size);
#define calloc(num, size) continuousAllocate(num, size)
void *deallocate(void *ptr);
#define free(ptr) deallocate(ptr)
void addReference(void *ptr, size_t size);
void removeReference(void *ptr);

#endif //GARBAGECOLLECTOR_GC_H
