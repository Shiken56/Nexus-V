/*
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
*/ 
/*
#define LED_REG *((volatile unsigned int *)0x10000000)
#define UART_DATA *((volatile unsigned int *)0x20000000)
#define UART_STATUS *((volatile unsigned int *)0x20000004)

// --- SPI Registers (Slave 3) ---
#define SPI_DATA *((volatile unsigned int *)0x30000000)
#define SPI_CTRL *((volatile unsigned int *)0x30000004)
//VRAM
#define VRAM_BASE 0x40000000
#define GRID_WIDTH 20
#define GRID_HEIGHT 15

// Example Function to test the screen
void draw_screen_test() {
    volatile unsigned char *vram = (volatile unsigned char *)VRAM_BASE;
    
    // Clear the screen (Black = 0x00)
    for (int i = 0; i < 300; i++) {
        vram[i] = 0x00; 
    }

    // Draw a Green Snake Head at X=10, Y=7 (Center)
    // RGB332 Green = 0b000_111_00 = 0x1C
    vram[(7 * GRID_WIDTH) + 5] = 0x1C;
    
    // Draw a Red Apple at X=15, Y=3
    // RGB332 Red = 0b111_000_00 = 0xE0
    vram[(3 * GRID_WIDTH) + 15] = 0xE0;

    vram[(5 * GRID_WIDTH) + 15] = 0xE0;
}

void delay_5ms()
{
    // Roughly 5ms at 100MHz
    for (volatile int i = 0; i < 500000; i++)
        ;
}

void uart_putc(char c)
{
    while (!(UART_STATUS & 0x01))
        ;
    UART_DATA = c;
}

void uart_print(const char *str)
{
    while (*str)
    {
        uart_putc(*str++);
    }
}

void uart_print_hex8(unsigned int val)
{
    const char hex_chars[] = "0123456789ABCDEF";
    uart_putc(hex_chars[(val >> 4) & 0x0F]);
    uart_putc(hex_chars[val & 0x0F]);
}

// --- SPI Hardware Drivers ---
void wait_spi_ready()
{
    while ((SPI_CTRL & 0x01) == 0)
        ;
}

unsigned int spi_transfer(unsigned int data_to_send)
{
    wait_spi_ready();

    // LINTER FIX: Dummy read to clear any stale RX flags before we transmit
    volatile unsigned int dummy = SPI_DATA;
    (void)dummy; // Prevent compiler warning

    SPI_DATA = data_to_send;

    // Wait for the new RX byte to arrive (Bit 1)
    while ((SPI_CTRL & 0x02) == 0)
        ;

    return SPI_DATA & 0xFF;
}

void spi_cs_low()
{
    wait_spi_ready(); // LINTER FIX: Never pull CS low mid-transfer
    SPI_CTRL = 0x00;
}

void spi_cs_high()
{
    wait_spi_ready(); // LINTER FIX: Never pull CS high mid-transfer
    SPI_CTRL = 0x02;
}

void adxl_write_reg(unsigned int reg_addr, unsigned int val)
{
    spi_cs_low();
    spi_transfer(0x0A);
    spi_transfer(reg_addr);
    spi_transfer(val);
    spi_cs_high();
}

// --- Main Program ---
void main()
{
    LED_REG = 0xAA;
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
    while (device_id != 0xAD)
    {
        uart_print("Probing Sensor... ");

        spi_cs_low();
        spi_transfer(0x0B);
        spi_transfer(0x00);
        device_id = spi_transfer(0x00);
        spi_cs_high();

        if (device_id == 0xAD)
        {
            uart_print("SUCCESS! (0xAD)\r\n");
            break; // Break out of the loop and start measuring
        }
        else
        {
            uart_print("FAILED. Got: 0x");
            uart_print_hex8(device_id);
            uart_print(". Retrying in 1s...\r\n");

            // Wait ~1 second before trying again
            for (int i = 0; i < 10; i++)
                delay_5ms();
        }
    }

    // 2. Turn on Measurement Mode
    adxl_write_reg(0x2D, 0x02);

    uart_print("Starting Data Stream...\r\n\r\n");

    draw_screen_test(); // Test vga


    // --- Main Sensor Loop ---
    while (1)
    {
        LED_REG = ~LED_REG;

        spi_cs_low();
        spi_transfer(0x0B);
        spi_transfer(0x08);

        unsigned int x_data = spi_transfer(0x00);
        unsigned int y_data = spi_transfer(0x00);
        unsigned int z_data = spi_transfer(0x00);
        spi_cs_high();

        uart_print("X: 0x");
        uart_print_hex8(x_data);
        uart_print(" | Y: 0x");
        uart_print_hex8(y_data);
        uart_print(" | Z: 0x");
        uart_print_hex8(z_data);
        uart_print("\r");

        delay_5ms();
    }
}

*/


