
/*
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

int main() {
    spi_cs_high();
    responsive_delay(100000);

    // Clear entire screen initially
    for(int i = 0; i < 76800; i++) vram[i] = 0x00;

    // 1. Init Sensor
    unsigned int id = 0;
    while (id != 0xAD) { 
        spi_cs_low(); spi_transfer(0x0B); spi_transfer(0x00); id = spi_transfer(0x00); spi_cs_high();
        responsive_delay(50000);
    }

    adxl_write_reg(0x2D, 0x02); // Enable ADXL362

    // 2. Calibrate
    int x_off = 0, y_off = 0, z_off = 0;
    int x, y, z;
    for (int i = 0; i < 100; i++) {
        read_xyz(&x, &y, &z);
        x_off += x; y_off += y; z_off += z;
        responsive_delay(10000);
    }
    x_off /= 100; y_off /= 100; z_off /= 100;

    // 3. Physics Variables (Scaled for 320x240 screen)
    int pos_x = (160 << 8);  // Start in middle (x=160)
    int pos_y = (120 << 8);  // Start in middle (y=120)
    int vel_x = 0;
    int vel_y = 0;
    
    int prev_x = 0, prev_y = 0, prev_z = 0;
    int prev_draw_x = 160, prev_draw_y = 120;

    // Filter variables for noise reduction
    int filtered_x = 0;
    int filtered_y = 0;

    // 4. MAIN LOOP
    while (1) {
        check_reboot();

        // Read Sensor
        read_xyz(&x, &y, &z);
        int raw_clean_x = x - x_off;
        int raw_clean_y = y - y_off;

        // Apply EMA Filter
        filtered_x = ((filtered_x * 7) + raw_clean_x) >> 3; 
        filtered_y = ((filtered_y * 7) + raw_clean_y) >> 3;

        int final_x = filtered_x;
        int final_y = filtered_y;
        if (final_x > -5 && final_x < 5) final_x = 0; 
        if (final_y > -5 && final_y < 5) final_y = 0;

        // -- SHAKE DETECTION --
        int dx = x - prev_x; int dy = y - prev_y; int dz = z - prev_z;
        int motion = dx*dx + dy*dy + dz*dz;
        
        if (motion > 2500) { 
            // EXPLOSION! Flash screen red, then clear
            for(int i=0; i<76800; i++) vram[i] = 0xE0; // Fast Red fill
            for(int i=0; i<76800; i++) vram[i] = 0x00; // Fast Black clear
        }
        prev_x = x; prev_y = y; prev_z = z;

        // -- PHYSICS ENGINE --
        vel_x += final_x; 
        vel_y += final_y;

        vel_x = (vel_x * 250) >> 8; 
        vel_y = (vel_y * 250) >> 8;

        pos_x += vel_x;
        pos_y += vel_y;

        // Collision detection scaled for 320x240
        if (pos_x < 0) { pos_x = 0; vel_x = -vel_x / 2; }
        if (pos_x >= (319 << 8)) { pos_x = (319 << 8); vel_x = -vel_x / 2; }
        
        if (pos_y < 0) { pos_y = 0; vel_y = -vel_y / 2; }
        if (pos_y >= (239 << 8)) { pos_y = (239 << 8); vel_y = -vel_y / 2; }

        // -- GRAPHICS ENGINE --
        int draw_x = pos_x >> 8;
        int draw_y = pos_y >> 8;

        // Erase old ball position directly in VRAM
        vram[(prev_draw_y * 320) + prev_draw_x] = 0x00; // Black

        // Draw new ball position (0xFF is solid White in RGB332)
        vram[(draw_y * 320) + draw_x] = 0xFF; 

        prev_draw_x = draw_x;
        prev_draw_y = draw_y;

        responsive_delay(30000); 
    }

    return 0;
}
 */
/*
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

// --- Graphics Settings ---
#define OBJ_SIZE 5
#define WALL_COLOR 0x1C  // Green in RGB332
#define BG_COLOR 0x00    // Black
#define OBJ_COLOR 0xFF   // White

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

// --- Maze Generation ---
void draw_maze() {
    // 1. Clear Screen
    for(int i = 0; i < 76800; i++) vram[i] = BG_COLOR;
    
    // 2. Draw Outer Borders (5 pixels thick)
    for(int x = 0; x < 320; x++) {
        for(int y=0; y<5; y++) vram[(y * 320) + x] = WALL_COLOR; // Top
        for(int y=235; y<240; y++) vram[(y * 320) + x] = WALL_COLOR; // Bottom
    }
    for(int y = 0; y < 240; y++) {
        for(int x=0; x<5; x++) vram[(y * 320) + x] = WALL_COLOR; // Left
        for(int x=315; x<320; x++) vram[(y * 320) + x] = WALL_COLOR; // Right
    }
    
    // 3. Draw Inner Obstacles
    // Obstacle 1 (Top Left)
    for(int y = 40; y < 100; y++) {
        for(int x = 60; x < 80; x++) vram[(y * 320) + x] = WALL_COLOR;
    }
    // Obstacle 2 (Top Right)
    for(int y = 40; y < 100; y++) {
        for(int x = 240; x < 260; x++) vram[(y * 320) + x] = WALL_COLOR;
    }
    // Obstacle 3 (Bottom Center)
    for(int y = 180; y < 200; y++) {
        for(int x = 100; x < 220; x++) vram[(y * 320) + x] = WALL_COLOR;
    }
}

int main() {
    spi_cs_high();
    responsive_delay(100000);

    draw_maze(); // Initialize graphics

    // 1. Init Sensor
    unsigned int id = 0;
    while (id != 0xAD) { 
        spi_cs_low(); spi_transfer(0x0B); spi_transfer(0x00); id = spi_transfer(0x00); spi_cs_high();
        responsive_delay(50000);
    }

    adxl_write_reg(0x2D, 0x02); // Enable ADXL362

    // 2. Calibrate
    int x_off = 0, y_off = 0, z_off = 0;
    int x, y, z;
    for (int i = 0; i < 100; i++) {
        read_xyz(&x, &y, &z);
        x_off += x; y_off += y; z_off += z;
        responsive_delay(10000);
    }
    x_off /= 100; y_off /= 100; z_off /= 100;

    // 3. Physics Variables (Scaled for 320x240 screen, fixed-point Q24.8)
    int pos_x = (160 << 8);  // Start safely in middle
    int pos_y = (120 << 8);  // Start safely in middle
    int vel_x = 0;
    int vel_y = 0;
    
    int prev_x = 0, prev_y = 0, prev_z = 0;
    int prev_draw_x = 160, prev_draw_y = 120;

    int filtered_x = 0;
    int filtered_y = 0;

    // 4. MAIN LOOP
    while (1) {
        check_reboot();

        // Read Sensor
        read_xyz(&x, &y, &z);
        int raw_clean_x = x - x_off;
        int raw_clean_y = y - y_off;

        // Apply EMA Filter
        filtered_x = ((filtered_x * 7) + raw_clean_x) >> 3; 
        filtered_y = ((filtered_y * 7) + raw_clean_y) >> 3;

        int final_x = filtered_x;
        int final_y = filtered_y;
        
        // Reduced deadzone for much higher sensitivity
        if (final_x > -2 && final_x < 2) final_x = 0; 
        if (final_y > -2 && final_y < 2) final_y = 0;

        // -- SHAKE DETECTION --
        int dx = x - prev_x; int dy = y - prev_y; int dz = z - prev_z;
        int motion = dx*dx + dy*dy + dz*dz;
        
        if (motion > 2500) { 
            // EXPLOSION! Flash screen red, delay, then rebuild maze
            for(int i=0; i<76800; i++) vram[i] = 0xE0; // Fast Red fill
            responsive_delay(200000); // Wait so it's visible
            draw_maze();              // Re-draw walls and clear background
        }
        prev_x = x; prev_y = y; prev_z = z;

        // -- PHYSICS ENGINE --
        // Multiplied input for faster movement. 
        // Inverted Y because screen coordinates draw downwards.
        vel_x += (final_x * 4); 
        vel_y -= (final_y * 4); 

        // Friction
        vel_x = (vel_x * 250) >> 8; 
        vel_y = (vel_y * 250) >> 8;

        int next_pos_x = pos_x + vel_x;
        int next_pos_y = pos_y + vel_y;
        
        int next_draw_x = next_pos_x >> 8;
        int next_draw_y = next_pos_y >> 8;
        
        int collision = 0;

        // Boundary + VRAM Collision Detection
        // Check if the bounding box of our object overlaps a screen edge OR a WALL_COLOR pixel
        if (next_draw_x < 0 || next_draw_x > (320 - OBJ_SIZE) || 
            next_draw_y < 0 || next_draw_y > (240 - OBJ_SIZE)) {
            collision = 1;
        } else {
            // Check the 4 corners of our moving block against VRAM colors
            if (vram[(next_draw_y * 320) + next_draw_x] == WALL_COLOR ||
                vram[(next_draw_y * 320) + (next_draw_x + OBJ_SIZE - 1)] == WALL_COLOR ||
                vram[((next_draw_y + OBJ_SIZE - 1) * 320) + next_draw_x] == WALL_COLOR ||
                vram[((next_draw_y + OBJ_SIZE - 1) * 320) + (next_draw_x + OBJ_SIZE - 1)] == WALL_COLOR) {
                collision = 1;
            }
        }

        if (collision) {
            vel_x = -vel_x / 2; // Bounce off the wall
            vel_y = -vel_y / 2;
        } else {
            pos_x = next_pos_x;
            pos_y = next_pos_y;
        }

        // -- GRAPHICS ENGINE --
        int draw_x = pos_x >> 8;
        int draw_y = pos_y >> 8;

        // Erase old object (fill with black)
        for(int i=0; i<OBJ_SIZE; i++) {
            for(int j=0; j<OBJ_SIZE; j++) {
                vram[((prev_draw_y + j) * 320) + (prev_draw_x + i)] = BG_COLOR;
            }
        }

        // Draw new object (fill with white)
        for(int i=0; i<OBJ_SIZE; i++) {
            for(int j=0; j<OBJ_SIZE; j++) {
                vram[((draw_y + j) * 320) + (draw_x + i)] = OBJ_COLOR; 
            }
        }

        prev_draw_x = draw_x;
        prev_draw_y = draw_y;

        responsive_delay(30000); 
    }

    return 0;
}
// */
// #include <stdint.h>

// // --- AXI Memory Map Addresses ---
// #define VRAM_BASE 0x40000000
// #define UART_BASE 0x20000000
// #define SPI_BASE  0x30000000

// volatile uint8_t* const vram       = (volatile uint8_t*)VRAM_BASE;
// volatile uint32_t* const uart_data = (volatile uint32_t*)UART_BASE;
// volatile uint32_t* const uart_stat = (volatile uint32_t*)(UART_BASE + 4);
// volatile uint32_t* const spi_data  = (volatile uint32_t*)SPI_BASE;
// volatile uint32_t* const spi_ctrl  = (volatile uint32_t*)(SPI_BASE + 4);

// // --- Graphics Settings ---
// #define OBJ_SIZE 5
// #define WALL_COLOR 0x1C  // Green in RGB332
// #define BG_COLOR 0x00    // Black
// #define OBJ_COLOR 0xFF   // White

// // --- Soft Reset & Delay ---
// void software_reset() {
//     void (*bootloader)(void) = (void (*)(void))0x00000000;
//     bootloader();
// }

// void check_reboot() {
//     if ((*uart_stat) & 0x02) { 
//         if (((*uart_data) & 0xFF) == 0x7F) software_reset();
//     }
// }

// void responsive_delay(int cycles) {
//     for (volatile int i = 0; i < cycles; i++) check_reboot();
// }

// // --- SPI & ADXL362 FUNCTIONS ---
// void wait_spi_ready() { while (((*spi_ctrl) & 0x01) == 0); }

// unsigned int spi_transfer(unsigned int d) {
//     wait_spi_ready();
//     volatile unsigned int dummy = *spi_data; (void)dummy;
//     *spi_data = d;
//     while (((*spi_ctrl) & 0x02) == 0);
//     return (*spi_data) & 0xFF;
// }

// void spi_cs_low()  { wait_spi_ready(); *spi_ctrl = 0x00; }
// void spi_cs_high() { wait_spi_ready(); *spi_ctrl = 0x02; }

// void adxl_write_reg(unsigned int reg, unsigned int val) {
//     spi_cs_low(); spi_transfer(0x0A); spi_transfer(reg); spi_transfer(val); spi_cs_high();
// }

