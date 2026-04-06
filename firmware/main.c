#define LED_REG     *((volatile unsigned int*)0x10000000)
#define UART_DATA   *((volatile unsigned int*)0x20000000)
#define UART_STATUS *((volatile unsigned int*)0x20000004)

// --- SPI Registers (Slave 3) ---
#define SPI_DATA    *((volatile unsigned int*)0x30000000)
#define SPI_CTRL    *((volatile unsigned int*)0x30000004) 

// --- I2C Registers (Slave 5) ---
#define I2C_CTRL_STATUS *((volatile unsigned int*)0x50000000)
#define I2C_SLAVE_ADDR  *((volatile unsigned int*)0x50000004)
#define I2C_SUB_ADDR    *((volatile unsigned int*)0x50000008)
#define I2C_BYTE_LEN    *((volatile unsigned int*)0x5000000C)
#define I2C_TX_DATA     *((volatile unsigned int*)0x50000010)
#define I2C_RX_DATA     *((volatile unsigned int*)0x50000014)

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

void uart_print_int(int num) {
    char buf[10]; int i = 0;
    if (num == 0) { uart_putc('0'); return; }
    if (num < 0) { uart_putc('-'); num = -num; }
    while (num > 0) { buf[i++] = (num % 10) + '0'; num /= 10; }
    while (i > 0) { uart_putc(buf[--i]); }
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

// --- I2C Hardware Drivers ---
unsigned char i2c_read_reg(unsigned char dev_addr, unsigned char reg_addr) {
    while (I2C_CTRL_STATUS & 0x01);
    I2C_SLAVE_ADDR = (dev_addr << 1) | 0x01; 
    I2C_SUB_ADDR = reg_addr;
    I2C_BYTE_LEN = 1;
    I2C_CTRL_STATUS = 0x01;
    while (I2C_CTRL_STATUS & 0x01);
    return I2C_RX_DATA;
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

        // 1. Read SPI (Accelerometer)
        spi_cs_low();
        spi_transfer(0x0B); 
        spi_transfer(0x08); 
        
        unsigned int x_data = spi_transfer(0x00); 
        unsigned int y_data = spi_transfer(0x00); 
        unsigned int z_data = spi_transfer(0x00); 
        spi_cs_high();

        // 2. Read I2C (Temperature)
        unsigned char msb = i2c_read_reg(0x4B, 0x00);
        unsigned char lsb = i2c_read_reg(0x4B, 0x01);
        int temp_raw = (msb << 8) | lsb;
        temp_raw = temp_raw >> 3;
        int temp_c_int = temp_raw / 16;
        int temp_c_frac = ((temp_raw % 16) * 100) / 16;

        uart_print("X: 0x"); uart_print_hex8(x_data);
        uart_print(" | Y: 0x"); uart_print_hex8(y_data);
        uart_print(" | Z: 0x"); uart_print_hex8(z_data);

        //Printing Temp:
        uart_print(" | Temp: ");
        uart_print_int(temp_c_int);
        uart_print("."); 
        
        if (temp_c_frac >= 0 && temp_c_frac < 10) {
            uart_print("0");
        }
        uart_print_int(temp_c_frac);
        uart_print(" C");

        uart_print("\r"); 

        delay_5ms();
    }
}