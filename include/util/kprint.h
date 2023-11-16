#ifndef KPRINTF_H_
#define KPRINTF_H_

#define KPRINT_FN(FMT, ...) kprintf("%s: " FMT, __func__, ##__VA_ARGS__)

#define PANIC_FN(MESSAGE) panic_2str(__func__, ": " MESSAGE)

void kprintf(const char *fmt, ...);
__attribute__((noreturn)) void panic_2str(const char *s1, const char *s2);
__attribute__((noreturn)) void panic(const char *s);

#endif