// void read_xyz(int *x, int *y, int *z) {
//     spi_cs_low(); spi_transfer(0x0B); spi_transfer(0x08);
//     *x = (signed char)spi_transfer(0x00);
//     *y = (signed char)spi_transfer(0x00);
//     *z = (signed char)spi_transfer(0x00);
//     spi_cs_high();
// }

// // --- Maze Generation ---
// void draw_maze() {
//     // 1. Clear Screen
//     for(int i = 0; i < 76800; i++) vram[i] = BG_COLOR;
    
//     // 2. Draw Outer Borders (5 pixels thick)
//     for(int x = 0; x < 320; x++) {
//         for(int y=0; y<5; y++) vram[(y * 320) + x] = WALL_COLOR; // Top
//         for(int y=235; y<240; y++) vram[(y * 320) + x] = WALL_COLOR; // Bottom
//     }
//     for(int y = 0; y < 240; y++) {
//         for(int x=0; x<5; x++) vram[(y * 320) + x] = WALL_COLOR; // Left
//         for(int x=315; x<320; x++) vram[(y * 320) + x] = WALL_COLOR; // Right
//     }
    
//     // 3. Draw Inner Obstacles
//     for(int y = 40; y < 100; y++) {
//         for(int x = 60; x < 80; x++) vram[(y * 320) + x] = WALL_COLOR;
//     }
//     for(int y = 40; y < 100; y++) {
//         for(int x = 240; x < 260; x++) vram[(y * 320) + x] = WALL_COLOR;
//     }
//     for(int y = 180; y < 200; y++) {
//         for(int x = 100; x < 220; x++) vram[(y * 320) + x] = WALL_COLOR;
//     }
// }

// int main() {
//     spi_cs_high();
//     responsive_delay(100000);

//     draw_maze(); // Initialize graphics once. It will never be erased.

//     // 1. Init Sensor
//     unsigned int id = 0;
//     while (id != 0xAD) { 
//         spi_cs_low(); spi_transfer(0x0B); spi_transfer(0x00); id = spi_transfer(0x00); spi_cs_high();
//         responsive_delay(50000);
//     }

//     adxl_write_reg(0x2D, 0x02); // Enable ADXL362

//     // 2. Calibrate
//     int x_off = 0, y_off = 0, z_off = 0;
//     int x, y, z;
//     for (int i = 0; i < 100; i++) {
//         read_xyz(&x, &y, &z);
//         x_off += x; y_off += y; z_off += z;
//         responsive_delay(10000);
//     }
//     x_off /= 100; y_off /= 100; z_off /= 100;

//     // 3. Physics Variables 
//     int pos_x = (160 << 8);  
//     int pos_y = (120 << 8);  
//     int vel_x = 0;
//     int vel_y = 0;
//     int prev_draw_x = 160, prev_draw_y = 120;

//     // High-resolution filter accumulators
//     int filter_x_accum = 0;
//     int filter_y_accum = 0;

//     // Initialize previous values correctly to prevent frame-1 false shakes
//     read_xyz(&x, &y, &z);
//     int prev_x = x, prev_y = y, prev_z = z;

//     // 4. MAIN LOOP
//     while (1) {
//         check_reboot();

//         // Read Sensor
//         read_xyz(&x, &y, &z);
//         int raw_clean_x = x - x_off;
//         int raw_clean_y = y - y_off;

//         // -- HIGH-PRECISION ACCUMULATOR FILTER --
//         // This prevents integer division from swallowing small tilt values.
//         filter_x_accum = filter_x_accum - (filter_x_accum / 8) + raw_clean_x;
//         filter_y_accum = filter_y_accum - (filter_y_accum / 8) + raw_clean_y;

//         int final_x = filter_x_accum / 8;
//         int final_y = filter_y_accum / 8;
        
//         // Deadzone (Applied after filtering for maximum smoothness)
//         if (final_x >= -2 && final_x <= 2) final_x = 0; 
//         if (final_y >= -2 && final_y <= 2) final_y = 0;

//         // -- SHAKE DETECTION --
//         int dx = x - prev_x; int dy = y - prev_y; int dz = z - prev_z;
//         int motion = dx*dx + dy*dy + dz*dz;
        
//         // If shaken hard, teleport to center instead of ruining the background
//         if (motion > 4000) { 
//             // Erase current object
//             for(int i=0; i<OBJ_SIZE; i++) {
//                 for(int j=0; j<OBJ_SIZE; j++) {
//                     vram[((prev_draw_y + j) * 320) + (prev_draw_x + i)] = BG_COLOR;
//                 }
//             }
//             // Reset physics
//             pos_x = (160 << 8);
//             pos_y = (120 << 8);
//             vel_x = 0; 
//             vel_y = 0;
//             filter_x_accum = 0;
//             filter_y_accum = 0;
//         }
//         prev_x = x; prev_y = y; prev_z = z;

//         // -- PHYSICS ENGINE --
//         vel_x += (final_x * 4); 
//         vel_y -= (final_y * 4); // Invert Y because screen coordinates draw downwards

//         // Air Friction
//         vel_x = (vel_x * 250) >> 8; 
//         vel_y = (vel_y * 250) >> 8;

//         int next_pos_x = pos_x + vel_x;
//         int next_pos_y = pos_y + vel_y;
        
//         int next_draw_x = next_pos_x >> 8;
//         int next_draw_y = next_pos_y >> 8;
        
//         int collision = 0;

//         // Boundary + VRAM Collision Detection
//         if (next_draw_x < 0 || next_draw_x > (320 - OBJ_SIZE) || 
//             next_draw_y < 0 || next_draw_y > (240 - OBJ_SIZE)) {
//             collision = 1;
//         } else {
//             // Check the corners of our block against the background colors
//             if (vram[(next_draw_y * 320) + next_draw_x] == WALL_COLOR ||
//                 vram[(next_draw_y * 320) + (next_draw_x + OBJ_SIZE - 1)] == WALL_COLOR ||
//                 vram[((next_draw_y + OBJ_SIZE - 1) * 320) + next_draw_x] == WALL_COLOR ||
//                 vram[((next_draw_y + OBJ_SIZE - 1) * 320) + (next_draw_x + OBJ_SIZE - 1)] == WALL_COLOR) {
//                 collision = 1;
//             }
//         }

//         if (collision) {
//             vel_x = -vel_x / 2; // Bounce off
//             vel_y = -vel_y / 2;
//         } else {
//             pos_x = next_pos_x;
//             pos_y = next_pos_y;
//         }

//         // -- GRAPHICS ENGINE --
//         int draw_x = pos_x >> 8;
//         int draw_y = pos_y >> 8;

//         // Only redraw if the object actually moved to save VRAM bandwidth
//         if (draw_x != prev_draw_x || draw_y != prev_draw_y) {
//             // Erase old object
//             for(int i=0; i<OBJ_SIZE; i++) {
//                 for(int j=0; j<OBJ_SIZE; j++) {
//                     vram[((prev_draw_y + j) * 320) + (prev_draw_x + i)] = BG_COLOR;
//                 }
//             }

//             // Draw new object
//             for(int i=0; i<OBJ_SIZE; i++) {
//                 for(int j=0; j<OBJ_SIZE; j++) {
//                     vram[((draw_y + j) * 320) + (draw_x + i)] = OBJ_COLOR; 
//                 }
//             }

//             prev_draw_x = draw_x;
//             prev_draw_y = draw_y;
//         }

//         responsive_delay(30000); 
//     }

//     return 0;
// }
/*
#include <stdint.h>

// --- AXI Memory Map Addresses ---
#define VRAM_BASE 0x40000000
#define UART_BASE 0x20000000
#define I2C_BASE  0x50000000

volatile uint8_t* const vram      = (volatile uint8_t*)VRAM_BASE;
volatile uint32_t* const uart_data = (volatile uint32_t*)UART_BASE;
volatile uint32_t* const uart_stat = (volatile uint32_t*)(UART_BASE + 4);

// I2C Registers
volatile uint32_t* const i2c_ctrl  = (volatile uint32_t*)(I2C_BASE + 0x00);
volatile uint32_t* const i2c_addr  = (volatile uint32_t*)(I2C_BASE + 0x04);
volatile uint32_t* const i2c_sub   = (volatile uint32_t*)(I2C_BASE + 0x08);
volatile uint32_t* const i2c_len   = (volatile uint32_t*)(I2C_BASE + 0x0C);
volatile uint32_t* const i2c_wdata = (volatile uint32_t*)(I2C_BASE + 0x10);
volatile uint32_t* const i2c_rdata = (volatile uint32_t*)(I2C_BASE + 0x14);

// --- Standard 8-bit VGA Colors (RRRGGGBB) ---
#define COLOR_BLACK   0x00
#define COLOR_DARK    0x48  // Dark gray/grid
#define COLOR_CYAN    0x1B  // Blue-ish cool
#define COLOR_YELLOW  0xFC  // Warning
#define COLOR_ORANGE  0xEC  // Hot
#define COLOR_RED     0xE0  // Critical
#define COLOR_WHITE   0xFF

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

// --- I2C & ADT7420 FUNCTIONS ---
int get_temperature_highres() {
    // Wait for I2C master to be idle (busy bit 0)
    while ((*i2c_ctrl) & 0x01); 

    *i2c_addr = 0x97;  // ADT7420 address (0x4B << 1) | 1 (Read)
    *i2c_sub  = 0x00;  // Temp Value Register
    *i2c_len  = 2;     // Read 2 bytes
    *i2c_ctrl = 0x01;  // req_trans = 1, sub_len = 0

    // Wait for MSB valid_out pulse
    while (((*i2c_ctrl) & 0x04) == 0);
    uint8_t msb = *i2c_rdata;

    // Wait for LSB valid_out pulse
    while (((*i2c_ctrl) & 0x04) == 0);
    uint8_t lsb = *i2c_rdata;

    // Wait for transaction to officially complete
    while ((*i2c_ctrl) & 0x01);

    // ADT7420 13-bit format: 1 LSB = 1/16th degree Celsius
    int16_t raw_temp = (msb << 8) | lsb;
    return raw_temp / 8; // Keep in 1/16ths resolution
}

// --- UI RENDER FUNCTION ---
void draw_reactor_ui(int temp_hr, int frame_count) {
    // ULTRA-SENSITIVE MATH:
    // 1 block = exactly 0.0625°C.
    // The base of the reactor is hardcoded to 24.0°C (24 * 16).
    // If your room is hotter/colder, adjust the "24" below to match your baseline!
    int blocks = temp_hr - (24 * 16); 
    
    if (blocks < 0) blocks = 0;
    if (blocks > 13) blocks = 13; // Max height inside the 15-row screen

    // Determine the Core Color and Alarm State
    uint8_t core_color;
    uint8_t wall_color = COLOR_DARK;
    int critical_alarm = 0;

    // Because it is so sensitive, the color changes happen fast
    if (blocks < 4) {
        core_color = COLOR_CYAN;   // < 24.25°C
    } else if (blocks < 9) {
        core_color = COLOR_ORANGE; // 24.25°C to 24.56°C
    } else {
        core_color = COLOR_RED;    // >= 24.56°C
        critical_alarm = 1;
    }

    // Flash the containment walls Red if Critical
    if (critical_alarm && (frame_count & 0x04)) { 
        wall_color = COLOR_RED; 
    }

    // Loop through the entire 20x15 screen
    for (int y = 0; y < 15; y++) {
        for (int x = 0; x < 20; x++) {
            int idx = y * 20 + x;

            // Draw the outer "Containment Vessel" Walls
            if ((x == 6 || x == 13) && y >= 1 && y <= 14) {
                vram[idx] = wall_color;
            }
            // Draw the top/bottom caps of the vessel
            else if ((y == 1 || y == 14) && x > 6 && x < 13) {
                vram[idx] = wall_color;
            }
            // Draw the active Fluid/Heat Core
            else if (x > 6 && x < 13 && y > 1 && y < 14) {
                if ((14 - y) <= blocks) {
                    vram[idx] = core_color; // Filled space
                } else {
                    vram[idx] = COLOR_BLACK; // Empty space
                }
            }
            // Background Void
            else {
                // Faint static dot grid pattern to the background
                if ((x % 2 == 0) && (y % 2 == 0)) {
                    vram[idx] = 0x24; // Very faint grey
                } else {
                    vram[idx] = COLOR_BLACK;
                }
            }
        }
    }
}

// --- MAIN FIRMWARE ---
int main() {
    responsive_delay(100000); // Give I2C time to stabilize on boot

    int frame_counter = 0;
    int current_temp_hr = 24 * 16; 

    while (1) {
        check_reboot();

        // Read I2C Temp every ~10 frames so we don't spam the bus
        if (frame_counter % 10 == 0) {
            current_temp_hr = get_temperature_highres();
        }

        // Draw the visual UI
        draw_reactor_ui(current_temp_hr, frame_counter);

        frame_counter++;
        responsive_delay(30000); // Frame delay
    }

    return 0;
}
*/
// #include <stdint.h>

