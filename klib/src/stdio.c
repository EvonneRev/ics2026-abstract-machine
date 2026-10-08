
#include <klib.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

typedef struct {
  char *buffer;
  size_t capacity;
  size_t length;
  int console;
} format_sink_t;

static void format_put(format_sink_t *sink, char ch) {
  if (sink->console) {
    putch(ch);
  } else if (sink->buffer != NULL && sink->capacity > 0 &&
             sink->length < sink->capacity - 1) {
    sink->buffer[sink->length] = ch;
  }
  sink->length++;
}

static void format_repeat(format_sink_t *sink, char ch, int count) {
  while (count-- > 0) format_put(sink, ch);
}

static unsigned int format_divmod(unsigned long long *value, unsigned int base) {
  uint16_t limbs[4] = {
    (uint16_t)(*value >> 48), (uint16_t)(*value >> 32),
    (uint16_t)(*value >> 16), (uint16_t)*value
  };
  uint16_t quotient[4];
  unsigned int remainder = 0;
  for (int i = 0; i < 4; i++) {
    unsigned int part = (remainder << 16) | limbs[i];
    quotient[i] = part / base;
    remainder = part % base;
  }
  *value = ((unsigned long long)quotient[0] << 48) |
           ((unsigned long long)quotient[1] << 32) |
           ((unsigned long long)quotient[2] << 16) | quotient[3];
  return remainder;
}

static int format_write(format_sink_t *sink, const char *fmt, va_list ap) {
  while (*fmt != '\0') {
    if (*fmt != '%') {
      format_put(sink, *fmt++);
      continue;
    }
    fmt++;

    int left = 0, zero = 0, plus = 0, space = 0, alternate = 0;
    for (;;) {
      if (*fmt == '-') left = 1;
      else if (*fmt == '0') zero = 1;
      else if (*fmt == '+') plus = 1;
      else if (*fmt == ' ') space = 1;
      else if (*fmt == '#') alternate = 1;
      else break;
      fmt++;
    }

    int width = 0;
    if (*fmt == '*') {
      long long requested = va_arg(ap, int);
      if (requested < 0) {
        left = 1;
        requested = -requested;
      }
      width = requested > INT_MAX ? INT_MAX : (int)requested;
      fmt++;
    } else {
      while (*fmt >= '0' && *fmt <= '9') {
        int digit = *fmt++ - '0';
        width = width > (INT_MAX - digit) / 10 ?
                INT_MAX : width * 10 + digit;
      }
    }

    int precision = -1;
    if (*fmt == '.') {
      fmt++;
      precision = 0;
      if (*fmt == '*') {
        precision = va_arg(ap, int);
        fmt++;
      } else {
        while (*fmt >= '0' && *fmt <= '9') {
          int digit = *fmt++ - '0';
          precision = precision > (INT_MAX - digit) / 10 ?
                      INT_MAX : precision * 10 + digit;
        }
      }
    }

    enum { LEN_DEFAULT, LEN_HH, LEN_H, LEN_L, LEN_LL, LEN_Z } length = LEN_DEFAULT;
    if (*fmt == 'h') {
      fmt++;
      length = *fmt == 'h' ? (fmt++, LEN_HH) : LEN_H;
    } else if (*fmt == 'l') {
      fmt++;
      length = *fmt == 'l' ? (fmt++, LEN_LL) : LEN_L;
    } else if (*fmt == 'z') {
      fmt++;
      length = LEN_Z;
    }

    char spec = *fmt;
    if (spec == '\0') break;
    fmt++;

    if (spec == 's' || spec == 'c' || spec == '%') {
      const char *str = NULL;
      char single;
      size_t count = 0;
      if (spec == 's') {
        str = va_arg(ap, const char *);
        if (str == NULL) str = "(null)";
        while (str[count] != '\0' &&
               (precision < 0 || count < (size_t)precision)) count++;
      } else {
        single = spec == 'c' ? (char)va_arg(ap, int) : '%';
        str = &single;
        count = 1;
      }
      int padding = (size_t)width > count ? width - (int)count : 0;
      if (!left) format_repeat(sink, ' ', padding);
      for (size_t i = 0; i < count; i++) format_put(sink, str[i]);
      if (left) format_repeat(sink, ' ', padding);
      continue;
    }

    int is_signed = spec == 'd' || spec == 'i';
    if (!is_signed && spec != 'u' && spec != 'o' && spec != 'x' &&
        spec != 'X' && spec != 'p') {
      format_put(sink, '%');
      format_put(sink, spec);
      continue;
    }

    unsigned long long value;
    char sign = '\0';
    if (is_signed) {
      long long signed_value;
      if (length == LEN_LL) signed_value = va_arg(ap, long long);
      else if (length == LEN_L) signed_value = va_arg(ap, long);
      else if (length == LEN_Z) signed_value = va_arg(ap, ptrdiff_t);
      else if (length == LEN_H) signed_value = (short)va_arg(ap, int);
      else if (length == LEN_HH) signed_value = (signed char)va_arg(ap, int);
      else signed_value = va_arg(ap, int);
      if (signed_value < 0) {
        sign = '-';
        value = 0ull - (unsigned long long)signed_value;
      } else {
        value = (unsigned long long)signed_value;
        if (plus) sign = '+';
        else if (space) sign = ' ';
      }
    } else if (spec == 'p') {
      value = (uintptr_t)va_arg(ap, void *);
    } else {
      if (length == LEN_LL) value = va_arg(ap, unsigned long long);
      else if (length == LEN_L) value = va_arg(ap, unsigned long);
      else if (length == LEN_Z) value = va_arg(ap, size_t);
      else if (length == LEN_H) value = (unsigned short)va_arg(ap, unsigned int);
      else if (length == LEN_HH) value = (unsigned char)va_arg(ap, unsigned int);
      else value = va_arg(ap, unsigned int);
    }

    unsigned int base = spec == 'o' ? 8 :
                        (spec == 'x' || spec == 'X' || spec == 'p') ? 16 : 10;
    const char *alphabet = spec == 'X' ?
                           "0123456789ABCDEF" : "0123456789abcdef";
    char digits[sizeof(value) * 8];
    int count = 0;
    if (value != 0 || precision != 0 || spec == 'p') {
      do {
        digits[count++] = alphabet[format_divmod(&value, base)];
      } while (value != 0);
    }

    const char *prefix = "";
    int prefix_length = 0;
    if (spec == 'p' || (alternate && count > 0 &&
        (spec == 'x' || spec == 'X') && digits[count - 1] != '0')) {
      prefix = spec == 'X' ? "0X" : "0x";
      prefix_length = 2;
    } else if (spec == 'o' && alternate &&
               (count == 0 || digits[count - 1] != '0') && precision <= count) {
      prefix = "0";
      prefix_length = 1;
    }

    int leading_zeros = precision > count ? precision - count : 0;
    long long occupied = (sign != '\0') + prefix_length +
                         (long long)leading_zeros + count;
    int padding = occupied < width ? (int)(width - occupied) : 0;
    if (!left && (!zero || precision >= 0)) format_repeat(sink, ' ', padding);
    if (sign != '\0') format_put(sink, sign);
    for (int i = 0; i < prefix_length; i++) format_put(sink, prefix[i]);
    if (!left && zero && precision < 0) format_repeat(sink, '0', padding);
    format_repeat(sink, '0', leading_zeros);
    while (count > 0) format_put(sink, digits[--count]);
    if (left) format_repeat(sink, ' ', padding);
  }

  if (!sink->console && sink->buffer != NULL && sink->capacity > 0) {
    size_t end = sink->length < sink->capacity ? sink->length : sink->capacity - 1;
    sink->buffer[end] = '\0';
  }
  return (int)sink->length;
}

