#ifndef UART_H_
#define UART_H_

#include "config/basic_types.h"
#include "riscv/regs.h"

#define UART_BASE 0x10000000
#define UART_SIZE 0x100
#define UART_END (UART_BASE + UART_SIZE)
#define UART_IRQ 0xa

#define UART_REG(REG) (UART_BASE + REG)
#define READ_UART_REG(REG) GET_REG(char, UART_REG(REG))
#define WRITE_UART_REG(REG, VAL) (READ_UART_REG(REG) = VAL)

#define RHR 0
#define THR 0
#define IER 1
#define FCR 2
#define ISR 2
#define LCR 3
#define LSR 5

#define IER_RHRI_ENABLE (1 << 0)
#define IER_THRI_ENABLE (1 << 1)

#define FCR_FIFO_ENABLE (1 << 0)
#define FCR_RF_REST (1 << 1)
#define FCR_TF_REST (1 << 2)

#define LCR_WORD_LEN_0 (1 << 0)
#define LCR_WORD_LEN_1 (1 << 1)
// DLAB mode to set baud rate
#define LCR_BAUD_LATCH (1 << 7)

#define LSR_RHR_READY (1 << 0)
#define LSR_THR_READY (1 << 5)

void uartinit(void);
void uart_polling_putc(char c);
void uart_write(const char *s, size_t len);
void uart_putc(char c);
char uart_getc(void);
int uart_just_getc(void);
void uart_intr(void);

#endif