// // --- AXI Memory Map Addresses ---
// #define VRAM_BASE 0x40000000
// #define UART_BASE 0x20000000
// #define SPI_BASE  0x30000000

// volatile uint8_t* const vram       = (volatile uint8_t*)VRAM_BASE;
// volatile uint32_t* const uart_data = (volatile uint32_t*)UART_BASE;
// volatile uint32_t* const uart_stat = (volatile uint32_t*)(UART_BASE + 4);
// volatile uint32_t* const spi_data  = (volatile uint32_t*)SPI_BASE;
// volatile uint32_t* const spi_ctrl  = (volatile uint32_t*)(SPI_BASE + 4);

// // --- Game & Graphics Settings ---
// #define OBJ_SIZE 5
// #define BG_COLOR   0x00  // Black
// #define WALL_COLOR 0x1F  // Cyan (Very attractive retro arcade color)
// #define OBJ_COLOR  0xFF  // White
// #define GOAL_COLOR 0x1C  // Green

// // Global Game State
// int current_level = 1;
// int goal_x, goal_y, goal_w, goal_h;
// int spawn_x, spawn_y;

// // --- Soft Reset & Delay ---
// void software_reset() {
//     void (*bootloader)(void) = (void (*)(void))0x00000000;
//     bootloader();
// }

// void check_reboot() {
//     if ((*uart_stat) & 0x02) { 
//         if (((*uart_data) & 0xFF) == 0x7F) software_reset();
//     }
// }

// void responsive_delay(int cycles) {
//     for (volatile int i = 0; i < cycles; i++) check_reboot();
// }

// // --- SPI & ADXL362 FUNCTIONS ---
// void wait_spi_ready() { while (((*spi_ctrl) & 0x01) == 0); }

// unsigned int spi_transfer(unsigned int d) {
//     wait_spi_ready();
//     volatile unsigned int dummy = *spi_data; (void)dummy;
//     *spi_data = d;
//     while (((*spi_ctrl) & 0x02) == 0);
//     return (*spi_data) & 0xFF;
// }

// void spi_cs_low()  { wait_spi_ready(); *spi_ctrl = 0x00; }
// void spi_cs_high() { wait_spi_ready(); *spi_ctrl = 0x02; }

// void adxl_write_reg(unsigned int reg, unsigned int val) {
//     spi_cs_low(); spi_transfer(0x0A); spi_transfer(reg); spi_transfer(val); spi_cs_high();
// }

// void read_xyz(int *x, int *y, int *z) {
//     spi_cs_low(); spi_transfer(0x0B); spi_transfer(0x08);
//     *x = (signed char)spi_transfer(0x00);
//     *y = (signed char)spi_transfer(0x00);
//     *z = (signed char)spi_transfer(0x00);
//     spi_cs_high();
// }

// // --- Graphics Helper ---
// void draw_rect(int x, int y, int w, int h, uint8_t color) {
//     for (int row = y; row < y + h; row++) {
//         if (row < 0 || row >= 240) continue;
//         for (int col = x; col < x + w; col++) {
//             if (col < 0 || col >= 320) continue;
//             vram[(row * 320) + col] = color;
//         }
//     }
// }

// // --- Level Design ---
// void load_level(int level) {
//     // 1. Clear Screen
//     for(int i = 0; i < 76800; i++) vram[i] = BG_COLOR;
    
//     // 2. Outer Borders
//     draw_rect(0, 0, 320, 5, WALL_COLOR);     // Top
//     draw_rect(0, 235, 320, 5, WALL_COLOR);   // Bottom
//     draw_rect(0, 0, 5, 240, WALL_COLOR);     // Left
//     draw_rect(315, 0, 5, 240, WALL_COLOR);   // Right

//     if (level == 1) {
//         // LEVEL 1: A simple "C" shape track
//         spawn_x = 20; spawn_y = 20;
        
//         // Define Goal
//         goal_x = 280; goal_y = 200; goal_w = 30; goal_h = 30;
        
//         // Inner Walls
//         draw_rect(60, 0, 20, 180, WALL_COLOR);   // First vertical divider
//         draw_rect(140, 60, 20, 180, WALL_COLOR); // Second vertical divider
//         draw_rect(220, 0, 20, 180, WALL_COLOR);  // Third vertical divider
        
//     } else if (level == 2) {
//         // LEVEL 2: Tight corridors and a central box
//         spawn_x = 20; spawn_y = 200;
        
//         // Define Goal
//         goal_x = 145; goal_y = 105; goal_w = 30; goal_h = 30;
        
//         // Walls
//         draw_rect(50, 50, 220, 10, WALL_COLOR);   // Top inner ring
//         draw_rect(50, 180, 220, 10, WALL_COLOR);  // Bottom inner ring
//         draw_rect(50, 50, 10, 140, WALL_COLOR);   // Left inner ring
//         draw_rect(260, 50, 10, 100, WALL_COLOR);  // Right inner ring (gap at bottom)
        
//         // Central Box around goal
//         draw_rect(110, 100, 100, 10, WALL_COLOR);
//         draw_rect(110, 130, 100, 10, WALL_COLOR); // Gap on sides
//     } else {
//         // WIN SCREEN!
//         spawn_x = 160; spawn_y = 120;
//         goal_x = 0; goal_y = 0; goal_w = 0; goal_h = 0; // Remove goal
//         // Fill screen with green
//         for(int i = 0; i < 76800; i++) vram[i] = GOAL_COLOR;
//     }

//     // Draw Goal Pad
//     if (level <= 2) {
//         draw_rect(goal_x, goal_y, goal_w, goal_h, GOAL_COLOR);
//     }
// }

// int main() {
//     spi_cs_high();
//     responsive_delay(100000);

//     // 1. Init Sensor
//     unsigned int id = 0;
//     while (id != 0xAD) { 
//         spi_cs_low(); spi_transfer(0x0B); spi_transfer(0x00); id = spi_transfer(0x00); spi_cs_high();
//         responsive_delay(50000);
//     }
//     adxl_write_reg(0x2D, 0x02); // Enable ADXL362

//     // 2. Calibrate
//     int x_off = 0, y_off = 0, z_off = 0;
//     int x, y, z;
//     for (int i = 0; i < 100; i++) {
//         read_xyz(&x, &y, &z);
//         x_off += x; y_off += y; z_off += z;
//         responsive_delay(10000);
//     }
//     x_off /= 100; y_off /= 100; z_off /= 100;

//     // Load initial level
//     current_level = 1;
//     load_level(current_level);

//     // 3. Physics Variables (Q24.8)
//     int pos_x = (spawn_x << 8);  
//     int pos_y = (spawn_y << 8);  
//     int vel_x = 0;
//     int vel_y = 0;
//     int prev_draw_x = spawn_x, prev_draw_y = spawn_y;

//     int filter_x_accum = 0;
//     int filter_y_accum = 0;
    
//     read_xyz(&x, &y, &z);
//     int prev_x = x, prev_y = y, prev_z = z;
//     int frame_counter = 0;

//     // 4. MAIN LOOP
//     while (1) {
//         check_reboot();

//         // Read Sensor
//         read_xyz(&x, &y, &z);
//         int raw_clean_x = x - x_off;
//         int raw_clean_y = y - y_off;

//         // Apply EMA Accumulator Filter
//         filter_x_accum = filter_x_accum - (filter_x_accum / 8) + raw_clean_x;
//         filter_y_accum = filter_y_accum - (filter_y_accum / 8) + raw_clean_y;

//         int final_x = filter_x_accum / 8;
//         int final_y = filter_y_accum / 8;
        
//         // Deadzone
//         if (final_x >= -2 && final_x <= 2) final_x = 0; 
//         if (final_y >= -2 && final_y <= 2) final_y = 0;

//         // -- SHAKE RESET -- (Teleport to spawn on shake)
//         int dx = x - prev_x; int dy = y - prev_y; int dz = z - prev_z;
//         if ((dx*dx + dy*dy + dz*dz) > 4000) { 
//             draw_rect(prev_draw_x, prev_draw_y, OBJ_SIZE, OBJ_SIZE, BG_COLOR); // Erase
//             pos_x = (spawn_x << 8); pos_y = (spawn_y << 8);
//             vel_x = 0; vel_y = 0;
//             filter_x_accum = 0; filter_y_accum = 0;
//         }
//         prev_x = x; prev_y = y; prev_z = z;

//         // -- PHYSICS ENGINE --
//         // Reduced sensitivity: multiplier changed from 4 to 2
//         vel_x += (final_x * 2); 
//         vel_y -= (final_y * 2); 

//         // Increased Friction: multiplier changed from 250 to 220
//         vel_x = (vel_x * 220) >> 8; 
//         vel_y = (vel_y * 220) >> 8;

//         int next_pos_x = pos_x + vel_x;
//         int next_pos_y = pos_y + vel_y;
        
//         int next_draw_x = next_pos_x >> 8;
//         int next_draw_y = next_pos_y >> 8;
        
//         int collision = 0;

//         // Boundary + VRAM Collision Detection
//         if (next_draw_x < 0 || next_draw_x > (320 - OBJ_SIZE) || 
//             next_draw_y < 0 || next_draw_y > (240 - OBJ_SIZE)) {
//             collision = 1;
//         } else {
//             // Check only against WALL_COLOR (Allows moving over GOAL_COLOR)
//             if (vram[(next_draw_y * 320) + next_draw_x] == WALL_COLOR ||
//                 vram[(next_draw_y * 320) + (next_draw_x + OBJ_SIZE - 1)] == WALL_COLOR ||
//                 vram[((next_draw_y + OBJ_SIZE - 1) * 320) + next_draw_x] == WALL_COLOR ||
//                 vram[((next_draw_y + OBJ_SIZE - 1) * 320) + (next_draw_x + OBJ_SIZE - 1)] == WALL_COLOR) {
//                 collision = 1;
//             }
//         }

//         if (collision) {
//             vel_x = -vel_x / 2; // Bounce
//             vel_y = -vel_y / 2;
//         } else {
//             pos_x = next_pos_x;
//             pos_y = next_pos_y;
//         }

//         int draw_x = pos_x >> 8;
//         int draw_y = pos_y >> 8;

//         // -- GOAL DETECTION --
//         // Check if the center of the player is inside the goal area
//         if (current_level <= 2) {
//             int center_x = draw_x + (OBJ_SIZE / 2);
//             int center_y = draw_y + (OBJ_SIZE / 2);
            
//             if (center_x >= goal_x && center_x <= (goal_x + goal_w) &&
//                 center_y >= goal_y && center_y <= (goal_y + goal_h)) {
                
//                 current_level++;
//                 load_level(current_level);
                
//                 pos_x = (spawn_x << 8); pos_y = (spawn_y << 8);
//                 vel_x = 0; vel_y = 0;
//                 filter_x_accum = 0; filter_y_accum = 0;
//                 draw_x = spawn_x; draw_y = spawn_y;
//                 prev_draw_x = spawn_x; prev_draw_y = spawn_y;
//             }
//         }

//         // -- GRAPHICS ENGINE --
//         if (draw_x != prev_draw_x || draw_y != prev_draw_y) {
            
//             // Erase old object
//             // If the old object was over the goal, redraw the goal color, else black
//             for(int i=0; i<OBJ_SIZE; i++) {
//                 for(int j=0; j<OBJ_SIZE; j++) {
//                     int px = prev_draw_x + i;
//                     int py = prev_draw_y + j;
//                     if (px >= goal_x && px < goal_x + goal_w && py >= goal_y && py < goal_y + goal_h) {
//                         vram[(py * 320) + px] = GOAL_COLOR;
//                     } else {
//                         vram[(py * 320) + px] = BG_COLOR;
//                     }
//                 }
//             }

//             // Draw new object
//             draw_rect(draw_x, draw_y, OBJ_SIZE, OBJ_SIZE, OBJ_COLOR);
            
//             prev_draw_x = draw_x;
//             prev_draw_y = draw_y;
//         }

//         // Make goal blink slightly for aesthetics
//         frame_counter++;
//         if (current_level <= 2 && (frame_counter % 20 == 0)) {
//             // Just redraw a pixel in the corner of the goal to show it's active
//             vram[(goal_y * 320) + goal_x] = (frame_counter % 40 == 0) ? OBJ_COLOR : GOAL_COLOR;
//         }

