
#define LED_REG     *((volatile unsigned int*)0x10000000)
#define UART_DATA   *((volatile unsigned int*)0x20000000)
#define UART_STATUS *((volatile unsigned int*)0x20000004)


void delay() {
    for (volatile int i = 0; i < 5000000; i++);
}


void uart_putc(char c) {

    while (!(UART_STATUS & 0x01));

    UART_DATA = c;
}


void uart_print(const char *str) {
    while (*str) {
        uart_putc(*str++);
    }
}

void main() {

    *((volatile unsigned int*) 0x10000000) = 0xFF; 

    while(1) {

        uart_print("Axi_lite works :)\r\n");


        LED_REG = ~LED_REG; 


        delay();
    }
}