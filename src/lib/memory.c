#include "../graphics/graphics.h"

void *memory_copy(void *dest, const void *src, size_t count) {
    const char *sp = (const char *) src;
    char *dp = (char *) dest;
    for (; count != 0; count--) *dp++ = *sp++;
    return dest;
}

void *memory_set(void *dest, char val, size_t count) {
    char *temp = (char *) dest;
    for (; count != 0; count--) *temp++ = val;
    return dest;
}

int memory_compare(const void *a_ptr, const void *b_ptr, size_t size) {
    const unsigned char *a = (const unsigned char *) a_ptr;
    const unsigned char *b = (const unsigned char *) b_ptr;
    for (size_t i = 0; i < size; i++) {
        if (a[i] < b[i])
            return -1;
        else if (b[i] < a[i])
            return 1;
    }
    return 0;
}