`timescale 1ns / 1ps

module tb_bootloader();

    // --- Signals ---
    reg  clk;
    reg  resetn;
    wire [7:0] led;
    wire uart_tx;
    reg  uart_rx;

    // --- Instantiate the SoC ---
    top uut (
        .clk(clk),
        .resetn(resetn),
        .led(led),
        .uart_tx(uart_tx),
        .uart_rx(uart_rx)
    );

    // --- Clock Generation (100MHz = 10ns period) ---
    initial begin
        clk = 0;
        forever #5 clk = ~clk; 
    end

    // --- Python Script Simulator (UART TX Task) ---
    // At 100MHz and 115200 Baud, 1 bit = ~868 clock cycles = 8680ns
    task send_byte(input [7:0] data);
        integer i;
        begin
            uart_rx = 0; // Start bit
            #8680;
            for (i = 0; i < 8; i = i + 1) begin
                uart_rx = data[i]; // Data bits
                #8680;
            end
            uart_rx = 1; // Stop bit
            #8680;
        end
    endtask

    // --- Main Test Sequence ---
    initial begin
        // 1. Initialize everything
        resetn = 0;
        uart_rx = 1; // UART idle state is HIGH
        
        // 2. Release reset after 100ns
        #100 resetn = 1;

        // 3. Wait for the Bootloader to initialize and hit the UART loop
        #50000; 
        
        // ---------------------------------------------------------
        // SIMULATING THE PYTHON UPLOAD SCRIPT
        // ---------------------------------------------------------
        
        $display("[TB] Sending Upload Command ('U')...");
        send_byte(8'h55); // ASCII code for 'U'

        $display("[TB] Sending File Size (4 bytes)...");
        send_byte(8'h04); // Size Byte 0 (Little Endian for size = 4)
        send_byte(8'h00); // Size Byte 1
        send_byte(8'h00); // Size Byte 2
        send_byte(8'h00); // Size Byte 3

        $display("[TB] Sending Dummy Payload (RISC-V NOP Instruction)...");
        // Sending RISC-V NOP: addi x0, x0, 0 -> 0x00000013
        send_byte(8'h13); // Payload Byte 0
        send_byte(8'h00); // Payload Byte 1
        send_byte(8'h00); // Payload Byte 2
        send_byte(8'h00); // Payload Byte 3

        $display("[TB] Upload complete. Waiting for CPU to jump...");
        
        // 4. Give the CPU time to write to RAM and jump to 0x1000
        #100000; 
        
        $display("[TB] Simulation finished.");
        $finish;
    end

endmodule