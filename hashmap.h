#pragma once

#include "common.h"

#include <stdint.h>

/**
 * This header provides 4 different data structures.
 *
 * Abstractly, a map translates unique keys to arbitrary values. A set contains
 * unique elements, that is to say any possible member either belongs to it or
 * doesn't. Neither has any concept of an ordering.
 *
 * Details of concrete implementations:
 *  - hashmap is a map from null terminated (char*) strings to void* values.
 *    It is the most general of these structures.
 *  - intmap is a map from intptr_t integers to void* values.
 *  - hashset is a set of strings.
 *  - intset is a set of intptr_t integers.
 *
 * They are implemented by the same algorithm and struct, generalmap. Use the
 * specific interfaces, and avoid using the struct fields directly.
 *
 * Important notes on pointer ownership / cleanup:
 *  - XXXFree does not free the map/set given to it.
 *  - hashXXXFree does not free the string keys given to it.
 *  - XXXmapFree does not free the void* values given to it.
 *  - XXXFreeObjs can be used to explicitly free these by providing destructor(s).
 * and
 *  - hashXXXMerge uses the same string keys in the destination as in the source.
 *  - Use hashXXXMergeDup to instead make a copy of each added to the dest.
 *  - Both will use the same void* value.
 */

/**
 * An efficient data structure for mapping from null terminated keys
 * to void* values.
 */
typedef struct generalmap {
    int size, elements;
    union {
        const char** keysStr;
        /*We don't know whether the user intends the map to take ownership of
          keys (freed with FreeObjs), so provide a mutable version*/
        char** keysStrMutable;
        intptr_t* keysInt;
    };
    int* hashes;
    void** values;
} generalmap;

typedef generalmap hashmap;
typedef generalmap intmap;

typedef generalmap hashset;
typedef generalmap intset;

#define hashmap(v) hashmap
#define intmap(k, v) intmap
#define intset(k) intset

static bool mapNull (generalmap map);

/*==== hashmap ====*/

typedef void (*hashmapKeyDtor)(char* key, const void* value);
typedef void (*hashmapValueDtor)(void* value);

static hashmap hashmapInit (int size, calloc_t calloc);

static hashmap* hashmapFree (hashmap* map);
static hashmap* hashmapFreeObjs (hashmap* map, hashmapKeyDtor keyDtor, hashmapValueDtor valueDtor);

static bool hashmapAdd (hashmap* map, const char* key, void* value);
static void hashmapMerge (hashmap* dest, hashmap* src);
static void hashmapMergeDup (hashmap* dest, const hashmap* src);

static void* hashmapMap (const hashmap* map, const char* key);

/*==== intmap ====*/

typedef void (*intmapValueDtor)(void* value, int key);

static intmap intmapInit (int size, calloc_t calloc);

static intmap* intmapFree (intmap* map);
static intmap* intmapFreeObjs (intmap* map, intmapValueDtor dtor);

static bool intmapAdd (intmap* map, intptr_t element, void* value);
static void intmapMerge (intmap* dest, const intmap* src);

static void* intmapMap (const intmap* map, intptr_t element);

/*==== hashset ====*/

typedef void (*hashsetDtor)(char* element);

static hashset hashsetInit (int size, calloc_t calloc);

static hashset* hashsetFree (hashset* set);
static hashset* hashsetFreeObjs (hashset* set, hashsetDtor dtor);

static bool hashsetAdd (hashset* set, const char* element);
static void hashsetMerge (hashset* dest, hashset* src);
static void hashsetMergeDup (hashset* dest, const hashset* src);

static bool hashsetTest (const hashset* set, const char* element);

/*==== intset ====*/

static intset intsetInit (int size, calloc_t calloc);
static intset* intsetFree (intset* set);

static bool intsetAdd (intset* set, intptr_t element);
static void intsetMerge (intset* dest, const intset* src);

static bool intsetTest (const intset* set, intptr_t element);

/*==== Inline implementations ====*/

#include "stdlib.h"
#include "string.h"
#include "assert.h"

static inline bool mapNull (generalmap map) {
    return map.elements == 0;
}

static intptr_t hashstr (const char* key, int mapsize);
static intptr_t hashint (intptr_t element, int mapsize);

typedef void (*generalmapKeyDtor)(char* key, const void* value);
typedef void (*generalmapValueDtor)(void* value);
typedef intptr_t (*generalmapHash)(const char* key, int mapsize);
//Like strcmp, returns 0 for match
typedef int (*generalmapCmp)(const char* actual, const char* key);
typedef char* (*generalmapDup)(const char* key);

static generalmap generalmapInit (int size, calloc_t calloc, bool hashes);

static generalmap* generalmapFree (generalmap* map, bool hashes);
static generalmap* generalmapFreeObjs (generalmap* map, generalmapKeyDtor keyDtor, generalmapValueDtor valueDtor,
                                bool hashes);

static bool generalmapAdd (generalmap* map, const char* key, void* value,
                           generalmapHash hashf, generalmapCmp cmp, bool values);
