
#define LED_REG     *((volatile unsigned int*)0x10000000)
#define UART_DATA   *((volatile unsigned int*)0x20000000)
#define UART_STATUS *((volatile unsigned int*)0x20000004)

#define I2C_CTRL_STATUS *((volatile unsigned int*)0x30000000)
#define I2C_SLAVE_ADDR  *((volatile unsigned int*)0x30000004)
#define I2C_SUB_ADDR    *((volatile unsigned int*)0x30000008)
#define I2C_BYTE_LEN    *((volatile unsigned int*)0x3000000C)
#define I2C_TX_DATA     *((volatile unsigned int*)0x30000010)
#define I2C_RX_DATA     *((volatile unsigned int*)0x30000014)


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

// Helper to print integers in bare-metal C without <stdio.h>
void uart_print_int(int num) {
    char buf[10];
    int i = 0;
    
    if (num == 0) {
        uart_putc('0');
        return;
    }
    if (num < 0) {
        uart_putc('-');
        num = -num;
    }
    while (num > 0) {
        buf[i++] = (num % 10) + '0';
        num /= 10;
    }
    // Print the array in reverse
    while (i > 0) {
        uart_putc(buf[--i]);
    }
}

unsigned char i2c_read_reg(unsigned char dev_addr, unsigned char reg_addr) {
    // 1. Wait until I2C is not busy (Bit 0 of status register)
    while (I2C_CTRL_STATUS & 0x01);

    // 2. Load the Address (Shift by 1, OR with 1 for 'Read' mode)
    I2C_SLAVE_ADDR = (dev_addr << 1) | 0x01; 
    
    // 3. Load the Sub-Address (Register inside the sensor)
    I2C_SUB_ADDR = reg_addr;
    
    // 4. Set length to read 1 byte
    I2C_BYTE_LEN = 1;

    // 5. Trigger transaction: bit 0 = start, bit 1 = sub_len (0 = 8-bit sub-addr)
    I2C_CTRL_STATUS = 0x01;

    // 6. Wait until transaction is complete
    while (I2C_CTRL_STATUS & 0x01);

    // 7. Return the data payload
    return I2C_RX_DATA;
}

void main() {
    // Initial LED test state to prove CPU boot
    LED_REG = 0xFF; 

    while(1) {
        // 1. Read the ADT7420 Temperature MSB (Reg 0x00) and LSB (Reg 0x01)
        unsigned char msb = i2c_read_reg(0x4B, 0x00);
        unsigned char lsb = i2c_read_reg(0x4B, 0x01);
        
        // 2. Combine them into a single 16-bit number
        int temp_raw = (msb << 8) | lsb;
        
        // 3. Shift right by 3 (ADT7420 13-bit mode puts 3 status flags at the bottom)
        temp_raw = temp_raw >> 3;
        
        // 4. Calculate Celsius (1 LSB = 0.0625 degrees, or 1/16th of a degree)
        int temp_c_int = temp_raw / 16;
        int temp_c_frac = ((temp_raw % 16) * 100) / 16; 
        
        // 5. Print to Terminal
        uart_print("Live Temp: ");
        uart_print_int(temp_c_int);
        uart_print(".");
        // Handle leading zeros for fractions (e.g., .06)
        if (temp_c_frac >= 0 && temp_c_frac < 10) uart_print("0");
        uart_print_int(temp_c_frac);
        uart_print(" C\r\n");

<<<<<<< Updated upstream
        uart_print("Hello World from RISC-V!\r\n");


        LED_REG = ~LED_REG; 

=======
        // 6. Display the integer temperature on the FPGA LEDs
        LED_REG = temp_c_int; 
>>>>>>> Stashed changes

        delay();
    }
}