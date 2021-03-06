// MIT License, Copyright (c) 2020 Marvin Borner

#ifndef MEM_H
#define MEM_H

#include <def.h>

int malloc_allocated;

void *malloc_debug(u32 size, const char *file, int line, const char *func, const char *inp);
void free_debug(void *ptr, const char *file, int line, const char *func, const char *inp);
#define malloc(size) malloc_debug(size, __FILE__, __LINE__, __func__, #size)
#define free(ptr) free_debug(ptr, __FILE__, __LINE__, __func__, #ptr)
void *realloc(void *ptr, u32 size);
void *zalloc(u32 size);

#ifdef kernel
void heap_init(u32 start);
#elif defined(userspace)
#else
#error "No lib target specified. Please use -Dkernel or -Duserspace"
#endif

void *memcpy(void *dest, const void *src, u32 n);
void *memset(void *dest, int val, u32 n);
void *memchr(void *src, int c, u32 n);
int memcmp(const void *s1, const void *s2, u32 n);
int mememp(const u8 *buf, u32 n);

#endif