//         responsive_delay(30000); 
//     }

//     return 0;
// }
// #include <stdint.h>

// // --- AXI Memory Map Addresses ---
// #define VRAM_BASE 0x40000000
// #define UART_BASE 0x20000000
// #define I2C_BASE  0x50000000

// volatile uint8_t* const vram       = (volatile uint8_t*)VRAM_BASE;
// volatile uint32_t* const uart_data = (volatile uint32_t*)UART_BASE;
// volatile uint32_t* const uart_stat = (volatile uint32_t*)(UART_BASE + 4);

// // --- I2C Registers ---
// volatile uint32_t* const i2c_ctrl  = (volatile uint32_t*)(I2C_BASE + 0x00);
// volatile uint32_t* const i2c_addr  = (volatile uint32_t*)(I2C_BASE + 0x04);
// volatile uint32_t* const i2c_sub   = (volatile uint32_t*)(I2C_BASE + 0x08);
// volatile uint32_t* const i2c_len   = (volatile uint32_t*)(I2C_BASE + 0x0C);
// volatile uint32_t* const i2c_wdata = (volatile uint32_t*)(I2C_BASE + 0x10);
// volatile uint32_t* const i2c_rdata = (volatile uint32_t*)(I2C_BASE + 0x14);

// // --- Soft Reset & Delay ---
// void software_reset() {
//     void (*bootloader)(void) = (void (*)(void))0x00000000;
//     bootloader();
// }

// void check_reboot() {
//     if ((*uart_stat) & 0x02) { 
//         if (((*uart_data) & 0xFF) == 0x7F) software_reset();
//     }
// }

// void responsive_delay(int cycles) {
//     for (volatile int i = 0; i < cycles; i++) check_reboot();
// }

// // --- I2C & ADT7420 FUNCTIONS ---
// int get_temperature_highres() {
//     // Wait for I2C idle
//     while ((*i2c_ctrl) & 0x01); 

//     *i2c_addr = 0x97;  // ADT7420 read address
//     *i2c_sub  = 0x00;  // Temp Value Reg
//     *i2c_len  = 2;     // 2 bytes
//     *i2c_ctrl = 0x01;  // Trigger read

//     // Wait for MSB valid
//     while (((*i2c_ctrl) & 0x04) == 0);
//     uint8_t msb = *i2c_rdata;

//     // Wait for LSB valid
//     while (((*i2c_ctrl) & 0x04) == 0);
//     uint8_t lsb = *i2c_rdata;

//     // Wait for transaction complete
//     while ((*i2c_ctrl) & 0x01);

//     int16_t raw_temp = (msb << 8) | lsb;
//     return raw_temp / 8; // 1/16ths resolution
// }

// // --- MAIN FIRMWARE ---
// int main() {
//     // 1. Wait for the ADT7420 to fully wake up on boot
//     responsive_delay(500000); 

//     // 2. Grab the baseline room temperature
//     int base_temp = get_temperature_highres(); 

//     while (1) {
//         check_reboot();

//         // Read current temp
//         int current_temp = get_temperature_highres();

//         // 3. Calculate Fill (1 block = 0.0625 C)
//         // Start with 3 blocks of visible liquid at baseline temp
//         int fill = (current_temp - base_temp) + 3; 
//         if (fill < 0) fill = 0;
//         if (fill > 15) fill = 15;

//         // 4. Color logic based on height
//         uint8_t color = 0x1C; // Green (Safe)
//         if (fill > 7) color = 0xFC;  // Yellow (Warm)
//         if (fill > 11) color = 0xE0; // Red (Hot)

//         // 5. Draw the massive centered thermometer
//         for (int y = 0; y < 15; y++) {
//             for (int x = 0; x < 20; x++) {
//                 int idx = y * 20 + x;

//                 // Center 4 columns (x = 8, 9, 10, 11)
//                 if (x >= 8 && x <= 11) {
//                     if (14 - y < fill) {
//                         vram[idx] = color; // Filled 
//                     } else {
//                         vram[idx] = 0x48;  // Empty (Dim Gray)
//                     }
//                 } else {
//                     vram[idx] = 0x00; // Black background
//                 }
//             }
//         }

//         // Delay to prevent spamming the I2C bus
//         responsive_delay(30000); 
//     }

//     return 0;
//}

// #include <stdint.h>

// #define VRAM_BASE 0x40000000
// #define UART_BASE 0x20000000
// #define SPI_BASE  0x30000000

// volatile uint8_t* const vram       = (volatile uint8_t*)VRAM_BASE;
// volatile uint32_t* const uart_data = (volatile uint32_t*)UART_BASE;
// volatile uint32_t* const spi_data  = (volatile uint32_t*)SPI_BASE;
// volatile uint32_t* const spi_ctrl  = (volatile uint32_t*)(SPI_BASE + 4);

// #define BG_COLOR    0x00 // Black
// #define WALL_COLOR  0x1F // Cyan
// #define HEAD_COLOR  0xFC // Yellow
// #define BODY_COLOR  0x1C // Green
// #define APPLE_COLOR 0xE0 // Red

// #define GRID_SIZE 4
// #define MAX_SNAKE 150 

// int16_t snk_x[MAX_SNAKE], snk_y[MAX_SNAKE];
// int snk_len, snk_dir_x, snk_dir_y;
// int apple_x, apple_y, score, level;

// // --- UART & SPI Helpers ---
// void uart_print(const char* str) {
//     while (*str) {
//         *uart_data = *str++;
//         for(volatile int i=0; i<5000; i++); // Wait for TX
//     }
// }

// void uart_print_num(int num) {
//     char buf[10]; int i = 0;
//     if (num == 0) buf[i++] = '0';
//     while (num > 0) { buf[i++] = (num % 10) + '0'; num /= 10; }
//     while (i > 0) { *uart_data = buf[--i]; for(volatile int d=0; d<5000; d++); }
//     uart_print("\r\n");
// }

// void spi_cs_low()  { while (((*spi_ctrl) & 0x01) == 0); *spi_ctrl = 0x00; }
// void spi_cs_high() { while (((*spi_ctrl) & 0x01) == 0); *spi_ctrl = 0x02; }
// unsigned int spi_transfer(unsigned int d) {
//     volatile unsigned int dummy = *spi_data; (void)dummy;
//     *spi_data = d;
//     while (((*spi_ctrl) & 0x02) == 0);
//     return (*spi_data) & 0xFF;
// }
// void read_tilt(int *x, int *y) {
//     spi_cs_low(); spi_transfer(0x0B); spi_transfer(0x08);
//     *x = (signed char)spi_transfer(0x00);
//     *y = (signed char)spi_transfer(0x00);
//     spi_transfer(0x00); 
//     spi_cs_high();
// }

// // --- Graphics & Logic ---
// void draw_block(int x, int y, uint8_t color) {
//     for (int r = 0; r < GRID_SIZE; r++)
//         for (int c = 0; c < GRID_SIZE; c++)
//             vram[((y + r) * 320) + (x + c)] = color;
// }

// // Pseudo-random generator for apple placement
// static uint32_t seed = 12345;
// int rand_pos(int max) {
//     seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF;
//     return (seed % (max / GRID_SIZE)) * GRID_SIZE;
// }

// void spawn_apple() {
//     while(1) {
//         apple_x = rand_pos(300) + 10;
//         apple_y = rand_pos(220) + 10;
//         if (vram[(apple_y * 320) + apple_x] == BG_COLOR) break; // Find empty spot
//     }
//     draw_block(apple_x, apple_y, APPLE_COLOR);
// }

// void build_maze(int lvl) {
//     for(int i = 0; i < 76800; i++) vram[i] = BG_COLOR;
    
//     // Outer Border (Lethal)
//     for(int x = 0; x < 320; x += GRID_SIZE) { draw_block(x, 0, WALL_COLOR); draw_block(x, 240-GRID_SIZE, WALL_COLOR); }
//     for(int y = 0; y < 240; y += GRID_SIZE) { draw_block(0, y, WALL_COLOR); draw_block(320-GRID_SIZE, y, WALL_COLOR); }

//     // Inner Maze (Non-Lethal Barriers)
//     if (lvl == 1) {
//         // City Blocks
//         for(int x = 40; x < 280; x += 60)
//             for(int y = 40; y < 200; y += 60)
//                 for(int i=0; i<20; i+=GRID_SIZE) draw_block(x+i, y, WALL_COLOR);
//     } else {
//         // Tight Corridors
//         for(int x = 40; x < 280; x += GRID_SIZE) draw_block(x, 60, WALL_COLOR);
//         for(int x = 80; x < 320; x += GRID_SIZE) draw_block(x, 120, WALL_COLOR);
//         for(int x = 40; x < 280; x += GRID_SIZE) draw_block(x, 180, WALL_COLOR);
//     }
//     spawn_apple();
// }

// void reset_game() {
//     uart_print("--- NEW GAME (Level "); uart_print_num(level); uart_print(") ---\r\n");
//     snk_len = 5; score = 0; snk_dir_x = 1; snk_dir_y = 0;
//     for(int i = 0; i < snk_len; i++) { snk_x[i] = 40 - (i*GRID_SIZE); snk_y[i] = 20; }
//     build_maze(level);
// }

// int main() {
//     spi_cs_high(); for(volatile int i=0; i<100000; i++);
//     spi_cs_low(); spi_transfer(0x0A); spi_transfer(0x2D); spi_transfer(0x02); spi_cs_high();

//     level = 1; reset_game();

//     while(1) {
//         int ax, ay; read_tilt(&ax, &ay);
//         if (ax > 15 && snk_dir_x == 0)       { snk_dir_x = 1; snk_dir_y = 0; }
//         else if (ax < -15 && snk_dir_x == 0) { snk_dir_x = -1; snk_dir_y = 0; }
//         else if (ay > 15 && snk_dir_y == 0)  { snk_dir_x = 0; snk_dir_y = -1; }
//         else if (ay < -15 && snk_dir_y == 0) { snk_dir_x = 0; snk_dir_y = 1; }

//         int next_x = snk_x[0] + (snk_dir_x * GRID_SIZE);
//         int next_y = snk_y[0] + (snk_dir_y * GRID_SIZE);
//         uint8_t target = vram[(next_y * 320) + next_x];

//         // Collision Logic
//         if (target == WALL_COLOR && (next_x == 0 || next_x >= 320-GRID_SIZE || next_y == 0 || next_y >= 240-GRID_SIZE)) {
//             level = 1; reset_game(); continue; // Hit outer wall (Die)
//         } else if (target == BODY_COLOR) {
//             level = 1; reset_game(); continue; // Hit self (Die)
//         } else if (target == WALL_COLOR) {
//             // Hit inner wall (Stop, don't die)
//         } else {
//             // Move forward
//             draw_block(snk_x[snk_len-1], snk_y[snk_len-1], BG_COLOR); // Erase tail
//             for(int i = snk_len - 1; i > 0; i--) { snk_x[i] = snk_x[i-1]; snk_y[i] = snk_y[i-1]; draw_block(snk_x[i], snk_y[i], BODY_COLOR); }
//             snk_x[0] = next_x; snk_y[0] = next_y;
            
//             if (target == APPLE_COLOR) {
//                 if (snk_len < MAX_SNAKE) snk_len += 3;
//                 score++;
//                 uart_print("Apples eaten: "); uart_print_num(score);
//                 if (score >= 5 && level == 1) { level = 2; reset_game(); continue; }
//                 spawn_apple();
//             }
//             draw_block(snk_x[0], snk_y[0], HEAD_COLOR);
//         }
//         for(volatile int i=0; i<150000; i++); 
//     }
// }
// #include <stdint.h>

// #define VRAM_BASE 0x40000000
// volatile uint8_t* const vram = (volatile uint8_t*)VRAM_BASE;

// #define MAX_CELLS 250 // Increased since you have more RAM
// #define FRAC_BITS 6   // Q9.6 fixed point

// int16_t p_x[MAX_CELLS], p_y[MAX_CELLS];
// int16_t v_x[MAX_CELLS], v_y[MAX_CELLS];

// static uint32_t r_state = 0x87654321;
// uint32_t fast_rand() {
//     r_state ^= r_state << 13; r_state ^= r_state >> 17; r_state ^= r_state << 5;
//     return r_state;
// }

// uint8_t get_speed_color(int vx, int vy) {
//     int speed = (vx*vx + vy*vy) >> (FRAC_BITS);
//     if (speed > 50) return 0xFF; // White (Fast)
//     if (speed > 20) return 0xFC; // Yellow (Medium)
//     return 0xE0;                 // Red (Slow)
// }

