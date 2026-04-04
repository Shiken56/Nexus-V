module vga_driver (
    input wire clk_100,      // System 100MHz Clock
    input wire resetn,       // Active-low reset
    input wire [7:0] vram_data, // Pixel color from VRAM
    
    output reg [8:0] vram_addr, // Address to read from VRAM
    output reg [3:0] vga_r,
    output reg [3:0] vga_g,
    output reg [3:0] vga_b,
    output reg hsync,
    output reg vsync
);

    // --- 25MHz Clock Enable Generation ---
    reg [1:0] ce_counter;
    wire ce_25 = (ce_counter == 2'b11); // Pulses HIGH once every 4 cycles

    always @(posedge clk_100) begin
        if (!resetn) ce_counter <= 0;
        else ce_counter <= ce_counter + 1;
    end

    // --- VGA Timing Parameters (640x480 @ 60Hz) ---
    parameter H_ACTIVE = 640, H_FRONT = 16,  H_SYNC = 96, H_BACK = 48, H_TOTAL = 800;
    parameter V_ACTIVE = 480, V_FRONT = 10,  V_SYNC = 2,  V_BACK = 33, V_TOTAL = 525;

    reg [9:0] h_cnt;
    reg [9:0] v_cnt;

    // --- Screen Counters ---
    always @(posedge clk_100) begin
        if (!resetn) begin
            h_cnt <= 0;
            v_cnt <= 0;
        end else if (ce_25) begin // ONLY act when the CE pulses!
            if (h_cnt == H_TOTAL - 1) begin
                h_cnt <= 0;
                if (v_cnt == V_TOTAL - 1)
                    v_cnt <= 0;
                else
                    v_cnt <= v_cnt + 1;
            end else begin
                h_cnt <= h_cnt + 1;
            end
        end
    end

    // --- Sync & Pixel Logic ---
    wire in_active_region = (h_cnt < H_ACTIVE) && (v_cnt < V_ACTIVE);

    always @(posedge clk_100) begin
        if (!resetn) begin
            hsync <= 1'b1;
            vsync <= 1'b1;
            vga_r <= 4'h0; vga_g <= 4'h0; vga_b <= 4'h0;
        end else if (ce_25) begin
            // Active Low Syncs
            hsync <= ~(h_cnt >= (H_ACTIVE + H_FRONT) && h_cnt < (H_ACTIVE + H_FRONT + H_SYNC));
            vsync <= ~(v_cnt >= (V_ACTIVE + V_FRONT) && v_cnt < (V_ACTIVE + V_FRONT + V_SYNC));

            // Output Color OR Blanking
            if (in_active_region) begin
                // Assuming RGB332 format: RRRGGGBB
                vga_r <= {vram_data[7:5], 1'b0}; 
                vga_g <= {vram_data[4:2], 1'b0};
                vga_b <= {vram_data[1:0], 2'b00};
            end else begin
                vga_r <= 4'h0; 
                vga_g <= 4'h0; 
                vga_b <= 4'h0;
            end
        end
    end

    // --- Calculate VRAM Address based on Grid ---
    // (20x15 grid on a 640x480 screen = 32x32 pixel tiles)
    wire [4:0] grid_x = h_cnt[9:5]; // Divide by 32
    wire [4:0] grid_y = v_cnt[9:5]; // Divide by 32

    always @(posedge clk_100) begin
        if (ce_25) begin
            if (in_active_region)
                vram_addr <= (grid_y * 20) + grid_x;
            else
                vram_addr <= 0; // Prevent out-of-bounds reads
        end
    end

endmodule
