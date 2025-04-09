#ifndef CONSOLE_H_
#define CONSOLE_H_

#include "config/basic_types.h"

#define CTRL(x) ((x) - '@') // Control-x

enum { BACKSPACE, GET_LINE, GET_EOF, GET_CH };

void console_init();
void console_kputc(char c);
void console_kwrite(const char *s, size_t len);
void console_kwrite_str(const char *s);
void console_putc(char c);
void console_write(const char *s, size_t len);
char console_getc(void);
int console_intr(char c);

#endif
