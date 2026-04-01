`timescale 1ns / 1ps

module tb_top();

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

    // --- Reset Sequence ---
    initial begin
        // Initialize everything
        resetn = 0;
        uart_rx = 1; // UART idle state is HIGH
        
        // Hold reset for 100ns, then release
        #100;
        resetn = 1;

        // Let the processor run for a solid chunk of time
        // 50 milliseconds should be enough to boot and print a short string
        #50_000_000; 
        
        $display("\n[TB] Simulation Time Limit Reached.");
        $finish;
    end

    // --- Magic UART Receiver (Prints TX to Console) ---
    // At 100MHz and 115200 Baud, 1 bit = ~868 clock cycles = 8680ns
    reg [7:0] rx_byte;
    integer i;
    
    always @(negedge uart_tx) begin
        #4340; // Wait half a bit period to sample perfectly in the middle
        if (uart_tx == 0) begin // Verify it's a valid Start Bit
            for (i = 0; i < 8; i = i + 1) begin
                #8680; // Wait one full bit period
                rx_byte[i] = uart_tx; // Sample the data bit
            end
            #8680; // Wait for the Stop Bit
            
            // Print the decoded ASCII character to the Vivado Tcl Console!
            $write("%c", rx_byte); 
        end
    end

endmodule