// #define LED_REG     *((volatile unsigned int*)0x10000000)
// #define UART_DATA   *((volatile unsigned int*)0x20000000)
// #define UART_STATUS *((volatile unsigned int*)0x20000004)

// // SPI
// #define SPI_DATA    *((volatile unsigned int*)0x30000000)
// #define SPI_CTRL    *((volatile unsigned int*)0x30000004) 

// // ---------------- DELAY ----------------
// void delay_5ms() {
//     for (volatile int i = 0; i < 500000; i++);
// }

// // ---------------- UART ----------------
// void uart_putc(char c) {
//     while (!(UART_STATUS & 0x01));
//     UART_DATA = c;
// }

// void uart_print(const char *s) {
//     while (*s) uart_putc(*s++);
// }

// // ---------------- SPI ----------------
// void wait_spi_ready() {
//     while ((SPI_CTRL & 0x01) == 0);
// }

// unsigned int spi_transfer(unsigned int d) {
//     wait_spi_ready();
//     volatile unsigned int dummy = SPI_DATA;
//     (void)dummy;

//     SPI_DATA = d;
//     while ((SPI_CTRL & 0x02) == 0);
//     return SPI_DATA & 0xFF;
// }

// void spi_cs_low() {
//     wait_spi_ready();
//     SPI_CTRL = 0x00;
// }

// void spi_cs_high() {
//     wait_spi_ready();
//     SPI_CTRL = 0x02;
// }

// // ---------------- ADXL ----------------
// void adxl_write_reg(unsigned int reg, unsigned int val) {
//     spi_cs_low();
//     spi_transfer(0x0A);
//     spi_transfer(reg);
//     spi_transfer(val);
//     spi_cs_high();
// }

// void read_xyz(int *x, int *y, int *z) {
//     spi_cs_low();
//     spi_transfer(0x0B);
//     spi_transfer(0x08);

//     *x = (signed char)spi_transfer(0x00);
//     *y = (signed char)spi_transfer(0x00);
//     *z = (signed char)spi_transfer(0x00);

//     spi_cs_high();
// }


// // ---------------- MAIN ----------------
// void main() {

//     LED_REG = 0x00;
//     spi_cs_high();

//     delay_5ms(); delay_5ms();

//     uart_print("\r\n=== Gesture Control Mode ===\r\n");

//     // Sensor detect
//     unsigned int id = 0;
//     while (id != 0xAD) {
//         spi_cs_low();
//         spi_transfer(0x0B);
//         spi_transfer(0x00);
//         id = spi_transfer(0x00);
//         spi_cs_high();
//         delay_5ms();
//     }

//     uart_print("Sensor Ready!\r\n");

//     // Enable measurement
//     adxl_write_reg(0x2D, 0x02);

//     // -------- CALIBRATION --------
//     int x_off=0, y_off=0, z_off=0;
//     int x,y,z;

