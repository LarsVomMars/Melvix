#include <stddef.h>

size_t strlen(const char *str) {
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

size_t strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *) s1 - *(const unsigned char *) s2;
}

char *strcat(char *dst, const char *src) {
    unsigned int i = 0;
    unsigned int j = 0;
    for (i = 0; dst[i] != 0; i++) {}
    for (j = 0; src[j] != 0; j++) {
        dst[i + j] = src[j];
    }
    dst[i + j] = 0;
    return dst;
}
