`timescale 1ns / 1ps

module tb_top_vga;

    // --- Inputs to top ---
    reg clk;
    reg resetn;
    reg uart_rx;
    reg spi_miso;

    // --- Outputs from top ---
    wire [7:0] led;
    wire uart_tx;
    wire [3:0] vga_r, vga_g, vga_b;
    wire vga_hs, vga_vs;

    // --- Instantiate the Complete Top Module ---
    top uut (
        .clk(clk),
        .resetn(resetn),
        .led(led),
        .uart_tx(uart_tx),
        .uart_rx(uart_rx),
        .spi_miso(spi_miso),
        .vga_r(vga_r), .vga_g(vga_g), .vga_b(vga_b),
        .vga_hs(vga_hs), .vga_vs(vga_vs)
    );

    // --- Clock Generation (100MHz) ---
    always #5 clk = ~clk;

    initial begin
        clk = 0;
        resetn = 0;
        uart_rx = 1;
        spi_miso = 0;

        $display("Starting Top-Level Integration Test...");
        #100 resetn = 1; // Release reset
        
        // Wait for PLL/VGA Clock to stabilize
        #200;

        // --- MANUALLY SIMULATE A CPU WRITE TO VRAM ---
        // Address 0x4000_0000 (Grid 0,0), Data 0xE0 (Red in RGB332)
        @(posedge clk);
        force uut.mem_axi_awaddr = 32'h4000_0000;
        force uut.mem_axi_awvalid = 1'b1;
        force uut.mem_axi_wdata = 32'h0000_00E0; 
        force uut.mem_axi_wstrb = 4'b0001;
        force uut.mem_axi_wvalid = 1'b1;

        // Wait for handshake
        wait(uut.mem_axi_awready && uut.mem_axi_wready);
        @(posedge clk);
        
        // Release the bus back to the CPU
        release uut.mem_axi_awaddr;
        release uut.mem_axi_awvalid;
        release uut.mem_axi_wdata;
        release uut.mem_axi_wstrb;
        release uut.mem_axi_wvalid;
        
        $display("[+] CPU Write to VRAM Simulated successfully.");

        // We will let the simulation run for a full frame (approx 17ms)
        // because we don't know where the beam currently is.
        $display("[*] Waiting for VGA beam to scan the screen...");
        
        // Wait for up to 20ms to ensure a full frame is drawn
        #20_000_000; 
        
        $display("====================================");
        $display("Simulation Complete. Check Waveforms");
        $display("====================================");
        $finish;
    end

    // --- NEW: Continuous Monitor Block ---
    // This watches the pins constantly. If red ever flashes on the screen, we win!
    reg test_passed = 0;
    always @(posedge clk) begin
        if (vga_r > 0) begin
            if (!test_passed) begin
                $display("[SUCCESS] Color detected on VGA pins at H:%0d, V:%0d!", uut.vga_inst.h_cnt, uut.vga_inst.v_cnt);
                test_passed = 1; // Latch it so it doesn't spam the terminal
            end
        end
    end
endmodule
