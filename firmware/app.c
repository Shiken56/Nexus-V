#include <stdint.h>

#define LED_REG     (*(volatile uint32_t*)0x10000000)
#define UART_DATA   (*(volatile uint32_t*)0x20000000)
#define UART_STATUS (*(volatile uint32_t*)0x20000004)

void uart_putc(char c) {
    while (!(UART_STATUS & 0x01)); // Wait for TX Ready
    UART_DATA = c;
}

void print_str(const char *str) {
    while (*str) {
        uart_putc(*str++);
    }
}

void delay(volatile uint32_t count) {
    while(count--);
}

void main() {
    uint8_t led_val = 1;
    
    while(1) {
        print_str("Hello from RISC-V SoC!\r\n");
        LED_REG = led_val;
        
        // Circular shift for Knight Rider effect
        led_val = (led_val << 1) | (led_val >> 7);
        
        delay(1000000); // Adjust based on clock speed
    }
}