static void generalmapMerge (generalmap* dest, const generalmap* src,
                             generalmapHash hash, generalmapCmp cmp, generalmapDup dup, bool values);

static void* generalmapMap (const generalmap* map, const char* key, generalmapHash hashf, generalmapCmp cmp);
static bool generalmapTest (const generalmap* map, const char* key, generalmapHash hashf, generalmapCmp cmp);

/*==== Hash functions ====*/

static inline intptr_t hashstr (const char* key, int mapsize) {
    /*Jenkin's One-at-a-Time Hash
      Taken from http://www.burtleburtle.net/bob/hash/doobs.html
      Public domain*/

    intptr_t hash = 0;

    for (int i = 0; key[i]; i++) {
        hash += key[i];
        hash += hash << 10;
        hash ^= hash >> 6;
    }

    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;

    /*Assumes mapsize is a power of two*/
    intptr_t mask = mapsize-1;
    return hash & mask;
}

static inline intptr_t hashint (intptr_t element, int mapsize) {
    /*The above, for a single value*/

    intptr_t hash = element;
    hash += hash << 10;
    hash ^= hash >> 6;
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;

    intptr_t mask = mapsize-1;
    return hash & mask;
}

/*==== generalmap ====*/

static bool generalmapIsMatch (const generalmap* map, int index, const char* key, int hash, generalmapCmp cmp);

/*Slightly counterintuitively, this doesn't actually find the location
  of the key. It finds the location where it stopped looking, which
  will be:
    1. The key
    2. The first zero after its expected position
    3. Another key, indicating that it was not found and that the
       buffer is full
  It is guaranteed to be a valid index in the buffer.*/
static int generalmapFind (const generalmap* map, const char* key,
                           int hash, generalmapCmp cmp);

static inline int pow2ize (int x) {
    assert(sizeof(x) <= 8);
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    /*No-op if x is less than 33 bits
      Shift twice to avoid warnings, as this is intended.*/
    x |= (x >> 16) >> 16;
    return x+1;
}

static inline generalmap generalmapInit (int size, calloc_t calloc, bool hashes) {
    /*The hash requires that the size is a power of two*/
    size = pow2ize(size);

    return (generalmap) {
        .size = size,
        .elements = 0,
        .keysInt = calloc(size, sizeof(intptr_t)),
        .hashes = hashes ? calloc(size, sizeof(int)) : 0,
        .values = calloc(size, sizeof(void*))
    };
}

static inline generalmap* generalmapFree (generalmap* map, bool hashes) {
    free(map->keysInt);

    if (hashes)
        free(map->hashes);

    free(map->values);

    map->keysInt = 0;
    map->hashes = 0;
    map->values = 0;
    return map;
}

static inline generalmap* generalmapFreeObjs (generalmap* map, generalmapKeyDtor keyDtor, generalmapValueDtor valueDtor,
                                              bool hashes) {
    /*Until the end of the buffer*/
    for (int index = 0; index < map->size; index++) {
        /*Skip empties*/
        if (map->values[index] == 0)
            continue;

        /*Call the dtor*/

        if (keyDtor)
            keyDtor(map->keysStrMutable[index], map->values[index]);

        if (valueDtor)
            valueDtor(map->values[index]);
    }

    return generalmapFree(map, hashes);
}

static inline bool generalmapIsMatch (const generalmap* map, int index, const char* key, int hash, generalmapCmp cmp) {
    if (cmp)
        return    map->hashes[index] == hash
               && !cmp(map->keysStr[index], key);

    else
        return map->keysStr[index] == key;
}

static inline int generalmapFind (const generalmap* map, const char* key,
                                  int hash, generalmapCmp cmp) {
    /*Check from the first choice slot all the way until the end of
      the values buffer, if need be*/
    for (int index = hash; index < map->size; index++)
        /*Exit if empty or found the key*/
        if (map->values[index] == 0 || generalmapIsMatch(map, index, key, hash, cmp))
            return index;

    /*Then from the start all the way back to the first choice slot*/
    for (int index = 0; index < hash; index++)
        if (map->values[index] == 0 || generalmapIsMatch(map, index, key, hash, cmp))
            return index;

    /*Done an entire sweep of the buffer, not found & full*/
    return hash;
}

static inline bool generalmapAdd (generalmap* map, const char* key, void* value,
                                  generalmapHash hashf, generalmapCmp cmp, bool values) {
    /*Half full: create a new one twice the size and copy elements over.
      Allows us to assume there is space for the key.*/
    if (map->elements*2 + 1 >= map->size) {
        generalmap newmap = generalmapInit(map->size*2, calloc, cmp != 0);
        generalmapMerge(&newmap, map, hashf, cmp, 0, values);
        generalmapFree(map, cmp != 0);
        *map = newmap;
    }

    int hash = hashf(key, map->size);
    int index = generalmapFind(map, key, hash, cmp);

    bool present;

    /*Empty spot*/
    if (map->values[index] == 0) {
        map->keysStr[index] = key;
        map->elements++;
        present = false;

        if (cmp != 0)
            map->hashes[index] = hash;

    /*Present, remap*/
    } else {
        assert(generalmapIsMatch(map, index, key, hash, cmp));
        present = true;
    }

    map->values[index] = values ? value : (void*) true;

    return present;
}