// void fade_screen() {
//     // Randomly eat pixels to create fading trails without clearing the whole screen
//     for(int i = 0; i < 2000; i++) {
//         vram[fast_rand() % 76800] = 0x00;
//     }
// }

// int main() {
//     for(int i = 0; i < 76800; i++) vram[i] = 0x00;
    
//     for(int i = 0; i < MAX_CELLS; i++) {
//         p_x[i] = (fast_rand() % 320) << FRAC_BITS;
//         p_y[i] = (fast_rand() % 240) << FRAC_BITS;
//         v_x[i] = (fast_rand() % 64) - 32;
//         v_y[i] = (fast_rand() % 64) - 32;
//     }

//     while(1) {
//         fade_screen(); // Creates the comet-trail effect

//         for(int i = 0; i < MAX_CELLS; i++) {
//             // Roaming behavior: slowly shift velocity
//             if ((fast_rand() % 100) < 10) v_x[i] += (fast_rand() % 9) - 4;
//             if ((fast_rand() % 100) < 10) v_y[i] += (fast_rand() % 9) - 4;

//             // Speed limit
//             if (v_x[i] > 64) v_x[i] = 64;  if (v_x[i] < -64) v_x[i] = -64;
//             if (v_y[i] > 64) v_y[i] = 64;  if (v_y[i] < -64) v_y[i] = -64;

//             p_x[i] += v_x[i];
//             p_y[i] += v_y[i];

//             // Bounce off walls
//             if (p_x[i] <= 0 || p_x[i] >= (319 << FRAC_BITS)) v_x[i] = -v_x[i];
//             if (p_y[i] <= 0 || p_y[i] >= (239 << FRAC_BITS)) v_y[i] = -v_y[i];

//             int draw_x = p_x[i] >> FRAC_BITS;
//             int draw_y = p_y[i] >> FRAC_BITS;
            
//             if(draw_x >= 0 && draw_x < 320 && draw_y >= 0 && draw_y < 240) {
//                 vram[(draw_y * 320) + draw_x] = get_speed_color(v_x[i], v_y[i]);
//             }
//         }
//         for(volatile int d=0; d<100000; d++); // Smooth framerate delay
//     }
// }
//Mandelbrot
// #include <stdint.h>

// #define VRAM_BASE 0x40000000
// volatile uint8_t* const vram = (volatile uint8_t*)VRAM_BASE;

// // Fixed-Point Math Parameters
// #define SHIFT 20
// #define ONE (1 << SHIFT)
// #define MULT(a, b) (int32_t)((((int64_t)(a)) * ((int64_t)(b))) >> SHIFT)

// #define MAX_ITER 64

// // Color Palette (RGB332 Mapping for depth)
// uint8_t get_fractal_color(int iter) {
//     if (iter == MAX_ITER) return 0x00; // Black inside the set
    
//     // Smooth transition from Blue -> Cyan -> Green -> Yellow -> Red
//     int c = iter % 16;
//     if (c < 4)  return 0x03 + (c << 2);      // Blues
//     if (c < 8)  return 0x1F - ((c-4) << 2);  // Cyans
//     if (c < 12) return 0x1C + ((c-8) << 5);  // Greens/Yellows
//     return 0xFC - ((c-12) << 2);             // Reds/Oranges
// }

// int main() {
//     // Fractal bounding box coordinates in Q12.20
//     int32_t x_min = (int32_t)(-2.0 * ONE);
//     int32_t x_max = (int32_t)( 1.0 * ONE);
//     int32_t y_min = (int32_t)(-1.2 * ONE);
//     int32_t y_max = (int32_t)( 1.2 * ONE);

//     int32_t dx = (x_max - x_min) / 320;
//     int32_t dy = (y_max - y_min) / 240;

//     int32_t c_y = y_min;

//     // Render loop
//     for (int py = 0; py < 240; py++) {
//         int32_t c_x = x_min;
//         for (int px = 0; px < 320; px++) {
//             int32_t z_x = 0;
//             int32_t z_y = 0;
//             int iter = 0;

//             // Z = Z^2 + C
//             while (iter < MAX_ITER) {
//                 int32_t z_x_sq = MULT(z_x, z_x);
//                 int32_t z_y_sq = MULT(z_y, z_y);

//                 // Escape condition: x^2 + y^2 > 4.0
//                 if ((z_x_sq + z_y_sq) > (4 * ONE)) break;

//                 int32_t temp_z_x = z_x_sq - z_y_sq + c_x;
//                 z_y = (MULT(z_x, z_y) << 1) + c_y; // 2*z_x*z_y + c_y
//                 z_x = temp_z_x;

//                 iter++;
//             }

//             vram[(py * 320) + px] = get_fractal_color(iter);
//             c_x += dx;
//         }
//         c_y += dy;
//     }

//     // Infinite loop when done to hold the beautiful image
//     while(1) {
//         for(volatile int d=0; d<10000; d++); 
//     }
// }
// #include <stdint.h>

// #define VRAM_BASE 0x40000000
// #define UART_BASE 0x20000000
// #define SPI_BASE  0x30000000

// volatile uint8_t* const vram       = (volatile uint8_t*)VRAM_BASE;
// volatile uint32_t* const uart_data = (volatile uint32_t*)UART_BASE;
// volatile uint32_t* const spi_data  = (volatile uint32_t*)SPI_BASE;
// volatile uint32_t* const spi_ctrl  = (volatile uint32_t*)(SPI_BASE + 4);

// // --- Colors (RGB332) ---
// #define BG_COLOR    0x00 // Black
// #define WALL_COLOR  0xE3 // Purple (Replaced Cyan)
// #define HEAD_COLOR  0xFC // Yellow
// #define BODY_COLOR  0x1C // Green
// #define APPLE_COLOR 0xE0 // Red

// #define GRID_SIZE 4
// #define MAX_SNAKE 150 

// int16_t snk_x[MAX_SNAKE], snk_y[MAX_SNAKE];
// int snk_len, snk_dir_x, snk_dir_y;
// int apple_x, apple_y, score, level;

// // --- UART & SPI Helpers ---
// void uart_print(const char* str) {
//     while (*str) {
//         *uart_data = *str++;
//         for(volatile int i=0; i<5000; i++); 
//     }
// }

// void uart_print_num(int num) {
//     char buf[10]; int i = 0;
//     if (num == 0) buf[i++] = '0';
//     while (num > 0) { buf[i++] = (num % 10) + '0'; num /= 10; }
//     while (i > 0) { *uart_data = buf[--i]; for(volatile int d=0; d<5000; d++); }
//     uart_print("\r\n");
// }

// void spi_cs_low()  { while (((*spi_ctrl) & 0x01) == 0); *spi_ctrl = 0x00; }
// void spi_cs_high() { while (((*spi_ctrl) & 0x01) == 0); *spi_ctrl = 0x02; }
// unsigned int spi_transfer(unsigned int d) {
//     volatile unsigned int dummy = *spi_data; (void)dummy;
//     *spi_data = d;
//     while (((*spi_ctrl) & 0x02) == 0);
//     return (*spi_data) & 0xFF;
// }
// void read_tilt(int *x, int *y) {
//     spi_cs_low(); spi_transfer(0x0B); spi_transfer(0x08);
//     *x = (signed char)spi_transfer(0x00);
//     *y = (signed char)spi_transfer(0x00);
//     spi_transfer(0x00); 
//     spi_cs_high();
// }

// // --- Graphics & Logic ---
// void draw_block(int x, int y, uint8_t color) {
//     for (int r = 0; r < GRID_SIZE; r++)
//         for (int c = 0; c < GRID_SIZE; c++)
//             vram[((y + r) * 320) + (x + c)] = color;
// }

// // Helper to draw clean walls in the maze
// void draw_wall_rect(int x, int y, int w, int h) {
//     for (int r = 0; r < h; r += GRID_SIZE) {
//         for (int c = 0; c < w; c += GRID_SIZE) {
//             draw_block(x + c, y + r, WALL_COLOR);
//         }
//     }
// }

// static uint32_t seed = 98765;
// int rand_pos(int max) {
//     seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF;
//     return (seed % (max / GRID_SIZE)) * GRID_SIZE;
// }

// void spawn_apple() {
//     while(1) {
//         apple_x = rand_pos(300);
//         apple_y = rand_pos(220);
        
//         // Keep it safely away from the absolute borders
//         if (apple_x < GRID_SIZE * 2) apple_x = GRID_SIZE * 2;
//         if (apple_y < GRID_SIZE * 2) apple_y = GRID_SIZE * 2;
        
//         // Only spawn if the spot is pure background black
//         if (vram[(apple_y * 320) + apple_x] == BG_COLOR) break; 
//     }
//     draw_block(apple_x, apple_y, APPLE_COLOR);
// }

// void build_maze(int lvl) {
//     for(int i = 0; i < 76800; i++) vram[i] = BG_COLOR;
    
//     // Outer Borders (Solid Walls)
//     draw_wall_rect(0, 0, 320, GRID_SIZE);             // Top
//     draw_wall_rect(0, 240-GRID_SIZE, 320, GRID_SIZE); // Bottom
//     draw_wall_rect(0, 0, GRID_SIZE, 240);             // Left
//     draw_wall_rect(320-GRID_SIZE, 0, GRID_SIZE, 240); // Right

//     // Actual Maze Generation
//     if (lvl == 1) {
//         // Level 1: A classic "Pac-Man" style structured maze
//         draw_wall_rect(40, 40, 240, GRID_SIZE);   // Top inner bound
//         draw_wall_rect(40, 40, GRID_SIZE, 160);   // Left inner bound
//         draw_wall_rect(40, 200, 240, GRID_SIZE);  // Bottom inner bound
        
//         // Middle blockers
//         draw_wall_rect(120, 80, GRID_SIZE, 80);   // Vertical divider
//         draw_wall_rect(120, 120, 80, GRID_SIZE);  // Horizontal branch
        
//         // Small corner rooms
//         draw_wall_rect(200, 40, GRID_SIZE, 40);
//         draw_wall_rect(240, 120, 40, GRID_SIZE);
        
//     } else {
//         // Level 2: Harder, tighter labyrinth
//         draw_wall_rect(40, 40, 240, GRID_SIZE);
//         draw_wall_rect(160, 40, GRID_SIZE, 100);
        
//         draw_wall_rect(40, 80, 80, GRID_SIZE);
//         draw_wall_rect(80, 120, GRID_SIZE, 80);
        
//         draw_wall_rect(120, 160, 120, GRID_SIZE);
//         draw_wall_rect(240, 80, GRID_SIZE, 120);
//         draw_wall_rect(200, 80, 40, GRID_SIZE);
//     }
    
//     spawn_apple();
// }

// void reset_game() {
//     uart_print("--- NEW GAME (Level "); uart_print_num(level); uart_print(") ---\r\n");
//     snk_len = 5; 
//     score = 0; 
//     snk_dir_x = 1; 
//     snk_dir_y = 0;
    
//     // Spawn snake safely inside the maze
//     for(int i = 0; i < snk_len; i++) { 
//         snk_x[i] = 40 - (i*GRID_SIZE); 
//         snk_y[i] = 20; 
//     }
//     build_maze(level);
// }

// int main() {
//     spi_cs_high(); for(volatile int i=0; i<100000; i++);
//     spi_cs_low(); spi_transfer(0x0A); spi_transfer(0x2D); spi_transfer(0x02); spi_cs_high();

//     level = 1; 
//     reset_game();

//     while(1) {
//         int ax, ay; read_tilt(&ax, &ay);
        
//         // Change direction based on tilt
//         if (ax > 15 && snk_dir_x == 0)       { snk_dir_x = 1; snk_dir_y = 0; }
//         else if (ax < -15 && snk_dir_x == 0) { snk_dir_x = -1; snk_dir_y = 0; }
//         else if (ay > 15 && snk_dir_y == 0)  { snk_dir_x = 0; snk_dir_y = -1; }
//         else if (ay < -15 && snk_dir_y == 0) { snk_dir_x = 0; snk_dir_y = 1; }

//         int next_x = snk_x[0] + (snk_dir_x * GRID_SIZE);
//         int next_y = snk_y[0] + (snk_dir_y * GRID_SIZE);
        
//         // 1. Boundary Crash Protection
//         if (next_x < 0 || next_x > (320 - GRID_SIZE) || next_y < 0 || next_y > (240 - GRID_SIZE)) {
//             // Hit absolute outer boundary. Treat it like a solid wall (stop).
//             next_x = snk_x[0];
//             next_y = snk_y[0];
//         } else {
//             // Safe to check VRAM
//             uint8_t target = vram[(next_y * 320) + next_x];

