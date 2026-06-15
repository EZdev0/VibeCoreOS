/*
 * ============================================================
 *  VibeCore OS — String-Utilities Header
 * ============================================================
 */

#ifndef _STRING_H
#define _STRING_H

#include "types.h"

/* ── String-Funktionen ───────────────────────────────────── */
size_t strlen(const char *str);
int    strcmp(const char *a, const char *b);
int    strncmp(const char *a, const char *b, size_t n);
char  *strcpy(char *dst, const char *src);
char  *strncpy(char *dst, const char *src, size_t n);

/* ── Speicher-Funktionen ─────────────────────────────────── */
void  *memset(void *ptr, int val, size_t n);
void  *memcpy(void *dst, const void *src, size_t n);
void  *memmove(void *dst, const void *src, size_t n);
int    memcmp(const void *a, const void *b, size_t n);

/* ── UART-Ausgabe-Helfer ─────────────────────────────────── */
void uart_putc(char c);
void uart_puts(const char *str);
void uart_putu(u64 n);
void uart_puthex(u64 n, bool upper);
void uart_printf(const char *fmt, ...);

/* ── UART Driver ─────────────────────────────────────────── */
void uart_init(void);
char uart_getc(void);
bool uart_has_char(void);

#endif /* _STRING_H */
