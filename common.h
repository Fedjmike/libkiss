#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#define swap(x, y)                       \
    do {__typeof__(x) __swap_tmp = (x);  \
        (x) = (y); (y) = __swap_tmp;} while (0)

#define for_n_args(length, decl, lastparam, continuation)   \
    va_list __for_args; va_start(__for_args, (lastparam));  \
    for (int __for_i = 0; __for_i < (length); __for_i++) {  \
        decl = va_arg(__for_args, void*);                   \
        {continuation}                                      \
    } va_end(__for_args);

typedef void* (*malloc_t)(size_t);
typedef void* (*calloc_t)(size_t, size_t);
typedef void (*free_t)(void*);
typedef void* (*realloc_t)(void*, size_t);
typedef char* (*strdup_t)(const char*);

typedef struct alloc_t {
    malloc_t malloc;
    calloc_t clear;
    free_t free;
    realloc_t realloc;
    strdup_t strdup;
} alloc_t;

#define stdalloc ((alloc_t) {malloc, clear, free, realloc, strdup})

static inline void* malloci (size_t size, const void* src) {
    void* obj = malloc(size);
    memcpy(obj, src, size);
    return obj;
}

static inline void* alloci (size_t size, const void* src, malloc_t malloc) {
    void* obj = malloc(size);
    memcpy(obj, src, size);
    return obj;
}

static inline intmax_t logi (intmax_t x, int base) {
    int n = 0;

    while (x >= base) {
        x /= base;
        n++;
    }

    return n;
}

static inline int intdiv_roundup (int dividend, int divisor) {
    return (dividend - 1) / divisor + 1;
}

static inline int intlen (intmax_t number) {
    return logi(number, 10) + 1;
}

static inline int dryprintf (const char* format, ...) {
    static FILE* nullfile = 0;

    //todo: is this slow?
    if (!nullfile)
        nullfile = fopen("/dev/null", "w");

    va_list args;
    va_start(args, format);
    int width = vfprintf(nullfile, format, args);
    va_end(args);

    return width;
}

static inline void putnchar (int character, int times) {
    for (int i = 0; i < times; i++)
        putchar(character);
}

#ifndef _XOPEN_SOURCE

static inline char* strdup (const char* src) {
    return strcpy(malloc(strlen(src)+1), src);
}

#endif

static inline size_t strwidth (const char* str) {
    //todo unicode
    return strlen(str);
}

static inline char* strnchr (int n, const char* str, char character) {
    //todo optimize

    for (int i = 0; i < n && str[i]; i++)
        if (str[i] == character)
            return (char*) str + i;

    return 0;
}

static inline int strnchrcount (int n, char* str, char character) {
    str = strnchr(n, str, character);

    int count = 0;

    for (; str; count++)
        str = strnchr(n, str+1, character);

    return count;
}

static inline size_t strcatwith (char* buffer, size_t n, char** strs, const char* separator) {
    size_t pos = 0;

    /*Copy all but the last, with sep*/
    for (size_t i = 0; i < n-1; i++)
        pos += sprintf(buffer+pos, "%s%s", strs[i], separator);

    pos += sprintf(buffer+pos, "%s", strs[n-1]);

    return pos;
}

static inline char* strjoinwith (size_t n, char** strs, const char* separator, malloc_t malloc) {
    if (n <= 0) {
        char* str = malloc(1);
        *str = 0;
        return str;
    }

    /*Work out the length*/

    size_t length = 1;
    size_t seplength = strlen(separator);
    length += seplength * (n-1);

    for (size_t i = 0; i < n; i++)
        length += strlen(strs[i]);

    /*Make the string*/

    char* str = malloc(length);
    strcatwith(str, n, strs, separator);

    return str;
}

static inline char* strjoin (int n, char** strs, malloc_t malloc) {
    return strjoinwith(n, strs, "", malloc);
}

/*strcat, resizing the buffer if need be*/
static inline char* strrecat (char* dest, size_t* size, const char* src) {
    size_t destlength = strlen(dest);
    size_t length = destlength + strlen(src) + 1;

    if (length > *size) {
        *size = length*2;
        dest = realloc(dest, *size);
    }

    return strcat(dest+destlength, src);
}

static inline int qsort_cstr (const void* left, const void* right) {
    return strcmp(*(char**) left, *(char**) right);
}

static inline char* readall (FILE* file, alloc_t alloc) {
    size_t bufsize = 512;
    char* buffer = alloc.malloc(bufsize);
    size_t pos = 0;

    for (;;) {
        size_t space = bufsize-pos;
        size_t bytes = fread(buffer+pos, 1, space, file);
        pos += bytes;

        if (bytes < space)
            break;

        else
            buffer = alloc.realloc(buffer, bufsize *= 2);
    }

    if (pos >= bufsize)
        buffer = alloc.realloc(buffer, bufsize += 1);

    buffer[pos-1] = 0;
    return buffer;
}