//             // 2. Solid Wall Logic
//             if (target == WALL_COLOR) {
//                 // Do not advance the snake's position
//                 next_x = snk_x[0];
//                 next_y = snk_y[0];
//             } 
//             // 3. Death Logic (Biting Self)
//             else if (target == BODY_COLOR) {
//                 level = 1; 
//                 reset_game(); 
//                 continue; 
//             } 
//             // 4. Food & Level Logic
//             else if (target == APPLE_COLOR) {
//                 if (snk_len < MAX_SNAKE) snk_len += 3; // Grow
//                 score++;
                
//                 uart_print("Apples eaten: "); uart_print_num(score);
                
//                 // Advance to Level 2 after 3 apples
//                 if (score >= 3 && level == 1) { 
//                     level = 2; 
//                     reset_game(); 
//                     continue; 
//                 }
//                 spawn_apple();
//             }
//         }

//         // Only update the body arrays and draw if the snake actually moved.
//         // If it is stuck on a wall, it will wait here until you tilt away.
//         if (next_x != snk_x[0] || next_y != snk_y[0]) {
//             draw_block(snk_x[snk_len-1], snk_y[snk_len-1], BG_COLOR); // Erase tail
            
//             // Shift body forward
//             for(int i = snk_len - 1; i > 0; i--) { 
//                 snk_x[i] = snk_x[i-1]; 
//                 snk_y[i] = snk_y[i-1]; 
//                 draw_block(snk_x[i], snk_y[i], BODY_COLOR); 
//             }
            
//             // Move Head
//             snk_x[0] = next_x; 
//             snk_y[0] = next_y;
//             draw_block(snk_x[0], snk_y[0], HEAD_COLOR);
//         }

//         // Delay to control game speed
//         for(volatile int i=0; i<150000; i++); 
//     }
// }
// #include <stdint.h>

// #define VRAM_BASE 0x40000000
// #define UART_BASE 0x20000000
// #define SPI_BASE  0x30000000

// volatile uint8_t* const vram       = (volatile uint8_t*)VRAM_BASE;
// volatile uint32_t* const uart_data = (volatile uint32_t*)UART_BASE;
// volatile uint32_t* const spi_data  = (volatile uint32_t*)SPI_BASE;
// volatile uint32_t* const spi_ctrl  = (volatile uint32_t*)(SPI_BASE + 4);

// // --- Colors (RGB332) ---
// #define BG_COLOR    0x00 // Black
// #define WALL_COLOR  0xE3 // Purple
// #define HEAD_COLOR  0xFC // Yellow
// #define BODY_COLOR  0x1C // Green
// #define APPLE_COLOR 0xE0 // Red

// #define GRID_SIZE 4
// #define MAX_SNAKE 150 

// int16_t snk_x[MAX_SNAKE], snk_y[MAX_SNAKE];
// int snk_len, snk_dir_x, snk_dir_y;
// int apple_x, apple_y;

// // Game stats
// int level = 1;
// int total_apples = 0;
// int apples_this_level = 0;

// // --- UART & SPI Helpers ---
// void uart_print(const char* str) {
//     while (*str) {
//         *uart_data = *str++;
//         for(volatile int i=0; i<5000; i++); 
//     }
// }

// void uart_print_num(int num) {
//     char buf[10]; int i = 0;
//     if (num == 0) buf[i++] = '0';
//     while (num > 0) { buf[i++] = (num % 10) + '0'; num /= 10; }
//     while (i > 0) { *uart_data = buf[--i]; for(volatile int d=0; d<5000; d++); }
//     // Note: Removed the automatic \r\n here so we can print on one line
// }

// void spi_cs_low()  { while (((*spi_ctrl) & 0x01) == 0); *spi_ctrl = 0x00; }
// void spi_cs_high() { while (((*spi_ctrl) & 0x01) == 0); *spi_ctrl = 0x02; }
// unsigned int spi_transfer(unsigned int d) {
//     volatile unsigned int dummy = *spi_data; (void)dummy;
//     *spi_data = d;
//     while (((*spi_ctrl) & 0x02) == 0);
//     return (*spi_data) & 0xFF;
// }
// void read_tilt(int *x, int *y) {
//     spi_cs_low(); spi_transfer(0x0B); spi_transfer(0x08);
//     *x = (signed char)spi_transfer(0x00);
//     *y = (signed char)spi_transfer(0x00);
//     spi_transfer(0x00); 
//     spi_cs_high();
// }

// // --- Graphics & Logic ---
// void draw_block(int x, int y, uint8_t color) {
//     for (int r = 0; r < GRID_SIZE; r++)
//         for (int c = 0; c < GRID_SIZE; c++)
//             vram[((y + r) * 320) + (x + c)] = color;
// }

// void draw_wall_rect(int x, int y, int w, int h) {
//     for (int r = 0; r < h; r += GRID_SIZE) {
//         for (int c = 0; c < w; c += GRID_SIZE) {
//             draw_block(x + c, y + r, WALL_COLOR);
//         }
//     }
// }

// static uint32_t seed = 98765;
// int rand_pos(int max) {
//     seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF;
//     return (seed % (max / GRID_SIZE)) * GRID_SIZE;
// }

// void spawn_apple() {
//     while(1) {
//         apple_x = rand_pos(300);
//         apple_y = rand_pos(220);
        
//         if (apple_x < GRID_SIZE * 2) apple_x = GRID_SIZE * 2;
//         if (apple_y < GRID_SIZE * 2) apple_y = GRID_SIZE * 2;
        
//         if (vram[(apple_y * 320) + apple_x] == BG_COLOR) break; 
//     }
//     draw_block(apple_x, apple_y, APPLE_COLOR);
// }

// void build_maze(int lvl) {
//     for(int i = 0; i < 76800; i++) vram[i] = BG_COLOR;
    
//     // Outer Borders
//     draw_wall_rect(0, 0, 320, GRID_SIZE);             
//     draw_wall_rect(0, 240-GRID_SIZE, 320, GRID_SIZE); 
//     draw_wall_rect(0, 0, GRID_SIZE, 240);             
//     draw_wall_rect(320-GRID_SIZE, 0, GRID_SIZE, 240); 

//     int maze_type = (lvl - 1) % 3; // Cycles through 3 layouts endlessly

//     if (maze_type == 0) {
//         // Layout 1: Classic
//         draw_wall_rect(40, 40, 240, GRID_SIZE);   
//         draw_wall_rect(40, 40, GRID_SIZE, 160);   
//         draw_wall_rect(40, 200, 240, GRID_SIZE);  
//         draw_wall_rect(120, 80, GRID_SIZE, 80);   
//         draw_wall_rect(120, 120, 80, GRID_SIZE);  
//         draw_wall_rect(200, 40, GRID_SIZE, 40);
//         draw_wall_rect(240, 120, 40, GRID_SIZE);
//     } 
//     else if (maze_type == 1) {
//         // Layout 2: Tight Corridors
//         draw_wall_rect(40, 40, 240, GRID_SIZE);
//         draw_wall_rect(160, 40, GRID_SIZE, 100);
//         draw_wall_rect(40, 80, 80, GRID_SIZE);
//         draw_wall_rect(80, 120, GRID_SIZE, 80);
//         draw_wall_rect(120, 160, 120, GRID_SIZE);
//         draw_wall_rect(240, 80, GRID_SIZE, 120);
//         draw_wall_rect(200, 80, 40, GRID_SIZE);
//     }
//     else {
//         // Layout 3: Pillars / Checkered
//         for(int x = 60; x < 280; x += 60) {
//             for(int y = 60; y < 200; y += 60) {
//                 draw_wall_rect(x, y, 20, 20);
//             }
//         }
//     }
    
//     spawn_apple();
// }

// void load_level() {
//     uart_print("\r\n--- STARTING LEVEL "); uart_print_num(level); uart_print(" ---\r\n");
//     snk_len = 5; 
//     apples_this_level = 0;
//     snk_dir_x = 1; 
//     snk_dir_y = 0;
    
//     for(int i = 0; i < snk_len; i++) { 
//         snk_x[i] = 40 - (i*GRID_SIZE); 
//         snk_y[i] = 20; 
//     }
//     build_maze(level);
// }

// void game_over() {
//     uart_print("\r\n!!! GAME OVER !!!\r\n");
//     uart_print("You made it to Level: "); uart_print_num(level);
//     uart_print(" with "); uart_print_num(total_apples); uart_print(" total apples.\r\n");
    
//     // Reset global stats
//     level = 1;
//     total_apples = 0;
//     load_level();
// }

// int main() {
//     spi_cs_high(); for(volatile int i=0; i<100000; i++);
//     spi_cs_low(); spi_transfer(0x0A); spi_transfer(0x2D); spi_transfer(0x02); spi_cs_high();

//     // Start first game
//     level = 1;
//     total_apples = 0;
//     load_level();

//     while(1) {
//         int ax, ay; read_tilt(&ax, &ay);
        
//         if (ax > 15 && snk_dir_x == 0)       { snk_dir_x = 1; snk_dir_y = 0; }
//         else if (ax < -15 && snk_dir_x == 0) { snk_dir_x = -1; snk_dir_y = 0; }
//         else if (ay > 15 && snk_dir_y == 0)  { snk_dir_x = 0; snk_dir_y = -1; }
//         else if (ay < -15 && snk_dir_y == 0) { snk_dir_x = 0; snk_dir_y = 1; }

//         int next_x = snk_x[0] + (snk_dir_x * GRID_SIZE);
//         int next_y = snk_y[0] + (snk_dir_y * GRID_SIZE);
        
//         if (next_x < 0 || next_x > (320 - GRID_SIZE) || next_y < 0 || next_y > (240 - GRID_SIZE)) {
//             next_x = snk_x[0];
//             next_y = snk_y[0];
//         } else {
//             uint8_t target = vram[(next_y * 320) + next_x];

//             if (target == WALL_COLOR) {
//                 next_x = snk_x[0];
//                 next_y = snk_y[0];
//             } 
//             else if (target == BODY_COLOR) {
//                 game_over(); 
//                 continue; 
//             } 
//             else if (target == APPLE_COLOR) {
//                 if (snk_len < MAX_SNAKE) snk_len += 3;
                
//                 total_apples++;
//                 apples_this_level++;
                
//                 // UART Format: "Level: X | Total Apples: Y"
//                 uart_print("Level: "); uart_print_num(level);
//                 uart_print(" | Total Apples: "); uart_print_num(total_apples);
//                 uart_print("\r\n");
                
//                 // Level up after every 3 apples
//                 if (apples_this_level >= 3) { 
//                     level++; 
//                     load_level(); 
//                     continue; 
//                 } else {
//                     spawn_apple();
//                 }
//             }
//         }

//         if (next_x != snk_x[0] || next_y != snk_y[0]) {
//             draw_block(snk_x[snk_len-1], snk_y[snk_len-1], BG_COLOR); 
            
//             for(int i = snk_len - 1; i > 0; i--) { 
//                 snk_x[i] = snk_x[i-1]; 
//                 snk_y[i] = snk_y[i-1]; 
//                 draw_block(snk_x[i], snk_y[i], BODY_COLOR); 
//             }
            
//             snk_x[0] = next_x; 
//             snk_y[0] = next_y;
//             draw_block(snk_x[0], snk_y[0], HEAD_COLOR);
//         }

//         for(volatile int i=0; i<150000; i++); 
//     }
// }
// #include <stdint.h>

// ==========================================
// 1. HARDWARE MEMORY MAP
// ==========================================
// Based on your AXI Crossbar configuration:
// RAM  = 0x00000000
// LED  = 0x10000000
// UART = 0x20000000
// SPI  = 0x30000000
// VRAM = 0x40000000

// #define LED_REG         (*(volatile uint32_t*)0x10000000)

// #define UART_DATA_REG   (*(volatile uint32_t*)0x20000000)
// #define UART_STATUS_REG (*(volatile uint32_t*)0x20000004)

// // VRAM is byte-addressable for 8-bit pixels
// #define VRAM_BASE       ((volatile uint8_t*)0x40000000)

// // VGA Display Constants (Assuming 320x240 resolution for 76800 bytes)
// #define SCREEN_WIDTH    320
// #define SCREEN_HEIGHT   240

// // ==========================================
// // 2. CUSTOM LIGHTWEIGHT MATH FUNCTIONS
// // ==========================================
// #define PI 3.14159265f

// float my_fabs(float x) {
//     return (x < 0) ? -x : x;
// }

// float my_sin(float x) {
//     // Wrap angle to the range -PI to PI
//     while (x >  PI) x -= 2.0f * PI;
//     while (x < -PI) x += 2.0f * PI;