static inline void generalmapMerge (generalmap* dest, const generalmap* src,
                                    generalmapHash hash, generalmapCmp cmp, generalmapDup dup, bool values) {
    for (int index = 0; index < src->size; index++) {
        if (src->keysInt[index] == 0)
            continue;

        char* key = src->keysStrMutable[index];

        if (dup)
            key = dup(key);

        generalmapAdd(dest, key, values ? src->values[index] : 0, hash, cmp, values);
    }
}

static inline void* generalmapMap (const generalmap* map, const char* key, generalmapHash hashf, generalmapCmp cmp) {
    int hash = hashf(key, map->size);
    int index = generalmapFind(map, key, hash, cmp);
    return generalmapIsMatch(map, index, key, hash, cmp) ? map->values[index] : 0;
}

static inline bool generalmapTest (const generalmap* map, const char* key, generalmapHash hashf, generalmapCmp cmp) {
    int hash = hashf(key, map->size);
    int index = generalmapFind(map, key, hash, cmp);
    return generalmapIsMatch(map, index, key, hash, cmp);
}

/*==== HASHMAP ====*/

static inline hashmap hashmapInit (int size, calloc_t calloc) {
    return generalmapInit(size, calloc, true);
}

static inline hashmap* hashmapFree (hashmap* map) {
    return generalmapFree(map, true);
}

static inline hashmap* hashmapFreeObjs (hashmap* map, hashmapKeyDtor keyDtor, hashmapValueDtor valueDtor) {
    return generalmapFreeObjs(map, keyDtor, valueDtor, true);
}

static inline bool hashmapAdd (hashmap* map, const char* key, void* value) {
    return generalmapAdd(map, key, value, hashstr, strcmp, true);
}

static inline void hashmapMerge (hashmap* dest, hashmap* src) {
    generalmapMerge(dest, src, hashstr, strcmp, 0, true);
}

static inline void hashmapMergeDup (hashmap* dest, const hashmap* src) {
    generalmapMerge(dest, src, hashstr, strcmp, strdup, true);
}

static inline void* hashmapMap (const hashmap* map, const char* key) {
    return generalmapMap(map, key, hashstr, strcmp);
}

/*==== intmap ====*/

static inline intmap intmapInit (int size, calloc_t calloc) {
    return generalmapInit(size, calloc, false);
}

static inline intmap* intmapFree (intmap* map) {
    return generalmapFree(map, false);
}

static inline intmap* intmapFreeObjs (intmap* map, intmapValueDtor dtor) {
    return generalmapFreeObjs(map, 0, (generalmapValueDtor) dtor, false);
}

static inline bool intmapAdd (intmap* map, intptr_t element, void* value) {
    return generalmapAdd(map, (void*) element, value, (generalmapHash) hashint, 0, true);
}

static inline void intmapMerge (intmap* dest, const intmap* src) {
    generalmapMerge(dest, src, (generalmapHash) hashint, 0, 0, true);
}

static inline void* intmapMap (const intmap* map, intptr_t element) {
    return generalmapMap(map, (void*) element, (generalmapHash) hashint, 0);
}

/*==== hashset ====*/

static inline hashset hashsetInit (int size, calloc_t calloc) {
    return generalmapInit(size, calloc, true);
}

static inline hashset* hashsetFree (hashset* set) {
    return generalmapFree(set, true);
}

static inline hashset* hashsetFreeObjs (hashset* set, hashsetDtor dtor) {
    return generalmapFreeObjs(set, (generalmapKeyDtor) dtor, 0, true);
}

static inline bool hashsetAdd (hashset* set, const char* element) {
    return generalmapAdd(set, element, 0, hashstr, strcmp, false);
}

static inline void hashsetMerge (hashset* dest, hashset* src) {
    generalmapMerge(dest, src, hashstr, strcmp, 0, false);
}

static inline void hashsetMergeDup (hashset* dest, const hashset* src) {
    generalmapMerge(dest, src, hashstr, strcmp, strdup, false);
}

static inline bool hashsetTest (const hashset* set, const char* element) {
    return generalmapTest(set, element, hashstr, strcmp);
}

/*==== intset ====*/

static inline intset intsetInit (int size, calloc_t calloc) {
    return generalmapInit(size, calloc, false);
}

static inline intset* intsetFree (intset* set) {
    return generalmapFree(set, false);
}

static inline bool intsetAdd (intset* set, intptr_t element) {
    return generalmapAdd(set, (void*) element, 0, (generalmapHash) hashint, 0, false);
}

static inline void intsetMerge (intset* dest, const intset* src) {
    generalmapMerge(dest, src, (generalmapHash) hashint, 0, 0, false);
}

static inline bool intsetTest (const intset* set, intptr_t element) {
    return generalmapTest(set, (void*) element, (generalmapHash) hashint, 0);
}