//     for (int i=0;i<100;i++){
//         read_xyz(&x,&y,&z);
//         x_off+=x; y_off+=y; z_off+=z;
//         delay_5ms();
//     }

//     x_off/=100; y_off/=100; z_off/=100;

//     uart_print("Cal Done\r\n");

//     // -------- STATE --------
//     int prev_x=0, prev_y=0, prev_z=0;
//     int tap_state = 0;

//     while (1) {

//         read_xyz(&x,&y,&z);

//         x -= x_off;
//         y -= y_off;
//         z -= z_off;

//         int dx = x - prev_x;
//         int dy = y - prev_y;
//         int dz = z - prev_z;

//         int motion = dx*dx + dy*dy + dz*dz;

//         // -------- GESTURE LOGIC --------

//         // 🔴 SHAKE
//         if (motion > 300) {
//             LED_REG = 0xFF;
//         }

//         // 🟡 TAP (quick spike)
//         else if (motion > 120) {
//             tap_state = !tap_state;
//             LED_REG = tap_state ? 0xF0 : 0x0F;
//         }

//         // 🔵 TILT RIGHT
//         else if (x > 20) {
//             LED_REG = 0x01;
//         }

//         // 🔵 TILT LEFT
//         else if (x < -20) {
//             LED_REG = 0x80;
//         }

//         // 🟢 FORWARD / BACK
//         else if (y > 20) {
//             LED_REG = 0x18;
//         }
//         else if (y < -20) {
//             LED_REG = 0x24;
//         }

//         // ⚪ IDLE (breathing effect)
//         else {
//             static int counter = 0;
//             counter++;
//             LED_REG = (1 << (counter % 8));
//         }

//         prev_x = x;
//         prev_y = y;
//         prev_z = z;