//     float x2 = x * x;
//     float term1 = x;
//     float term2 = (term1 * x2) / 6.0f;           // x^3 / 3!
//     float term3 = (term2 * x2) / 20.0f;          // x^5 / 5!
//     float term4 = (term3 * x2) / 42.0f;          // x^7 / 7!
//     float term5 = (term4 * x2) / 72.0f;          // x^9 / 9!

//     return term1 - term2 + term3 - term4 + term5;
// }

// float my_cos(float x) {
//     return my_sin(x + 1.57079632f); 
// }

// float my_sqrt(float n) {
//     if (n <= 0.0f) return 0.0f;
//     float x = n;
//     float y = 1.0f;
//     float e = 0.001f;
//     while (my_fabs(x - y) > e) {
//         x = (x + y) / 2.0f;
//         y = n / x;
//     }
//     return x;
// }

// // ==========================================
// // 3. BARE-METAL UART PRINTING (Replaces printf)
// // ==========================================

// // Send a single character
// void uart_putc(char c) {
//     // Wait until TX is ready (bit 0 of status register is 1)
//     while ((UART_STATUS_REG & 0x01) == 0);
//     UART_DATA_REG = c;
// }

// // Print a string
// void print_str(const char* str) {
//     while (*str) {
//         uart_putc(*str++);
//     }
// }

// // Print an integer (handles negative numbers)
// void print_int(int num) {
//     if (num == 0) {
//         uart_putc('0');
//         return;
//     }
//     if (num < 0) {
//         uart_putc('-');
//         num = -num;
//     }
    
//     char buffer[10];
//     int i = 0;
//     while (num > 0) {
//         buffer[i++] = (num % 10) + '0';
//         num /= 10;
//     }
    
//     // Print in reverse order
//     while (i > 0) {
//         uart_putc(buffer[--i]);
//     }
// }

// // ==========================================
// // 4. GRAPHICS / VRAM UTILITIES
// // ==========================================
// void draw_pixel(int x, int y, uint8_t color) {
//     if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
//         VRAM_BASE[(y * SCREEN_WIDTH) + x] = color;
//     }
// }

// void clear_screen(uint8_t color) {
//     for (int i = 0; i < (SCREEN_WIDTH * SCREEN_HEIGHT); i++) {
//         VRAM_BASE[i] = color;
//     }
// }

// // ==========================================
// // 5. MAIN APPLICATION
// // ==========================================
// void delay(int count) {
//     for (volatile int i = 0; i < count; i++); // Simple busy-wait delay
// }

// int main() {
//     print_str("\r\n==============================\r\n");
//     print_str("Nexus-V Bare-Metal Firmware OS\r\n");
//     print_str("Libraries loaded: 0 Bytes\r\n");
//     print_str("Math unit: Software Emulated\r\n");
//     print_str("==============================\r\n");

//     clear_screen(0x00); // Clear to black
    
//     uint8_t led_val = 1;
//     float time_t = 0.0f;

//     while (1) {
//         // --- 1. Blink LEDs ---
//         LED_REG = led_val;
//         led_val = (led_val << 1);
//         if (led_val == 0) led_val = 1;

//         // --- 2. Calculate Math ---
//         // Let's calculate a bouncing Y coordinate using our custom sine wave
//         // Map sine (-1.0 to 1.0) to screen height (0 to 240)
//         float s_val = my_sin(time_t);
//         int target_y = (int)((s_val + 1.0f) * (SCREEN_HEIGHT / 2.0f));
        
//         // --- 3. Draw to Screen ---
//         // Let x be driven by time, wrapping around the screen
//         int target_x = (int)(time_t * 50.0f) % SCREEN_WIDTH;
        
//         // Draw a white pixel at the calculated coordinate
//         draw_pixel(target_x, target_y, 0xFF);

//         // --- 4. Print Status via UART ---
//         print_str("Time: ");
//         print_int((int)time_t);
//         print_str(" | Y-Coord: ");
//         print_int(target_y);
//         print_str("\r\n");

//         // Advance time and delay to prevent flooding
//         time_t += 0.1f;
//         delay(5000); // Adjust this delay based on your 25MHz CPU speed
//     }

//     return 0;
// }
// #include <stdint.h>

// #define VRAM_BASE 0x40000000
// volatile uint8_t* const vram = (volatile uint8_t*)VRAM_BASE;

// // Global Look-Up Tables
// uint8_t sin_lut[256];
// uint8_t palette[256];

// // --- Custom Math for Boot Initialization ---
// #define PI 3.14159265f

// float my_sin(float x) {
//     while (x >  PI) x -= 2.0f * PI;
//     while (x < -PI) x += 2.0f * PI;
//     float x2 = x * x;
//     // Taylor series approximation
//     float term1 = x;
//     float term2 = (term1 * x2) / 6.0f;
//     float term3 = (term2 * x2) / 20.0f;
//     float term4 = (term3 * x2) / 42.0f;
//     return term1 - term2 + term3 - term4;
// }

// // --- Initialize Look-Up Tables ---
// void init_tables() {
//     // 1. Generate 256-value Sine Table (scaled from 0 to 255)
//     for(int i = 0; i < 256; i++) {
//         float angle = ((float)i / 256.0f) * 2.0f * PI;
//         sin_lut[i] = (uint8_t)((my_sin(angle) + 1.0f) * 127.5f);
//     }

//     // 2. Generate a highly vibrant RGB332 Rainbow Palette
//     for(int i = 0; i < 256; i++) {
//         // Offset the color phases to create a shifting rainbow
//         float r_angle = ((float)i / 256.0f) * 2.0f * PI;
//         float g_angle = r_angle + (2.0f * PI / 3.0f); // offset by 120 degrees
//         float b_angle = r_angle + (4.0f * PI / 3.0f); // offset by 240 degrees

//         // Scale to RGB332 bits (Red: 0-7, Green: 0-7, Blue: 0-3)
//         uint8_t r = (uint8_t)((my_sin(r_angle) + 1.0f) * 3.5f);
//         uint8_t g = (uint8_t)((my_sin(g_angle) + 1.0f) * 3.5f);
//         uint8_t b = (uint8_t)((my_sin(b_angle) + 1.0f) * 1.5f);

//         palette[i] = (r << 5) | (g << 2) | b;
//     }
// }

// // --- Main Application ---
// int main() {
//     // Boot: Pre-calculate all complex math
//     init_tables();
    
//     uint8_t time = 0;

//     // Run the high-speed rendering loop
//     while(1) {
//         // Calculate time offsets for the frame
//         uint8_t t1 = time;
//         uint8_t t2 = (time * 3) / 2; // Move at a different speed
//         uint8_t t3 = time / 2;       // Slow moving layer

//         int ptr = 0;
        
//         for (int y = 0; y < 240; y++) {
//             // Calculate Y components once per row to save CPU cycles!
//             uint8_t y_comp1 = sin_lut[(y + t1) & 255];  // Fast vertical wave
//             uint8_t y_comp2 = sin_lut[(y + t3) & 255];  // Slow vertical wave
            
//             for (int x = 0; x < 320; x++) {
//                 // Calculate X and combined components
//                 uint8_t x_comp1 = sin_lut[(x + t2) & 255];       // Horizontal wave
//                 uint8_t xy_comp = sin_lut[(x + y + t1) & 255];   // Diagonal wave
                
//                 // Combine the 4 overlapping waves
//                 // We shift right by 2 (divide by 4) to keep the value between 0-255
//                 uint8_t combined_plasma = (x_comp1 + y_comp1 + xy_comp + y_comp2) >> 2;

//                 // Shift the color over time and write to VRAM
//                 vram[ptr++] = palette[(combined_plasma + time) & 255];
//             }
//         }
        
//         // Advance time for the next frame
//         time += 2; 
        
//         // Slight delay to prevent screen tearing, adjust if it runs too fast/slow
//         for(volatile int d=0; d<10000; d++); 
//     }
// }

// //New snake
// #include <stdint.h>

// #define VRAM_BASE 0x40000000
// #define UART_BASE 0x20000000
// #define SPI_BASE  0x30000000

// volatile uint8_t* const vram       = (volatile uint8_t*)VRAM_BASE;
// volatile uint32_t* const uart_data   = (volatile uint32_t*)UART_BASE;
// volatile uint32_t* const uart_status = (volatile uint32_t*)(UART_BASE + 4);
// volatile uint32_t* const spi_data  = (volatile uint32_t*)SPI_BASE;
// volatile uint32_t* const spi_ctrl  = (volatile uint32_t*)(SPI_BASE + 4);

// #define BG_COLOR    0x00 // Black
// #define WALL_COLOR  0xE3 // Purple
// #define HEAD_COLOR  0xFC // Yellow
// #define BODY_COLOR  0x1C // Green
// #define APPLE_COLOR 0xE0 // Red

// #define GRID_SIZE 4
// #define MAX_SNAKE 150 

// int16_t snk_x[MAX_SNAKE], snk_y[MAX_SNAKE];
// int snk_len, snk_dir_x, snk_dir_y;
// int apple_x, apple_y;
// int level = 1;
// int current_speed = 150000;

// void uart_print(const char* str) {
//     while (*str) {
//         while (!((*uart_status) & 0x01)); 
//         *uart_data = *str++;
//     }
// }

// void spi_cs_low()  { while (((*spi_ctrl) & 0x01) == 0); *spi_ctrl = 0x00; }
// void spi_cs_high() { while (((*spi_ctrl) & 0x01) == 0); *spi_ctrl = 0x02; }
// unsigned int spi_transfer(unsigned int d) {
//     volatile unsigned int dummy = *spi_data; (void)dummy;
//     *spi_data = d;
//     while (((*spi_ctrl) & 0x02) == 0);
//     return (*spi_data) & 0xFF;
// }

// void read_tilt(int *x, int *y) {
//     spi_cs_low(); spi_transfer(0x0B); spi_transfer(0x08);
//     *x = (signed char)spi_transfer(0x00);
//     *y = (signed char)spi_transfer(0x00);
//     spi_transfer(0x00); spi_cs_high();
// }

// void draw_block(int x, int y, uint8_t color) {
//     for (int r = 0; r < GRID_SIZE; r++)
//         for (int c = 0; c < GRID_SIZE; c++)
//             vram[((y + r) * 320) + (x + c)] = color;
// }

// void draw_wall_rect(int x, int y, int w, int h) {
//     for (int r = 0; r < h; r += GRID_SIZE)
//         for (int c = 0; c < w; c += GRID_SIZE)
//             draw_block(x + c, y + r, WALL_COLOR);
// }

// static uint32_t seed = 98765;
// int rand_pos(int max) {
//     seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF;
//     return (seed % (max / GRID_SIZE)) * GRID_SIZE;
// }

// void spawn_apple() {
//     while(1) {
//         apple_x = rand_pos(300);
//         apple_y = rand_pos(220);
//         if (apple_x < GRID_SIZE * 2) apple_x = GRID_SIZE * 2;
//         if (apple_y < GRID_SIZE * 2) apple_y = GRID_SIZE * 2;
//         if (vram[(apple_y * 320) + apple_x] == BG_COLOR) break; 
//     }
//     draw_block(apple_x, apple_y, APPLE_COLOR);
// }

// void build_maze() {
//     for(int i = 0; i < 76800; i++) vram[i] = BG_COLOR; // Wipe screen
    
//     // Outer Borders (Visual Walls)
//     draw_wall_rect(0, 0, 320, GRID_SIZE);            
//     draw_wall_rect(0, 240-GRID_SIZE, 320, GRID_SIZE); 
//     draw_wall_rect(0, 0, GRID_SIZE, 240);            
//     draw_wall_rect(320-GRID_SIZE, 0, GRID_SIZE, 240); 

//     if (level == 2) {
//         // Level 2: Four Pillars
//         draw_wall_rect(80, 60, 20, 40);
//         draw_wall_rect(220, 60, 20, 40);
//         draw_wall_rect(80, 140, 20, 40);
//         draw_wall_rect(220, 140, 20, 40);
//     } 
//     else if (level == 3) {
//         // Level 3: The Gauntlet
//         draw_wall_rect(40, 40, 240, GRID_SIZE);
//         draw_wall_rect(160, 40, GRID_SIZE, 100);
//         draw_wall_rect(40, 80, 80, GRID_SIZE);
//         draw_wall_rect(80, 120, GRID_SIZE, 80);
//         draw_wall_rect(120, 160, 120, GRID_SIZE);
//         draw_wall_rect(240, 80, GRID_SIZE, 120);
//         draw_wall_rect(200, 80, 40, GRID_SIZE);
//     }
//     spawn_apple();
// }

