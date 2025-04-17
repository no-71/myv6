#include "driver/uart.h"
#include "io/console/console.h"
#include "lock/spin_lock.h"
#include "scheduler/sleep.h"
#include "trap/introff.h"

// require 2^64 % UART_OUTPUT_SIZE == 0, 2^64 % UART_INPUT_SIZE == 0, so that we
// can be safe when uint64 overflow
#define UART_OUTPUT_SIZE 16
#define UART_INPUT_SIZE 64

char uart_output_queue[UART_OUTPUT_SIZE];
uint64 uart_oq_beg, uart_oq_end;
struct spin_lock uart_output_lock;

char uart_input_queue[UART_INPUT_SIZE];
uint64 uart_iq_beg, uart_iq_end;
struct spin_lock uart_input_lock;

void uartinit(void)
{
    init_spin_lock(&uart_output_lock);
    init_spin_lock(&uart_input_lock);

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
void uart_polling_putc(char c)
{
    while ((READ_UART_REG(LSR) & LSR_THR_READY) == 0) {
    }
    WRITE_UART_REG(THR, c);
}

static int uart_queue_empty(uint64 beg, uint64 end) { return beg == end; }

static int uart_queue_full(uint64 beg, uint64 end, uint64 size)
{
    return end - beg == size;
}

int uart_input_not_empty(void) { return uart_iq_beg != uart_iq_end; }

static int uart_putc_output_queue(void)
{
    if ((READ_UART_REG(LSR) & LSR_THR_READY) &&
        uart_queue_empty(uart_oq_beg, uart_oq_end) == 0) {
        WRITE_UART_REG(THR, uart_output_queue[uart_oq_beg % UART_OUTPUT_SIZE]);
        uart_oq_beg++;
        return 0;
    }
    return -1;
}

static void uart_running_output_queue(void)
{
    int has_output = 0;
    while (uart_putc_output_queue() == 0) {
        has_output = 1;
    }
    if (has_output) {
        wake_up(&uart_output_queue);
    }
}

int uart_getc_input_queue(void)
{
    if (READ_UART_REG(LSR) & LSR_RHR_READY) {
        char c = READ_UART_REG(RHR);

        int res = console_intr(c);
        switch (res) {
        case BACKSPACE:
            if (uart_queue_empty(uart_iq_beg, uart_iq_end) == 0) {
                uart_iq_end--;
            }
            return 0;

        case GET_LINE:
            c = '\n';
            break;

        case GET_EOF:
            break;
        }

        uart_input_queue[uart_iq_end % UART_INPUT_SIZE] = c;
        if (uart_queue_full(uart_iq_beg, uart_iq_end, UART_INPUT_SIZE)) {
            uart_iq_beg++;
        }
        uart_iq_end++;
        return 0;
    }
    return -1;
}

static void uart_running_input_queue(void)
{
    int has_input = 0;
    while (uart_getc_input_queue() == 0) {
        has_input = 1;
    }
    if (has_input) {
        wake_up(&uart_input_queue);
    }
}

void uart_write(const char *s, size_t len)
{
    acquire_spin_lock(&uart_output_lock);

    size_t cur = 0;
    while (cur < len) {
        while (uart_queue_full(uart_oq_beg, uart_oq_end, UART_OUTPUT_SIZE)) {
            sleep(&uart_output_lock, &uart_output_queue);
        }

        while (uart_queue_full(uart_oq_beg, uart_oq_end, UART_OUTPUT_SIZE) ==
                   0 &&
               cur != len) {
            uart_output_queue[uart_oq_end % UART_OUTPUT_SIZE] = s[cur];
            uart_oq_end++;
            cur++;
        }
        uart_running_output_queue();
    }

    release_spin_lock(&uart_output_lock);
}

void uart_putc(char c) { uart_write(&c, 1); }

int uart_just_getc(void)
{
    acquire_spin_lock(&uart_input_lock);

    if (uart_queue_empty(uart_iq_beg, uart_iq_end)) {
        release_spin_lock(&uart_input_lock);
        return -1;
    }

    char c = uart_input_queue[uart_iq_beg % UART_INPUT_SIZE];
    uart_iq_beg++;

    release_spin_lock(&uart_input_lock);
    return c;
}

char uart_getc(void)
{
    acquire_spin_lock(&uart_input_lock);

    while (uart_queue_empty(uart_iq_beg, uart_iq_end)) {
        sleep(&uart_input_lock, &uart_input_queue);
    }

    char c = uart_input_queue[uart_iq_beg % UART_INPUT_SIZE];
    uart_iq_beg++;

    release_spin_lock(&uart_input_lock);
    return c;
}

void uart_intr(void)
{
    acquire_spin_lock(&uart_input_lock);
    uart_running_input_queue();
    release_spin_lock(&uart_input_lock);

    acquire_spin_lock(&uart_output_lock);
    uart_running_output_queue();
    release_spin_lock(&uart_output_lock);
}
