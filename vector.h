#pragma once

#include "common.h"

typedef struct vector {
    int length, capacity;
    void** buffer;
} vector;

#define vector(t) vector

///For use with vectorFreeObjs
typedef void (*vectorDtor)(void*);
///For use with vectorFor
typedef void (*vectorIter)(void*);
///For use with vectorMap
typedef void* (*vectorMapper)(void*);

static vector vectorInit (int initialCapacity, stdalloc allocator);

/**Clean up resources allocated by the vector but not the vector itself.
   This is safe to call on a null (all zero, uninitialized) vector*/
static vector* vectorFree (vector* v);

/**As with vectorFree, but also call a given destructor on each contained element @see vectorDtor*/
static vector* vectorFreeObjs (vector* v, void (*dtor)(void*));

/**Get an element, if n is in range*/
static void* vectorGet (vector v, int n);

/**Find an element, returning its index, or 0 if not found.*/
static int vectorFind (vector v, void* item);

static void vectorResize (vector* v, int size);

/**Add an item to the end of a vector. Returns the position.*/
static int vectorPush (vector* v, void* item);

/**Add to the end of a vector from an array or vector. Doesn't modify the source.*/
static vector* vectorPushFromArray (vector* v, void** array, int length, int elementSize);
static vector* vectorPushFromVector (vector* dest, const vector* src);

static void* vectorPop (vector* v);

/**Remove an item from the list, moving the last element into its place.
   Return that element, or 0 if n is out of bounds.*/
static void* vectorRemoveReorder (vector* v, int n);

/**Attempt to set an index to a value. Return whether it failed.*/
static bool vectorSet (vector* v, int n, void* value);

static void vectorFor (const vector* v, vectorIter f);

/**Maps dest[n] to f(src[n]) for n in min(dest->length, src->length).
   src and dest can match.
   @see vectorMapper*/
static void vectorMap (vector* dest, void* (*f)(void*), const vector* src);

/*==== Inline implementations ====*/

#include "stdlib.h"
#include "string.h"

inline static vector vectorInit (int initialCapacity, stdalloc allocator) {
    return (vector) {
        .capacity = initialCapacity,
        .buffer = allocator(initialCapacity*sizeof(void*))
    };
}

inline static vector* vectorFree (vector* v) {
    free(v->buffer);
    v->length = 0;
    v->capacity = 0;
    v->buffer = 0;
    return v;
}

inline static vector* vectorFreeObjs (vector* v, vectorDtor dtor) {
    /*This will mess up the vector, watevs*/
    vectorMap(v, (vectorMapper) dtor, v);
    return vectorFree(v);
}

inline static void* vectorGet (vector v, int n) {
    if (n < v.length && n >= 0)
        return v.buffer[n];

    else
        return 0;
}

inline static int vectorFind (vector v, void* item) {
    for (int i = 0; i < v.length; i++) {
        if (v.buffer[i] == item)
            return i;
    }

    return -1;
}

inline static void vectorResize (vector* v, int size) {
    v->capacity = size;
    v->buffer = realloc(v->buffer, v->capacity*sizeof(void*));

    if (v->capacity < v->length)
        v->length = size;
}

inline static int vectorPush (vector* v, void* item) {
    if (v->length == v->capacity)
        vectorResize(v, v->capacity*2);

    v->buffer[v->length] = item;
    return v->length++;
}

inline static vector* vectorPushFromArray (vector* v, void** array, int length, int elementSize) {
    /*Make sure it is at least big enough*/
    if (v->capacity < v->length + length)
        vectorResize(v, v->capacity + length*2);

    /*If the src and dest element size match, memcpy*/
    if (elementSize == sizeof(void*))
        memcpy(v->buffer+v->length, array, length*elementSize);

    /*Otherwise copy each element individually*/
    else
        for (int i = 0; i < length; i++)
            memcpy(v->buffer+i, (char*) array + i*elementSize, elementSize);

    v->length += length;
    return v;
}

inline static vector* vectorPushFromVector (vector* dest, const vector* src) {
    return vectorPushFromArray(dest, src->buffer, src->length, sizeof(void*));
}

inline static void* vectorPop (vector* v) {
    return v->buffer[--v->length];
}

inline static void* vectorRemoveReorder (vector* v, int n) {
    if (v->length <= n)
        return 0;

    void* last = vectorPop(v);
    vectorSet(v, n, last);
    return last;
}

inline static bool vectorSet (vector* v, int n, void* value) {
    if (n < v->length) {
        v->buffer[n] = value;
        return false;

    } else
        return true;
}

inline static void vectorFor (const vector* v, vectorIter f) {
    for (int n = 0; n < v->length; n++)
        f(v->buffer[n]);
}

inline static void vectorMap (vector* dest, vectorMapper f, const vector* src) {
    int upto = src->length > dest->capacity ? dest->capacity : src->length;

    for (int n = 0; n < upto; n++)
        dest->buffer[n] = f(src->buffer[n]);

    dest->length = upto;
}