// void load_level() {
//     uart_print("\r\n--- LOADING LEVEL ---\r\n");
    
//     if (level == 1) current_speed = 150000; // Normal
//     if (level == 2) current_speed = 80000;  // Fast
//     if (level == 3) current_speed = 30000;  // Max Speed

//     snk_len = 5; 
    
//     // DEAD STOP: The snake won't move until you tilt the board
//     snk_dir_x = 0; 
//     snk_dir_y = 0;
    
//     build_maze();
    
//     // Place snake safely in the middle of the screen
//     for(int i = 0; i < snk_len; i++) { 
//         snk_x[i] = 160 - (i*GRID_SIZE); 
//         snk_y[i] = 120; 
//         if (i == 0) draw_block(snk_x[i], snk_y[i], HEAD_COLOR);
//         else draw_block(snk_x[i], snk_y[i], BODY_COLOR);
//     }
// }

// int main() {
//     spi_cs_high(); for(volatile int i=0; i<100000; i++);
//     spi_cs_low(); spi_transfer(0x0A); spi_transfer(0x2D); spi_transfer(0x02); spi_cs_high();

//     uart_print("\r\n*** 3-LEVEL TILT SNAKE ***\r\n");
//     level = 1; 
//     load_level();

//     while(1) {
//         int ax, ay; read_tilt(&ax, &ay);
        
//         // Only change direction if we tilt hard enough
//         if (ax > 15 && snk_dir_x == 0)       { snk_dir_x = 1; snk_dir_y = 0; }
//         else if (ax < -15 && snk_dir_x == 0) { snk_dir_x = -1; snk_dir_y = 0; }
//         else if (ay > 15 && snk_dir_y == 0)  { snk_dir_x = 0; snk_dir_y = -1; }
//         else if (ay < -15 && snk_dir_y == 0) { snk_dir_x = 0; snk_dir_y = 1; }

//         // If we haven't tilted yet (start of level), just wait
//         if (snk_dir_x == 0 && snk_dir_y == 0) continue;

//         int next_x = snk_x[0] + (snk_dir_x * GRID_SIZE);
//         int next_y = snk_y[0] + (snk_dir_y * GRID_SIZE);
        
//         // ==========================================
//         // BUG FIX: HARD BORDER COLLISION CHECK
//         // Must happen BEFORE reading VRAM to prevent hardware bus crashes!
//         // ==========================================
//         if (next_x < 0 || next_x > (320 - GRID_SIZE) || next_y < 0 || next_y > (240 - GRID_SIZE)) {
//             uart_print("\r\n[!] CRASH! Hit the outer border. Back to Level 1.\r\n");
//             level = 1;
//             load_level();
//             continue;
//         }

//         uint8_t target = vram[(next_y * 320) + next_x];

//         // Hit an Internal Wall or Tail -> Restart completely
//         if (target == WALL_COLOR || target == BODY_COLOR) {
//             uart_print("\r\n[!] CRASH! Hit an obstacle. Back to Level 1.\r\n");
//             level = 1;
//             load_level(); 
//             continue; 
//         } 
        
//         // Hit the Apple -> LEVEL UP
//         if (target == APPLE_COLOR) {
//             level++;
            
//             if (level > 3) {
//                 uart_print("\r\n*** YOU BEAT LEVEL 3! YOU WIN! ***\r\n");
//                 level = 1; // Restart for fun
//             } else {
//                 uart_print("\r\n*** LEVEL UP! ***\r\n");
//             }

//             // Brief pause so you can see the apple was eaten before the screen wipes
//             for(volatile int i=0; i<3000000; i++); 

//             load_level(); 
//             continue; // Safely skip the rest of the loop and wait for new tilt
//         }

//         // Normal movement
//         draw_block(snk_x[snk_len-1], snk_y[snk_len-1], BG_COLOR); 
//         for(int i = snk_len - 1; i > 0; i--) { 
//             snk_x[i] = snk_x[i-1]; 
//             snk_y[i] = snk_y[i-1]; 
//             draw_block(snk_x[i], snk_y[i], BODY_COLOR); 
//         }
//         snk_x[0] = next_x; snk_y[0] = next_y;
//         draw_block(snk_x[0], snk_y[0], HEAD_COLOR);

//         for(volatile int i=0; i<current_speed; i++); // Speed delay
//     }
// }

//THERMOMETER
#include <stdint.h>

// =========================================
// MEMORY MAP
// =========================================
#define VRAM_BASE  0x40000000
#define I2C_BASE   0x50000000
#define UART_BASE  0x20000000

volatile uint8_t* const vram    = (volatile uint8_t*)VRAM_BASE;
volatile uint32_t* const i2c_reg  = (volatile uint32_t*)I2C_BASE;
volatile uint32_t* const uart_reg = (volatile uint32_t*)UART_BASE;

// =========================================
// I2C REGISTER MAP
// =========================================
#define I2C_CTRL        i2c_reg[0]   
#define I2C_ADDR        i2c_reg[1]   
#define I2C_SUB_ADDR    i2c_reg[2]   
#define I2C_BYTE_LEN    i2c_reg[3]   
#define I2C_TX_DATA     i2c_reg[4]   

#define I2C_STATUS      i2c_reg[0]
#define I2C_RX_DATA     i2c_reg[5]   

#define I2C_BUSY        (i2c_reg[0] & 0x01)
#define I2C_NACK        (i2c_reg[0] & 0x02)
#define I2C_VALID       (i2c_reg[0] & 0x04)
#define I2C_REQ_CHUNK   (i2c_reg[0] & 0x08)

// =========================================
// UART REGISTER MAP
// =========================================
#define UART_TX_DATA    uart_reg[0]
#define UART_STATUS     uart_reg[1]   
#define UART_TX_READY   (uart_reg[1] & 0x01)  
#define UART_RX_VALID   (uart_reg[1] & 0x02)  

// =========================================
// ADT7420 CONSTANTS
// =========================================
#define ADT7420_ADDR_W   0x96   
#define ADT7420_ADDR_R   0x97   
#define ADT7420_REG_TEMP 0x00   
#define ADT7420_REG_CFG  0x03   
#define ADT7420_CFG_16BIT 0x80  

// =========================================
// DISPLAY RANGE - HYPER SENSITIVE
// =========================================
// Centered around your exact current room temp!
#define MID_TEMP_MILI   31700   // 31.7 C
#define HALF_RANGE        200   // +/- 0.2 C
#define MIN_TEMP_MILI   (MID_TEMP_MILI - HALF_RANGE) // 31.5 C
#define MAX_TEMP_MILI   (MID_TEMP_MILI + HALF_RANGE) // 31.9 C

// =========================================
// UART DRIVER
// =========================================
void uart_putc(char c) {
    while (!UART_TX_READY);
    UART_TX_DATA = (uint32_t)c;
}
void uart_puts(const char* s) { while (*s) uart_putc(*s++); }

void uart_print_int(int v) {
    char buf[12]; int i = 0;
    if (v < 0) { uart_putc('-'); v = -v; }
    if (v == 0) { uart_putc('0'); return; }
    while (v > 0) { buf[i++] = '0' + (v % 10); v /= 10; }
    while (i--) uart_putc(buf[i]);
}

void uart_send_temp(int mili_c) {
    int whole = mili_c / 1000;
    int frac  = mili_c % 1000;
    if (frac < 0) frac = -frac;
    uart_puts("TEMP: ");
    uart_print_int(whole);
    uart_putc('.');
    if (frac < 100) uart_putc('0');
    if (frac <  10) uart_putc('0');
    uart_print_int(frac);
    uart_puts(" C\r\n");
}

// =========================================
// I2C DRIVER
// =========================================
static void i2c_delay() {
    for (volatile int i = 0; i < 10; i++);
}

int i2c_write_byte(uint8_t slave_addr_w, uint8_t reg_addr, uint8_t data) {
    I2C_TX_DATA  = data;           
    I2C_ADDR     = slave_addr_w;   
    I2C_SUB_ADDR = reg_addr;       
    I2C_BYTE_LEN = 1;              
    i2c_delay();

    I2C_CTRL = 0x01;               
    i2c_delay();
    I2C_CTRL = 0x00;               

    while (I2C_BUSY);

    if (I2C_NACK) {
        uart_puts("I2C NACK on write!\r\n");
        return 1;
    }
    return 0;
}

int i2c_read_bytes(uint8_t slave_addr_r, uint8_t reg_addr, uint8_t* buf, uint8_t num_bytes) {
    I2C_ADDR     = slave_addr_r;   
    I2C_SUB_ADDR = reg_addr;       
    I2C_BYTE_LEN = num_bytes;
    i2c_delay();

    I2C_CTRL = 0x01;
    i2c_delay();
    I2C_CTRL = 0x00;

    for (int b = 0; b < num_bytes; b++) {
        while (!I2C_VALID) {
            if (I2C_NACK) {
                uart_puts("I2C NACK on read!\r\n");
                return 1;
            }
        }
        buf[b] = (uint8_t)(I2C_RX_DATA & 0xFF);
        while (I2C_VALID); 
    }

    while (I2C_BUSY);  
    return 0;
}

// =========================================
// ADT7420 DRIVER
// =========================================
void adt7420_init() {
    uart_puts("Configuring ADT7420 to 16-bit mode...\r\n");
    if (i2c_write_byte(ADT7420_ADDR_W, ADT7420_REG_CFG, ADT7420_CFG_16BIT) == 0) {
        uart_puts("ADT7420 init OK\r\n");
    } else {
        uart_puts("ADT7420 init FAILED\r\n");
    }
    for (volatile int i = 0; i < 10000000; i++);
}

int adt7420_read_mili_c() {
    uint8_t buf[2] = {0, 0};
    if (i2c_read_bytes(ADT7420_ADDR_R, ADT7420_REG_TEMP, buf, 2) != 0)
        return 0;  
    uint16_t raw_u = ((uint16_t)buf[0] << 8) | buf[1];
    int16_t  raw   = (int16_t)raw_u;   
    return ((int32_t)raw * 1000) / 128;
}

// =========================================
// GRAPHICS (HORIZONTAL DESIGN)
// =========================================
void draw_rect(int x, int y, int w, int h, uint8_t col) {
    for (int r = 0; r < h; r++)
        for (int c = 0; c < w; c++)
            if ((y+r) < 240 && (x+c) < 320)
                vram[((y+r) * 320) + (x+c)] = col;
}

void build_thermometer_graphics() {
    for (int i = 0; i < 76800; i++) vram[i] = 0x00; // Clear
    
    // Draw the Bulb (Far Left)
    draw_rect(20, 100, 50, 40, 0xFF);  // Glass outline
    draw_rect(30, 110, 40, 20, 0xE0);  // Inner red fluid pool
    
    // Draw the Tube Outline (Stretching Right)
    draw_rect(70, 105, 230, 30, 0xFF); 
    
    // Draw Tick Marks Underneath
    for (int x = 70; x <= 290; x += 15) {
        draw_rect(x, 135, 2, 10, 0x92);
    }
}

// =========================================
// MAIN
// =========================================
int main() {
    build_thermometer_graphics();

    uart_puts("=== Horizontal Hyper-Sensitive Thermometer ===\r\n");

    adt7420_init();

    uart_puts("Range: ");
    uart_print_int(MIN_TEMP_MILI);
    uart_puts(" to ");
    uart_print_int(MAX_TEMP_MILI);
    uart_puts(" mC\r\n");

    // Dimensions for the internal horizontal tube
    const int tube_x = 70;
    const int tube_y = 110;
    const int tube_h = 20;
    const int tube_max_w = 225; // How far right the bar can go

    while (1) {
        int temp = adt7420_read_mili_c();
        uart_send_temp(temp);   

        int disp = temp;
        // Clamp the display to our hyper-sensitive range limits
        if (disp < MIN_TEMP_MILI) disp = MIN_TEMP_MILI;
        if (disp > MAX_TEMP_MILI) disp = MAX_TEMP_MILI;

        // Calculate the fluid width (instead of height)
        int fluid_w = ((disp - MIN_TEMP_MILI) * tube_max_w) / (MAX_TEMP_MILI - MIN_TEMP_MILI);
        int empty_w = tube_max_w - fluid_w;

        // Draw the red fluid growing to the right
        draw_rect(tube_x, tube_y, fluid_w, tube_h, 0xE0); 
        
        // Erase the remaining part of the tube (Black)
        if (empty_w > 0) {
            draw_rect(tube_x + fluid_w, tube_y, empty_w, tube_h, 0x00);       
        }

        for (volatile int i = 0; i < 2000000; i++);
    }
}
