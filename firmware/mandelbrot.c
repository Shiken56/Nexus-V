#include <stdint.h>

// --- Memory Map ---
#define VRAM_BASE 0x40000000
#define UART_BASE 0x20000000

volatile uint8_t* const vram = (volatile uint8_t*)VRAM_BASE;
volatile uint32_t* const uart_data   = (volatile uint32_t*)UART_BASE;
volatile uint32_t* const uart_stat   = (volatile uint32_t*)(UART_BASE + 4);

// --- Fixed-Point Math Parameters ---
#define SHIFT 20
#define ONE (1 << SHIFT)
#define MULT(a, b) (int32_t)((((int64_t)(a)) * ((int64_t)(b))) >> SHIFT)
#define MAX_ITER 64

// --- UART & Reset Functions (From older apps) ---
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

// Polls the RX line for the 0x7F reset signal from upload.py
void check_reboot() {
    if ((*uart_stat) & 0x02) { 
        if (((*uart_data) & 0xFF) == 0x7F) {
            software_reset();
        }
    }
}

// --- Color Palette (RGB332 Mapping for depth) ---
uint8_t get_fractal_color(int iter) {
    if (iter == MAX_ITER) return 0x00; // Black inside the set
    int c = iter % 16;
    if (c < 4)  return 0x03 + (c << 2);      // Blues
    if (c < 8)  return 0x1F - ((c-4) << 2);  // Cyans
    if (c < 12) return 0x1C + ((c-8) << 5);  // Greens/Yellows
    return 0xFC - ((c-12) << 2);             // Reds/Oranges
}

int main() {
    uart_print("\r\n*** Starting Mandelbrot Render ***\r\n");

    // Fractal bounding box coordinates in Q12.20
    int32_t x_min = (int32_t)(-2.0 * ONE);
    int32_t x_max = (int32_t)( 1.0 * ONE);
    int32_t y_min = (int32_t)(-1.2 * ONE);
    int32_t y_max = (int32_t)( 1.2 * ONE);

    int32_t dx = (x_max - x_min) / 320;
    int32_t dy = (y_max - y_min) / 240;

    int32_t c_y = y_min;

    // Render loop
    for (int py = 0; py < 240; py++) {
        int32_t c_x = x_min;
        
        for (int px = 0; px < 320; px++) {
            
            // AGGRESSIVE POLLING: Check for reset on every single pixel!
            check_reboot();

            int32_t z_x = 0;
            int32_t z_y = 0;
            int iter = 0;

            // Z = Z^2 + C
            while (iter < MAX_ITER) {
                int32_t z_x_sq = MULT(z_x, z_x);
                int32_t z_y_sq = MULT(z_y, z_y);

                // Escape condition: x^2 + y^2 > 4.0
                if ((z_x_sq + z_y_sq) > (4 * ONE)) break;

                int32_t temp_z_x = z_x_sq - z_y_sq + c_x;
                z_y = (MULT(z_x, z_y) << 1) + c_y; 
                z_x = temp_z_x;

                iter++;
            }

            vram[(py * 320) + px] = get_fractal_color(iter);
            c_x += dx;
        }
        c_y += dy;
    }

    uart_print("\r\n[+] Render Complete. Holding image.\r\n");

    // Infinite loop when done to hold the image, polling continuously
    while(1) {
        check_reboot(); 
    }
}