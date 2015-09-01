#pragma once

#include "common.h"

typedef struct vector {
    int length, capacity;
    void** buffer;
} vector;

#define vector(t) vector
#define const_vector(t) const_vector

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

/**Allocate a vector of and fill it with elements.
   \param length is the number of vararg elements given.*/
static vector vectorInitChain (int length, stdalloc allocator, ...);

/**Join n vectors into a newly allocated vecto*/
static vector vectorsJoin (int n, stdalloc allocator, ...);

/**Duplicate a vector, but copy the elements by value*/
static vector vectorDup (vector v, stdalloc allocator);

/**A vector is null if it needs a call to vectorInit*/
static bool vectorNull (vector v);

/**Get an element, if n is in range*/
static void* vectorGet (vector v, int n);

/**Get the last element*/
static void* vectorTop (vector v);

/**Find an element, returning its index, or 0 if not found.*/
static int vectorFind (vector v, void* item);

/**Changes the capacity of the vector, chopping off any excess elements*/
static void vectorResize (vector* v, int size);

/**Add an item to the end of a vector. Returns the position.*/
static int vectorPush (vector* v, const void* item);

/**Add to the end of a vector from an array or vector. Doesn't modify the source.*/
static vector* vectorPushFromArray (vector* v, void** array, int length, size_t elementSize);
static vector* vectorPushFromVector (vector* dest, vector src);

static void* vectorPop (vector* v);

/**Remove an item from the list, moving the last element into its place.
   Return that element, or 0 if n is out of bounds.*/
static void* vectorRemoveReorder (vector* v, int n);

/**Attempt to set an index to a value. Return whether it failed.*/
static bool vectorSet (vector* v, int n, void* value);

#define for_vector_indexed(index, namedecl, vec, continuation)  \
    do {                                                        \
        vector for_vector_vec__ = (vec);                        \
        for (int (index) = 0;                                   \
             (index) < for_vector_vec__.length;                 \
             (index)++) {                                       \
            namedecl = for_vector_vec__.buffer[index];          \
            {continuation}                                      \
        }                                                       \
    } while (0);

#define for_vector(namedecl, vec, continuation)              \
    do {                                                     \
        vector for_vector_vec__ = (vec);                     \
        for (int n = 0; n < for_vector_vec__.length; n++) {  \
            namedecl = for_vector_vec__.buffer[n];           \
            {continuation}                                   \
        }                                                    \
    } while (0);

#define for_vector_reverse(namedecl, vec, continuation)  \
    do {                                                 \
        vector for_vector_vec__ = (vec);                 \
        for (int n = for_vector_vec__.length; n; n--) {  \
            namedecl = for_vector_vec__.buffer[n-1];     \
            {continuation}                               \
        }                                                \
    } while (0);

/**Maps dest[n] to f(src[n]) for n in min(dest->length, src->length).
   src and dest can match.
   @see vectorMapper*/
static void vectorMap (vector* dest, void* (*f)(void*), vector src);

static vector vectorMapInit (void* (*f)(void*), vector src, stdalloc allocator);

/*==== Inline implementations ====*/

#include "stdlib.h"
#include "string.h"

inline static vector vectorInit (int initialCapacity, stdalloc allocator) {
    if (initialCapacity == 0)
        initialCapacity++;

    return (vector) {
        .capacity = initialCapacity,
        .buffer = allocator(initialCapacity*sizeof(void*))
    };
}
#define array_len__(array) sizeof(array)/sizeof(*(array))

#define vectorInitFrom(array, allocator)                            \
    vectorPushFromArray(vectorInit(array_len__(array), allocator),  \
                        (array), array_len__(array), sizeof(*(array)))


inline static vector vectorInitChain (int length, stdalloc allocator, ...) {
    vector v = vectorInit(length, allocator);
    v.length = length;

    va_list args;
    va_start(args, allocator);

    for (int i = 0; i < length; i++)
        v.buffer[i] = va_arg(args, void*);

    va_end(args);

    return v;
}

static inline vector vectorsJoin (int n, stdalloc allocator, ...) {
    va_list args;

    /*Work out the length*/

    int length = 0;

    va_start(args, allocator);

    for (int i = 0; i < n; i++)
        length += va_arg(args, vector).length;

    va_end(args);

    /*Allocate and join*/

    vector v = vectorInit(length, allocator);

    va_start(args, allocator);

    for (int i = 0; i < n; i++)
        vectorPushFromVector(&v, va_arg(args, vector));

    va_end(args);

    return v;
}

inline static vector* vectorFree (vector* v) {
    free(v->buffer);
    v->length = 0;
    v->capacity = 0;
    v->buffer = 0;
    return v;
}

inline static vector* vectorFreeObjs (vector* v, vectorDtor dtor) {
    for_vector (void* item, *v, {
        dtor(item);
    })

    return vectorFree(v);
}

inline static vector vectorDup (vector v, stdalloc allocator) {
    vector dup = vectorInit(v.length, allocator);
    vectorPushFromVector(&dup, v);
    return dup;
}

inline static bool vectorNull (vector v) {
    return v.buffer == 0;
}

inline static void* vectorGet (vector v, int n) {
    if (n < v.length && n >= 0)
        return v.buffer[n];

    else
        return 0;
}

inline static void* vectorTop (vector v) {
    return v.length > 0 ? v.buffer[v.length-1] : 0;
}

inline static int vectorFind (vector v, void* item) {
    for (int i = 0; i < v.length; i++) {
        if (v.buffer[i] == item)
            return i;
    }

    return -1;
}

inline static void vectorResize (vector* v, int size) {
	if (size > v->capacity)
        v->buffer = realloc(v->buffer, size*sizeof(void*));

    v->capacity = size;

    if (v->capacity < v->length)
        v->length = size;
}

inline static int vectorPush (vector* v, const void* item) {
    if (v->length == v->capacity)
        vectorResize(v, v->capacity*2);

    v->buffer[v->length] = (void*) item;
    return v->length++;
}

inline static vector* vectorPushFromArray (vector* v, void** array, int length, size_t elementSize) {
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

inline static vector* vectorPushFromVector (vector* dest, vector src) {
    return vectorPushFromArray(dest, src.buffer, src.length, sizeof(void*));
}

inline static void* vectorPop (vector* v) {
    if (v->length >= 1)
        return v->buffer[--v->length];

    else
        return 0;
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

inline static void vectorMap (vector* dest, vectorMapper f, vector src) {
    int upto = src.length > dest->capacity ? dest->capacity : src.length;

    for (int n = 0; n < upto; n++)
        dest->buffer[n] = f(src.buffer[n]);

    dest->length = upto;
}

inline static vector vectorMapInit (void* (*f)(void*), vector src, stdalloc allocator) {
    vector result = vectorInit(src.length, allocator);

    for (int n = 0; n < src.length; n++)
        vectorPush(&result, f(src.buffer[n]));

    return result;
}
