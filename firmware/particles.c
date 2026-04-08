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

// --- UART & Soft Reset Functions ---
void uart_print(const char* str) {
    while (*str) {
        while (!((*uart_stat) & 0x01)); // Wait for TX Ready
        *uart_data = *str++;
    }
}

void software_reset() {
    uart_print("\r\n[!] SOFT RESET INITIATED. Jumping to bootloader...\r\n");
    void (*bootloader)(void) = (void (*)(void))0x00000000;
    bootloader();
}

void check_reboot() {
    if ((*uart_stat) & 0x02) { // RX Valid
        if (((*uart_data) & 0xFF) == 0x7F) {
            software_reset();
        }
    }
}

// --- SPI & ADXL362 Accelerometer Functions ---
void spi_cs_low()  { while (((*spi_ctrl) & 0x01) == 0); *spi_ctrl = 0x00; }
void spi_cs_high() { while (((*spi_ctrl) & 0x01) == 0); *spi_ctrl = 0x02; }

unsigned int spi_transfer(unsigned int d) {
    volatile unsigned int dummy = *spi_data; (void)dummy;
    *spi_data = d;
    while (((*spi_ctrl) & 0x02) == 0);
    return (*spi_data) & 0xFF;
}

void adxl_write_reg(unsigned int reg, unsigned int val) {
    spi_cs_low(); 
    spi_transfer(0x0A); spi_transfer(reg); spi_transfer(val); 
    spi_cs_high();
}

void read_tilt(int *x, int *y) {
    spi_cs_low(); 
    spi_transfer(0x0B); spi_transfer(0x08);
    *x = (signed char)spi_transfer(0x00);
    *y = (signed char)spi_transfer(0x00);
    spi_transfer(0x00); // Read Z (discarding)
    spi_cs_high();
}

// --- Fast Pseudo-Random Generator ---
static uint32_t rand_state = 123456789;
uint32_t fast_rand() {
    rand_state ^= rand_state << 13;
    rand_state ^= rand_state >> 17;
    rand_state ^= rand_state << 5;
    return rand_state;
}

// --- Physics Sandbox Settings ---
#define NUM_PARTICLES 250
#define SHIFT 8 // Q24.8 Fixed Point math for smooth physics

int32_t p_x[NUM_PARTICLES];
int32_t p_y[NUM_PARTICLES];
int32_t v_x[NUM_PARTICLES];
int32_t v_y[NUM_PARTICLES];
uint8_t p_col[NUM_PARTICLES];

int32_t old_draw_x[NUM_PARTICLES];
int32_t old_draw_y[NUM_PARTICLES];

int main() {
    uart_print("\r\n*** TILT-GRAVITY PARTICLE ENGINE ***\r\n");

    // 1. Initialize SPI & Accelerometer
    spi_cs_high();
    for(volatile int i=0; i<100000; i++);
    adxl_write_reg(0x2D, 0x02); // Enable ADXL362 Measurement Mode

    // 2. Clear VRAM (Black screen)
    for(int i = 0; i < 76800; i++) vram[i] = 0x00;

    // 3. Spawn Particles Randomly
    for(int i = 0; i < NUM_PARTICLES; i++) {
        p_x[i] = (fast_rand() % 320) << SHIFT;
        p_y[i] = (fast_rand() % 240) << SHIFT;
        v_x[i] = (fast_rand() % 512) - 256; // Random starting velocity
        v_y[i] = (fast_rand() % 512) - 256;
        
        old_draw_x[i] = p_x[i] >> SHIFT;
        old_draw_y[i] = p_y[i] >> SHIFT;

        // Assign a random vibrant RGB332 color (avoiding black/dark colors)
        p_col[i] = (fast_rand() % 200) + 55; 
    }

    // 4. Main Physics Loop
    while (1) {
        int ax, ay;
        read_tilt(&ax, &ay);

        // Apply a small deadzone to the accelerometer
        if (ax > -3 && ax < 3) ax = 0;
        if (ay > -3 && ay < 3) ay = 0;

        // Scale real-world tilt into simulated gravity acceleration
        int32_t grav_x = ax * 2; 
        int32_t grav_y = -(ay * 2); // Inverted because screen Y goes down

        for(int i = 0; i < NUM_PARTICLES; i++) {
            
            // AGGRESSIVE POLLING: Check for reset continuously during physics updates
            check_reboot();

            // Erase old pixel directly (much faster than clearing the whole screen)
            vram[(old_draw_y[i] * 320) + old_draw_x[i]] = 0x00;

            // Apply gravity to velocity
            v_x[i] += grav_x;
            v_y[i] += grav_y;

            // Apply simulated air friction (slows things down slightly)
            v_x[i] = (v_x[i] * 254) >> 8; 
            v_y[i] = (v_y[i] * 254) >> 8;

            // Update position
            p_x[i] += v_x[i];
            p_y[i] += v_y[i];

            // Wall Collisions (Bounce with energy loss)
            if (p_x[i] <= 0) { 
                p_x[i] = 0; 
                v_x[i] = -(v_x[i] * 200) >> 8; // Bounce and lose 20% energy
            } 
            else if (p_x[i] >= (319 << SHIFT)) { 
                p_x[i] = (319 << SHIFT); 
                v_x[i] = -(v_x[i] * 200) >> 8; 
            }

            if (p_y[i] <= 0) { 
                p_y[i] = 0; 
                v_y[i] = -(v_y[i] * 200) >> 8; 
            } 
            else if (p_y[i] >= (239 << SHIFT)) { 
                p_y[i] = (239 << SHIFT); 
                v_y[i] = -(v_y[i] * 200) >> 8; 
            }

            // Calculate integer draw coordinates
            int draw_x = p_x[i] >> SHIFT;
            int draw_y = p_y[i] >> SHIFT;

            // Draw new pixel
            vram[(draw_y * 320) + draw_x] = p_col[i];

            // Save coordinates for the erase step next frame
            old_draw_x[i] = draw_x;
            old_draw_y[i] = draw_y;
        }

        // Simple delay to prevent the physics from running too fast on a 100MHz CPU
        for(volatile int d=0; d<100000; d++); 
    }
}