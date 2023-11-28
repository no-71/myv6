#include "driver/uart.h"
#include "lock/spin_lock.h"

void uartinit(void)
{
    // disable interrupts.
    WRITE_UART_REG(IER, 0x00);

    // special mode to set baud rate.
    WRITE_UART_REG(LCR, LCR_BAUD_LATCH);

    // LSB for baud rate of 38.4K.
    WRITE_UART_REG(0, 0x03);

    // MSB for baud rate of 38.4K.
    WRITE_UART_REG(1, 0x00);

    // leave set-baud mode,
    // and set word length to 8 bits, no parity.
    WRITE_UART_REG(LCR, LCR_WORD_LEN_1 | LCR_WORD_LEN_0);

    // reset and enable FIFOs.
    WRITE_UART_REG(FCR, FCR_FIFO_ENABLE | FCR_RF_REST | FCR_TF_REST);

    // enable transmit and receive interrupts.
    WRITE_UART_REG(IER, IER_RHRI_ENABLE | IER_THRI_ENABLE);
}

// call with intr off, polling to write char
void uart_polling_write(char c)
{
    while ((READ_UART_REG(LSR) & LSR_THR_READY) == 0) {
    }
    WRITE_UART_REG(THR, c);
}
