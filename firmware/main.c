#define LED_REG     *((volatile unsigned int*)0x10000000)
#define UART_DATA   *((volatile unsigned int*)0x20000000)
#define UART_STATUS *((volatile unsigned int*)0x20000004)

// --- SPI Registers (Slave 3) ---
#define SPI_DATA    *((volatile unsigned int*)0x30000000)
#define SPI_CTRL    *((volatile unsigned int*)0x30000004) 

void delay_5ms() {
    // Roughly 5ms at 100MHz 
    for (volatile int i = 0; i < 500000; i++); 
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

void uart_print_hex8(unsigned int val) {
    const char hex_chars[] = "0123456789ABCDEF";
    uart_putc(hex_chars[(val >> 4) & 0x0F]); 
    uart_putc(hex_chars[val & 0x0F]);        
}

// --- SPI Hardware Drivers ---
void wait_spi_ready() {
    while ((SPI_CTRL & 0x01) == 0); 
}

unsigned int spi_transfer(unsigned int data_to_send) {
    wait_spi_ready();        
    
    // LINTER FIX: Dummy read to clear any stale RX flags before we transmit
    volatile unsigned int dummy = SPI_DATA; 
    (void)dummy; // Prevent compiler warning

    SPI_DATA = data_to_send; 
    
    // Wait for the new RX byte to arrive (Bit 1)
    while ((SPI_CTRL & 0x02) == 0); 
    
    return SPI_DATA & 0xFF;  
}

void spi_cs_low() {
    wait_spi_ready(); // LINTER FIX: Never pull CS low mid-transfer
    SPI_CTRL = 0x00; 
}

void spi_cs_high() {
    wait_spi_ready(); // LINTER FIX: Never pull CS high mid-transfer
    SPI_CTRL = 0x02; 
}

void adxl_write_reg(unsigned int reg_addr, unsigned int val) {
    spi_cs_low();
    spi_transfer(0x0A);      
    spi_transfer(reg_addr);  
    spi_transfer(val);       
    spi_cs_high();
}

// --- Main Program ---
void main() {
    LED_REG = 0xFF; 
    spi_cs_high(); 

    // LINTER FIX: Give the ADXL362 time to wake up after board power-on!
    delay_5ms();
    delay_5ms(); 

    uart_print("\r\n================================\r\n");
    uart_print("   Nexus-V Continuous Monitor\r\n");
    uart_print("================================\r\n");

    unsigned int device_id = 0x00;

    // --- LIVE DEBUG LOOP ---
    // Instead of halting, keep trying every 1 second until it works
    while (device_id != 0xAD) {
        uart_print("Probing Sensor... ");
        
        spi_cs_low();         
        spi_transfer(0x0B);   
        spi_transfer(0x00);   
        device_id = spi_transfer(0x00); 
        spi_cs_high();        

        if (device_id == 0xAD) {
            uart_print("SUCCESS! (0xAD)\r\n");
            break; // Break out of the loop and start measuring
        } else {
            uart_print("FAILED. Got: 0x");
            uart_print_hex8(device_id);
            uart_print(". Retrying in 1s...\r\n");
            
            // Wait ~1 second before trying again
            for (int i = 0; i < 10; i++) delay_5ms(); 
        }
    }

    // 2. Turn on Measurement Mode 
    adxl_write_reg(0x2D, 0x02);
    
    uart_print("Starting Data Stream...\r\n\r\n");

    // --- Main Sensor Loop ---
    while(1) {
        LED_REG = ~LED_REG; 

        spi_cs_low();
        spi_transfer(0x0B); 
        spi_transfer(0x08); 
        
        unsigned int x_data = spi_transfer(0x00); 
        unsigned int y_data = spi_transfer(0x00); 
        unsigned int z_data = spi_transfer(0x00); 
        spi_cs_high();

        uart_print("X: 0x"); uart_print_hex8(x_data);
        uart_print(" | Y: 0x"); uart_print_hex8(y_data);
        uart_print(" | Z: 0x"); uart_print_hex8(z_data);
        uart_print("\r"); 

        delay_5ms();
    }
}