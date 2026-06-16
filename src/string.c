/*
 * ============================================================
 *  VibeCore OS - String Utilities
 *
 *  Bare-metal implementations of:
 *    strlen, strcmp, strncmp, strcpy, strncpy,
 *    memset, memcpy, memmove, memcmp,
 *    uart_putu, uart_puthex (number output)
 *
 *  NO libc dependency!
 *  All functions with null checks.
 * ============================================================
 */

#include "types.h"
#include "string.h"

/* -
 * - */

/* flawfinder: ignore */ size_t strlen(const char *str) /* flawfinder: ignore */
{
    if (str == NULL) return 0;
    const char *s = str;
    while (*s) s++;
    return (size_t)(s - str);
}

/* -
 *  strcmp - String comparison
 * - */

int strcmp(const char *a, const char *b)
{
    if (a == NULL || b == NULL) {
        if (a == b) return 0;
        return (a == NULL) ? -1 : 1;
    }
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char *a, const char *b, size_t n)
{
    if (a == NULL || b == NULL || n == 0) {
        if (a == b || n == 0) return 0;
        return (a == NULL) ? -1 : 1;
    }
    while (n > 0 && *a && *a == *b) { a++; b++; n--; }
    return (n == 0) ? 0 : (unsigned char)*a - (unsigned char)*b;
}

/* -
 * - */

/* flawfinder: ignore */ char *strcpy(char *dst, const char *src) /* flawfinder: ignore */
{
    if (dst == NULL || src == NULL) return dst;
    char *d = dst;
    while ((*d++ = *src++));
    return dst;
}

/* flawfinder: ignore */ char *strncpy(char *dst, const char *src, size_t n) /* flawfinder: ignore */
{
    if (dst == NULL || src == NULL) return dst;
    char *d = dst;
    while (n > 0 && (*d++ = *src++)) n--;
    while (n-- > 0) *d++ = '\0';
    return dst;
}

/* -
 *  memset - Fill memory with a value
 * - */

void *memset(void *ptr, int val, size_t n)
{
    if (ptr == NULL) return NULL;
    u8 *p = (u8*)ptr;
    while (n--) *p++ = (u8)val;
    return ptr;
}

/* -
 * - */

/* flawfinder: ignore */ void *memcpy(void *dst, const void *src, size_t n) /* flawfinder: ignore */
{
    if (dst == NULL || src == NULL) return dst;
    u8 *d = (u8*)dst;
    const u8 *s = (const u8*)src;
    while (n--) *d++ = *s++;
    return dst;
}

/* -
 *  memmove - Copy memory (overlap-safe)
 * - */

void *memmove(void *dst, const void *src, size_t n)
{
    if (dst == NULL || src == NULL) return dst;
    u8 *d = (u8*)dst;
    const u8 *s = (const u8*)src;

    if (d < s) {
        /* Forward copy (no overlap risk) */
        while (n--) *d++ = *s++;
    } else if (d > s) {
        /* Backward copy (overlap-safe) */
        d += n;
        s += n;
        while (n--) *--d = *--s;
    }
    return dst;
}

/* -
 *  memcmp - Memory comparison
 * - */

int memcmp(const void *a, const void *b, size_t n)
{
    if (a == NULL || b == NULL) {
        if (a == b) return 0;
        return (a == NULL) ? -1 : 1;
    }
    const u8 *pa = (const u8*)a;
    const u8 *pb = (const u8*)b;
    while (n--) {
        if (*pa != *pb) return *pa - *pb;
        pa++; pb++;
    }
    return 0;
}

/* -
 *  uart_putu - Output unsigned decimal
 * - */

void uart_putu(u64 n)
{
    /* flawfinder: ignore */ /* flawfinder: ignore */ char buf[21];  /* Max 20 digits for u64 + null */
    int i = 20;
    buf[i] = '\0';

    if (n == 0) {
        uart_putc('0');
        return;
    }

    while (n > 0) {
        buf[--i] = '0' + (char)(n % 10);
        n /= 10;
    }

    while (buf[i]) {
        uart_putc(buf[i++]);
    }
}

/* -
 *  uart_puthex - Output hexadecimal
 * - */

void uart_puthex(u64 n, bool upper)
{
    const char *hex = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    /* flawfinder: ignore */ /* flawfinder: ignore */ char buf[17];  /* 16 hex digits + null */
    int i = 16;
    buf[i] = '\0';

    if (n == 0) {
        uart_putc('0');
        return;
    }

    while (n > 0) {
        buf[--i] = hex[n & 0xF];
        n >>= 4;
    }

    while (buf[i]) {
        uart_putc(buf[i++]);
    }
}