//         delay_5ms();
//     }
// }
/*
#include <stdint.h>

// --- AXI Memory Map Addresses ---
#define VRAM_BASE 0x40000000
#define UART_BASE 0x20000000

// Using volatile uint8_t pointers because your AXI wrapper perfectly handles byte-strobes!
volatile uint8_t* const vram = (volatile uint8_t*)VRAM_BASE;
volatile uint32_t* const uart_data = (volatile uint32_t*)UART_BASE;
volatile uint32_t* const uart_stat = (volatile uint32_t*)(UART_BASE + 4);

// RGB332 Colors
#define COLOR_RED   0xE0  // 111_000_00
#define COLOR_GREEN 0x1C  // 000_111_00
#define COLOR_BLACK 0x88  // 000_111_00
#define COLOR_WHITE 0x00  // 000_111_00

// Simple delay function
void delay(int count) {
    for (volatile int i = 0; i < count; i++);
}

// "Soft Reset" function to jump back to the bootloader
void software_reset() {
    // Jump to memory address 0x00000000 (RAM Base / Bootloader location)
    void (*bootloader)(void) = (void (*)(void))0x00000000;
    bootloader();
}

int main() {
    uint8_t frame = 0;
    
    while(1) {
        // --- 1. Draw the XOR Pattern ---
        // VRAM is a 20x15 grid (300 bytes)
        for (int y = 0; y < 15; y++) {
            for (int x = 0; x < 20; x++) {
                // The XOR Magic trick: (x XOR y XOR frame)
                // If the lowest bit is 1, it's Red. If 0, it's Green.
                if (((x ^ y) ^ frame) & 1) {
                    vram[y * 20 + x] = COLOR_BLACK;
                } else {
                    vram[y * 20 + x] = COLOR_WHITE;
                }
            }
        }
        
        frame++; // Advance the animation
        delay(300000); // Adjust this number to make the animation faster/slower

        // --- 2. Listen for "Magic Reboot Byte" from Python ---
        // In your Verilog: uart_rdata <= {30'b0, uart_rx_tvalid, uart_tx_tready};
        // So bit 1 (0x02) means rx_valid (data is waiting to be read)
        if ((*uart_stat) & 0x02) { 
            uint8_t rx_byte = (*uart_data) & 0xFF; // Read the byte
            
            // If we receive the magic byte 0x7F (DEL), reset the CPU!
            if (rx_byte == 0x7F) { 
                software_reset();
            }
        }
    }
    return 0;
}
*/
/*
#include <stdint.h>

// --- AXI Memory Map Addresses ---
#define VRAM_BASE 0x40000000
#define UART_BASE 0x20000000

volatile uint8_t* const vram = (volatile uint8_t*)VRAM_BASE;
volatile uint32_t* const uart_data = (volatile uint32_t*)UART_BASE;
volatile uint32_t* const uart_stat = (volatile uint32_t*)(UART_BASE + 4);

// --- Corrected RGB332 Colors ---
#define COLOR_BLACK   0x00  // 000_000_00
#define COLOR_WHITE   0xFF  // 111_111_11
#define COLOR_RED     0xE0  // 111_000_00
#define COLOR_GREEN   0x1C  // 000_111_00
#define COLOR_BLUE    0x03  // 000_000_11
#define COLOR_YELLOW  0xFC  // 111_111_00
#define COLOR_CYAN    0x1F  // 000_111_11
#define COLOR_MAGENTA 0xE3  // 111_000_11

// Soft Reset function to jump back to bootloader
void software_reset() {
    void (*bootloader)(void) = (void (*)(void))0x00000000;
    bootloader();
}

// A delay function that continually listens for the Python upload command
void responsive_delay(int cycles) {
    for (volatile int i = 0; i < cycles; i++) {
        // Check UART RX Valid (Bit 1)
        if ((*uart_stat) & 0x02) { 
            uint8_t rx_byte = (*uart_data) & 0xFF;
            if (rx_byte == 0x7F) { 
                software_reset(); // Reboot immediately if Python script is run
            }
        }
    }
}

// Helper function to fill the whole 20x15 screen with one color
void fill_screen(uint8_t color) {
    for (int i = 0; i < 300; i++) {
        vram[i] = color;
    }
}

int main() {
    while(1) {
        
        // ==========================================
        // TEST 1: Primary Colors (Checks pin wiring)
        // ==========================================
        fill_screen(COLOR_RED);
        responsive_delay(800000); 
        
        fill_screen(COLOR_GREEN);
        responsive_delay(800000); 
        
        fill_screen(COLOR_BLUE);
        responsive_delay(800000); 

        // ==========================================
        // TEST 2: SMPTE-style Vertical Color Bars
        // ==========================================
        // 8 colors across 20 columns. 
        uint8_t bars[] = {COLOR_WHITE, COLOR_YELLOW, COLOR_CYAN, COLOR_GREEN, 
                          COLOR_MAGENTA, COLOR_RED, COLOR_BLUE, COLOR_BLACK};
                          
        for (int y = 0; y < 15; y++) {
            for (int x = 0; x < 20; x++) {
                // Divide the 20 columns into 8 bars (approx 2.5 pixels wide)
                int bar_index = (x * 8) / 20; 
                vram[y * 20 + x] = bars[bar_index];
            }
        }
        responsive_delay(1500000); // Hold color bars for longer

        // ==========================================
        // TEST 3: Boundary & Center Crosshair Test
        // ==========================================
        // This ensures the monitor isn't cutting off the edges of your 20x15 grid
        fill_screen(COLOR_BLACK);
        
        for (int y = 0; y < 15; y++) {
            for (int x = 0; x < 20; x++) {
                // Draw white border on the extreme edges
                if (x == 0 || x == 19 || y == 0 || y == 14) {
                    vram[y * 20 + x] = COLOR_WHITE;
                }
                // Draw a simple red crosshair in the middle
                else if (x == 9 || x == 10 || y == 7) {
                    vram[y * 20 + x] = COLOR_RED;
                }
            }
        }
        responsive_delay(1500000); // Hold boundary test
        
    }
    return 0;
}
*/

