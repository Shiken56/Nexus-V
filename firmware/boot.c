#include <stdint.h>

#define UART_DATA   (*(volatile uint32_t*)0x20000000)
#define UART_STATUS (*(volatile uint32_t*)0x20000004)
#define APP_START   0x00001000

void uart_putc(char c) {
    while (!(UART_STATUS & 0x01)); // Wait for TX Ready (Bit 0)
    UART_DATA = c;
}

char uart_getc() {
    while (!(UART_STATUS & 0x02)); // Wait for RX Valid (Bit 1)
    return UART_DATA;
}

void main() {
    // 1. Wait for Upload Command
    while (uart_getc() != 'U');

    // 2. Read 4-byte size (Little Endian)
    uint32_t size = 0;
    for (int i = 0; i < 4; i++) {
        size |= (uart_getc() << (8 * i));
    }

    // 3. Download Payload to App Memory
    volatile char *ram = (volatile char *)APP_START;
    for (uint32_t i = 0; i < size; i++) {
        ram[i] = uart_getc();
    }

    // 4. Send Acknowledgment
    uart_putc('K');

    // 5. Jump to Application
    void (*app)(void) = (void (*)(void))APP_START;
    app();
}
