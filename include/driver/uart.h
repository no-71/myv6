#ifndef UART_H_
#define UART_H_

#define UART_BASE 0x10000000
#define UART_SIZE 0x100
#define UART_END (UART_BASE + UART_SIZE)
#define UART_REG(REG) (UART_BASE + REG)
#define READ_UART_REG(REG) (*(volatile char *)UART_REG(REG))
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
#define LCR_BAUD_LATCH (1 << 7) // DLAB mode to set baud rate

#define LSR_RHR_READY (1 << 0)
#define LSR_THR_READY (1 << 5)

void uartinit(void);
void uart_polling_write(char c);

#endif