//Basic Maze with pixel at center
/*
#include <stdint.h>

// --- AXI Memory Map Addresses ---
#define VRAM_BASE 0x40000000
#define UART_BASE 0x20000000
#define SPI_BASE  0x30000000

volatile uint8_t* const vram      = (volatile uint8_t*)VRAM_BASE;
volatile uint32_t* const uart_data = (volatile uint32_t*)UART_BASE;
volatile uint32_t* const uart_stat = (volatile uint32_t*)(UART_BASE + 4);
volatile uint32_t* const spi_data  = (volatile uint32_t*)SPI_BASE;
volatile uint32_t* const spi_ctrl  = (volatile uint32_t*)(SPI_BASE + 4);

// --- RGB332 Colors ---
#define COLOR_BLACK   0x00
#define COLOR_WHITE   0xFF
#define COLOR_RED     0xE0
#define COLOR_GREEN   0x1C
#define COLOR_BLUE    0x03
#define COLOR_YELLOW  0xFC
#define COLOR_GRAY    0x6D

void software_reset() {
    void (*bootloader)(void) = (void (*)(void))0x00000000;
    bootloader();
}

void responsive_delay(int cycles) {
    for (volatile int i = 0; i < cycles; i++) {
        if ((*uart_stat) & 0x02) { 
            uint8_t rx_byte = (*uart_data) & 0xFF;
            if (rx_byte == 0x7F) { 
                software_reset();
            }
        }
    }
}

int clamp(int val, int min, int max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

// --- SPI & ADXL362 FUNCTIONS ---
void wait_spi_ready() {
    while (((*spi_ctrl) & 0x01) == 0);
}

unsigned int spi_transfer(unsigned int d) {
    wait_spi_ready();
    volatile unsigned int dummy = *spi_data; 
    (void)dummy;

    *spi_data = d;
    while (((*spi_ctrl) & 0x02) == 0);
    return (*spi_data) & 0xFF;
}

void spi_cs_low() { wait_spi_ready(); *spi_ctrl = 0x00; }
void spi_cs_high() { wait_spi_ready(); *spi_ctrl = 0x02; }

void adxl_write_reg(unsigned int reg, unsigned int val) {
    spi_cs_low();
    spi_transfer(0x0A);
    spi_transfer(reg);
    spi_transfer(val);
    spi_cs_high();
}

void read_xyz(int *x, int *y, int *z) {
    spi_cs_low();
    spi_transfer(0x0B);
    spi_transfer(0x08);

    *x = (signed char)spi_transfer(0x00);
    *y = (signed char)spi_transfer(0x00);
    *z = (signed char)spi_transfer(0x00);

    spi_cs_high();
}

void draw_background() {
    for (int y = 0; y < 15; y++) {
        for (int x = 0; x < 20; x++) {
            if (x == 9 || y == 7) vram[y * 20 + x] = COLOR_GRAY;
            else vram[y * 20 + x] = COLOR_BLACK;
        }
    }
}

int main() {
    spi_cs_high();
    responsive_delay(100000);

    // 1. Wait for ADXL362 Sensor
    unsigned int id = 0;
    while (id != 0xAD) { 
        spi_cs_low();
        spi_transfer(0x0B);
        spi_transfer(0x00);
        id = spi_transfer(0x00);
        spi_cs_high();
        
        for(int i=0; i<300; i++) vram[i] = COLOR_RED; 
        responsive_delay(50000);
    }

    // 2. Enable measurement mode (0x02 is correct for ADXL362!)
    adxl_write_reg(0x2D, 0x02); 

    // 3. CALIBRATION PHASE
    // Screen turns Blue while calibrating. Keep the board flat!
    for(int i=0; i<300; i++) vram[i] = COLOR_BLUE; 
    
    int x_off = 0, y_off = 0, z_off = 0;
    int x, y, z;
    
    for (int i = 0; i < 100; i++) {
        read_xyz(&x, &y, &z);
        x_off += x; 
        y_off += y; 
        z_off += z;
        responsive_delay(10000);
    }
    x_off /= 100; 
    y_off /= 100; 
    z_off /= 100;

    // 4. MAIN LOOP
    while (1) {
        read_xyz(&x, &y, &z);

        // Apply calibration offset so "flat" is exactly 0
        x -= x_off;
        y -= y_off;

        // Map tilt to screen. Try changing '4' to a smaller number 
        // (like 2) if it's too sluggish, or larger (like 8) if it's too twitchy!
        int cursor_x = 9 + (x / 16); 
        int cursor_y = 7 + (y / 16);

        cursor_x = clamp(cursor_x, 0, 19);
        cursor_y = clamp(cursor_y, 0, 14);

        draw_background(); 
        vram[cursor_y * 20 + cursor_x] = COLOR_GREEN;

        responsive_delay(50000); 
    }

    return 0;
}
*/
#include <stdint.h>

