#include "gc.h"

#include <stdio.h>

#undef malloc
#undef realloc
#undef calloc
#undef free

void *allocate(size_t size) {

    updateStackPointer();

    void *ptr = malloc(size);
    if (ptr == NULL) {
        run_gc = 1;
        ptr = malloc(size);
    }

    char key[20];
    sprintf(key, "%p", ptr);
    char marked = 0;
    if (flipped) {
        marked = 1;
    }
    Pointer pointer = {ptr, marked, size};
    map_set(map, key, pointer);

    if (ptr > maxptr) {
        maxptr = ptr;
    }

    return ptr;
}
void *reallocate(void *ptr, size_t size) {
    updateStackPointer();

    void *newptr = realloc(ptr, size);
    if (newptr == NULL) {
        run_gc = 1;
        newptr = realloc(ptr, size);
    }

    char key[20];
    sprintf(key, "%p", ptr);
    map_remove(map, key);

    sprintf(key, "%p", newptr);
    char marked = 0;
    if (flipped) {
        marked = 1;
    }
    Pointer pointer = {newptr, marked, size};
    map_set(map, key, pointer);

    if (ptr > maxptr) {
        maxptr = ptr;
    }

    return newptr;
}

void *continuousAllocate(size_t num, size_t size) {
    updateStackPointer();

    void *ptr = calloc(num, size);
    if (ptr == NULL) {
        run_gc = 1;
        ptr = calloc(num, size);
    }

    char key[20];
    sprintf(key, "%p", ptr);
    char marked = 0;
    if (flipped) {
        marked = 1;
    }
    Pointer pointer = {ptr, marked, size};
    map_set(map, key, pointer);

    if (ptr > maxptr) {
        maxptr = ptr;
    }

    return ptr;
}

void *deallocate(void *ptr) {
    updateStackPointer();

    char key[20];
    sprintf(key, "%p", ptr);
    map_remove(map, key);

    free(ptr);
}

void addReference(void *ptr, size_t size) {
    updateStackPointer();

    char key[20];
    sprintf(key, "%p", ptr);
    char marked = 0;
    if (flipped) {
        marked = 1;
    }
    Pointer pointer = {ptr, marked, size};
    map_set(map, key, pointer);
}
void removeReference(void *ptr) {
    updateStackPointer();

    char key[20];
    sprintf(key, "%p", ptr);
    map_remove(map, key);
}

void signal_end() {
    ended = 1;
}

void setupHeap() {
    long *pointer = malloc(sizeof(long));
    long *pointer2 = malloc(sizeof(long));
    minptr = pointer;
    maxptr = pointer2;

    free(pointer);
    free(pointer2);
}

static unsigned long long getrsp() {
    __asm__ ("movq %rsp, %rax");
}
static void mark_stack(void) {
#ifdef __amd64__
    oldStackPointer = stackPointer;
    stackPointer = (void*)getrsp();
#else
    long stackTop = (long) NULL;
    oldStackPointer = stackPointer;
    stackPointer = &stackTop;
#endif
}

void updateStackPointer() {
    jmp_buf env;
    memset(&env, 0, sizeof(jmp_buf));
    setjmp(env);

    volatile int noinline = 1;
    void (*stack_mark)(void) = noinline
                               ? mark_stack
                               : (void(*)(void))(NULL);

    stack_mark();
}

static int gc_allocated(void *a) {
    char key[20];
    sprintf(key,"%p", a);
    void *value = map_get(map,key);
    if (value == NULL) {
        return 0;
    }
    return 1;
}

static int gc_isMarked(void *a) {
    char key[20];
    sprintf(key,"%p", a);
    Pointer *value = map_get(map,key);
    if (value->marked == 1) {
        return 1;
    }
    else {
        return 0;
    }
}

static void gc_mark(void *a) {
    char key[20];
    sprintf(key,"%p", a);
    Pointer *value = map_get(map,key);
    if (flipped) {
        value->marked = 0;
    }
    else {
        value->marked = 1;
    }
}

static void gc_recurse(void *ptr) {
    size_t *pointer = (size_t *) ptr;
    if (*pointer == 0) {
        return;
    }

    char key[20];
    sprintf(key,"%p", ptr);
    Pointer *value = map_get(map,key);
    for (size_t i = 0; i < value->size * sizeof(void*); i++) {
        size_t *p = (char*)ptr + i;
        gc_mark_item((void*)*p);
    }

}

void gc_mark_item(void *a){

    if ((size_t)a % sizeof(void*) == 0 && a >= minptr && a <= maxptr && gc_allocated(a) && !gc_isMarked(a)) {
        gc_mark(a);
        gc_recurse(a);
    }
}



static void mark(void *stackStart) {
    char *top = (char *) stackPointer;
    char *bottom = (char *) stackStart;

    for (char *p = top; p <= bottom; p += sizeof(void *)) {
        gc_mark_item(*p);
    }
}

static void sweep() {
    const char *key;
    map_iter_t iter = map_iter(map);

    while ((key = map_next(map, &iter))) {
        Pointer *value = map_get(map,key);
        if (!gc_isMarked(value->ptr)) {
            map_remove(map,key);
            free(value->ptr);
        }
    }
}

void *garbageCollector(void *ptr) {
    map_init(map);
    void *stackStart = NULL;
    stackStart = ptr;

    while (!ended) {

        if (oldStackPointer > stackPointer || run_gc) {
            pthread_mutex_lock(&lock);
            mark(stackStart);

            sweep();
            pthread_mutex_unlock(&lock);
            run_gc = 0;
            oldStackPointer = stackPointer;
        }
    }


    const char *key;
    map_iter_t iter = map_iter(map);

    while ((key = map_next(map, &iter))) {
        free(map_get(map,key)->ptr);
    }

    map_deinit(map);
}