int vprintf(const char *fmt, va_list ap) {
  format_sink_t sink = { NULL, 0, 0, 1 };
  return format_write(&sink, fmt, ap);
}

int printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int result = vprintf(fmt, ap);
  va_end(ap);
  return result;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  format_sink_t sink = { out, n, 0, 0 };
  return format_write(&sink, fmt, ap);
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  return vsnprintf(out, (size_t)-1, fmt, ap);
}

int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int result = vsprintf(out, fmt, ap);
  va_end(ap);
  return result;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int result = vsnprintf(out, n, fmt, ap);
  va_end(ap);
  return result;
}

int __am_vsscanf_internal(const char *str, const char **end_pstr, const char *fmt, va_list ap) {
  const char *pstr = str;
  const char *pfmt = fmt;
  int item = -1;
  while (*pfmt) {
    char ch = *pfmt ++;
    if (isspace(ch)) {
      for (ch = *pfmt; isspace(ch); ch = *(++ pfmt));
      for (ch = *pstr; isspace(ch); ch = *(++ pstr));
      item ++;
      continue;
    }
    switch (ch) {
      case '%': break;
      default:
        if (*pstr == ch) { // match
          pstr ++;
          item ++;
          continue;
        }
        goto end; // fail
    }

    char *p;
    ch = *pfmt ++;
    switch (ch) {
      // conversion specifier
      case 'd':
        *(va_arg(ap, int *)) = strtol(pstr, &p, 10);
        if (p == pstr) goto end; // fail
        pstr = p;
        item ++;
        break;

      case 'c':
        *(va_arg(ap, char *)) = *pstr ++;
        item ++;
        break;

      default:
        printf("Unsupported conversion specifier '%c'\n", ch);
        assert(0);
    }
  }

end:
  if (end_pstr) {
    *end_pstr = pstr;
  }
  return item;
}

int vsscanf(const char *str, const char *fmt, va_list ap) {
  return __am_vsscanf_internal(str, NULL, fmt, ap);
}

int sscanf(const char *str, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = vsscanf(str, fmt, ap);
  va_end(ap);
  return r;
}

int __isoc99_sscanf(const char *str, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = vsscanf(str, fmt, ap);
  va_end(ap);
  return r;
}

#endif