// --- AXI Memory Map Addresses ---
#define VRAM_BASE 0x40000000
#define UART_BASE 0x20000000
#define SPI_BASE  0x30000000

volatile uint8_t* const vram       = (volatile uint8_t*)VRAM_BASE;
volatile uint32_t* const uart_data = (volatile uint32_t*)UART_BASE;
volatile uint32_t* const uart_stat = (volatile uint32_t*)(UART_BASE + 4);
volatile uint32_t* const spi_data  = (volatile uint32_t*)SPI_BASE;
volatile uint32_t* const spi_ctrl  = (volatile uint32_t*)(SPI_BASE + 4);

// --- Fast Pseudo-Random Number Generator ---
static uint32_t rand_state = 123456789;
uint32_t xorshift32() {
    rand_state ^= rand_state << 13;
    rand_state ^= rand_state >> 17;
    rand_state ^= rand_state << 5;
    return rand_state;
}

// --- Soft Reset & Delay ---
void software_reset() {
    void (*bootloader)(void) = (void (*)(void))0x00000000;
    bootloader();
}

void check_reboot() {
    if ((*uart_stat) & 0x02) { 
        if (((*uart_data) & 0xFF) == 0x7F) software_reset();
    }
}

void responsive_delay(int cycles) {
    for (volatile int i = 0; i < cycles; i++) check_reboot();
}

// --- SPI & ADXL362 FUNCTIONS ---
void wait_spi_ready() { while (((*spi_ctrl) & 0x01) == 0); }

unsigned int spi_transfer(unsigned int d) {
    wait_spi_ready();
    volatile unsigned int dummy = *spi_data; (void)dummy;
    *spi_data = d;
    while (((*spi_ctrl) & 0x02) == 0);
    return (*spi_data) & 0xFF;
}

void spi_cs_low()  { wait_spi_ready(); *spi_ctrl = 0x00; }
void spi_cs_high() { wait_spi_ready(); *spi_ctrl = 0x02; }

void adxl_write_reg(unsigned int reg, unsigned int val) {
    spi_cs_low(); spi_transfer(0x0A); spi_transfer(reg); spi_transfer(val); spi_cs_high();
}

void read_xyz(int *x, int *y, int *z) {
    spi_cs_low(); spi_transfer(0x0B); spi_transfer(0x08);
    *x = (signed char)spi_transfer(0x00);
    *y = (signed char)spi_transfer(0x00);
    *z = (signed char)spi_transfer(0x00);
    spi_cs_high();
}

// --- GRAPHICS & PHYSICS CONSTANTS ---
// We use a custom Fire Palette: Black -> Dim Red -> Bright Red -> Orange -> Yellow -> White
const uint8_t palette[6] = {0x00, 0x80, 0xE0, 0xEC, 0xFC, 0xFF};

