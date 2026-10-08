#include <klib.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
    size_t len = 0;
  while (s[len] != '\0') len++;
  return len;
}

char *strcpy(char *dst, const char *src) {
  char *result = dst;
  while ((*dst++ = *src++) != '\0') {
  }
  return result;
}


char *strncpy(char *dst, const char *src, size_t n) {
 char *result = dst;
  size_t i = 0;

  while (i < n && src[i] != '\0') {
    dst[i] = src[i];
    i++;
  }
  while (i < n) {
    dst[i++] = '\0';
  }

  return result;
}

char *strcat(char *dst, const char *src) {
  char *result = dst;
  while (*dst != '\0') dst++;
  while ((*dst++ = *src++) != '\0') {
  }
  return result;
}

int strcmp(const char *s1, const char *s2) {
  while (*s1 != '\0' && *s1 == *s2) {
    s1++;
    s2++;
  }
  return (unsigned char)*s1 - (unsigned char)*s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
 for (size_t i = 0; i < n; i++) {
    unsigned char a = (unsigned char)s1[i];
    unsigned char b = (unsigned char)s2[i];
    if (a != b || a == '\0') return (int)a - (int)b;
  }
  return 0;
}

void *memset(void *s, int c, size_t n) {
  unsigned char *p = s;
  for (size_t i = 0; i < n; i++) {
    p[i] = (unsigned char)c;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  unsigned char *d = dst;
  const unsigned char *s = src;

  if ((uintptr_t)d > (uintptr_t)s) {
    while (n > 0) {
      n--;
      d[n] = s[n];
    }
  } else {
    for (size_t i = 0; i < n; i++) {
      d[i] = s[i];
    }
  }
  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
   unsigned char *d = out;
  const unsigned char *s = in;
  for (size_t i = 0; i < n; i++) {
    d[i] = s[i];
  }
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char *a = s1;
  const unsigned char *b = s2;
  for (size_t i = 0; i < n; i++) {
    if (a[i] != b[i]) return (int)a[i] - (int)b[i];
  }
  return 0;
}

char *strchr(const char *s, int c) {
  do {
    if (*s == c) return (char *)s;
    if (*s == '\0') break;
    s ++;
  } while (1);
  return NULL;
}

char *strrchr(const char *s, int c) {
  const char *p = s + strlen(s);
  do {
    if (*p == c) return (char *)p;
    if (s == p) break;
    p --;
  } while (1);
  return NULL;
}

#endif
