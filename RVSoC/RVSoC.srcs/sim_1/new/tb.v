`timescale 1ns / 1ps

module tb;
    reg clk;
    reg resetn;
    wire [7:0] led;
    wire uart_tx;
    reg uart_rx;

    // Instantiate your Top Module
    top uut (
        .clk(clk),
        .resetn(resetn),
        .led(led),
        .uart_tx(uart_tx),
        .uart_rx(uart_rx)
    );

    // 100MHz Clock Generation
    always #5 clk = ~clk;

    // --- UART TX Bit-Banging Task (Simulates the PC/Python Script) ---
    // 100MHz / 115200 baud = ~868 clock cycles per bit (8680 ns)
    task send_uart_byte;
        input [7:0] data;
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

    initial begin
        clk = 0;
        resetn = 0;
        uart_rx = 1; // Idle high
        
        #100 resetn = 1; // Release reset
        
        // Wait for CPU and bootloader to initialize
        #50000;
        
        // --- SIMULATE THE PYTHON SCRIPT UPLOADING FIRMWARE ---
        $display("Sending Upload Command...");
        send_uart_byte("U");
        
        $display("Sending File Size (e.g., 4 bytes long just to test)...");
        send_uart_byte(8'h04); // Size Byte 0
        send_uart_byte(8'h00); // Size Byte 1
        send_uart_byte(8'h00); // Size Byte 2
        send_uart_byte(8'h00); // Size Byte 3
        
        $display("Sending Dummy Firmware Data...");
        send_uart_byte(8'hAA); // Dummy Instruction 1
        send_uart_byte(8'hBB);
        send_uart_byte(8'hCC);
        send_uart_byte(8'hDD);
        
        $display("Done uploading. Watch the waveform to see if CPU jumps to 0x1000!");
        
        #500000 $finish;
    end
endmodule