// A local framebuffer to hold the "heat" of each pixel (0 to 5)
uint8_t framebuffer[300] = {0}; 

int main() {
    spi_cs_high();
    responsive_delay(100000);

    // 1. Init Sensor
    unsigned int id = 0;
    while (id != 0xAD) { 
        spi_cs_low(); spi_transfer(0x0B); spi_transfer(0x00); id = spi_transfer(0x00); spi_cs_high();
        for(int i=0; i<300; i++) vram[i] = 0xE0; // Red if missing
        responsive_delay(50000);
    }

    adxl_write_reg(0x2D, 0x02); // Enable ADXL362

    // 2. Calibrate
    for(int i=0; i<300; i++) vram[i] = 0x03; // Blue for calibration
    int x_off = 0, y_off = 0, z_off = 0;
    int x, y, z;
    for (int i = 0; i < 100; i++) {
        read_xyz(&x, &y, &z);
        x_off += x; y_off += y; z_off += z;
        responsive_delay(10000);
    }
    x_off /= 100; y_off /= 100; z_off /= 100;

    // 3. Physics Variables
    // We use "Fixed Point Math" (value * 256) so we can have smooth sub-pixel velocity!
    int pos_x = (9 << 8);  // Start in middle (x=9)
    int pos_y = (7 << 8);  // Start in middle (y=7)
    int vel_x = 0;
    int vel_y = 0;
    
    int prev_x = 0, prev_y = 0, prev_z = 0;

    // 4. MAIN LOOP
    while (1) {
        check_reboot();

        // Read Sensor
        read_xyz(&x, &y, &z);
        int clean_x = x - x_off;
        int clean_y = y - y_off;

        // -- SHAKE DETECTION --
        int dx = x - prev_x; int dy = y - prev_y; int dz = z - prev_z;
        int motion = dx*dx + dy*dy + dz*dz;
        
        if (motion > 2500) { 
            // EXPLOSION! Randomize the entire screen's heat
            for(int i=0; i<300; i++) framebuffer[i] = (xorshift32() % 6);
        }
        prev_x = x; prev_y = y; prev_z = z;

        // -- PHYSICS ENGINE --
        // Accelerometer acts as gravity
        vel_x += clean_x; 
        vel_y += clean_y;

        // Apply friction/air resistance (multiply by 0.98 essentially)
        vel_x = (vel_x * 250) >> 8; 
        vel_y = (vel_y * 250) >> 8;

        // Update position
        pos_x += vel_x;
        pos_y += vel_y;

        // Collision detection with screen boundaries (with bouncy energy loss)
        if (pos_x < 0) { pos_x = 0; vel_x = -vel_x / 2; }
        if (pos_x >= (19 << 8)) { pos_x = (19 << 8); vel_x = -vel_x / 2; }
        
        if (pos_y < 0) { pos_y = 0; vel_y = -vel_y / 2; }
        if (pos_y >= (14 << 8)) { pos_y = (14 << 8); vel_y = -vel_y / 2; }

        // -- GRAPHICS ENGINE --
        // 1. Cool down the trail
        for(int i=0; i<300; i++) {
            if (framebuffer[i] > 0) {
                // Randomly decay the pixels to create a flickering fire effect
                if ((xorshift32() & 0x03) != 0) { 
                    framebuffer[i]--; 
                }
            }
        }

        // 2. Draw the ball at maximum heat (5)
        int draw_x = pos_x >> 8;
        int draw_y = pos_y >> 8;
        framebuffer[draw_y * 20 + draw_x] = 5;

        // 3. Blit (Copy) the local framebuffer to the actual VGA VRAM using the color palette
        for(int i=0; i<300; i++) {
            vram[i] = palette[framebuffer[i]];
        }

        // Delay to set framerate (adjust to make it faster/slower)
        responsive_delay(30000); 
    }

    return 0;
}
