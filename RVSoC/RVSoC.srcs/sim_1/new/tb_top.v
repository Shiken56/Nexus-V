`timescale 1ns / 1ps

module tb_top();

    // 1. Declare signals to connect to our top module
    reg clk;
    reg resetn;
    wire led;

    // 2. Instantiate the top module (Device Under Test)
    top dut (
        .clk(clk),
        .resetn(resetn),
        .led(led)
    );

    // 3. Generate a 100MHz Clock (10ns period -> toggle every 5ns)
    always #5 clk = ~clk;

    // 4. Test Sequence
    initial begin
        // Initialize inputs
        clk = 0;
        resetn = 0; // Assert Active-Low Reset

        // Wait 100ns to let everything settle
        #100;

        // Release reset to wake up the processor
        resetn = 1;

        // The processor is now running! 
        // We will let the simulation run for 50,000 nanoseconds to watch the LED toggle
        #50000;
        
        // Stop the simulation
        $display("Simulation complete.");
        $finish;
    end

    // Optional: Monitor the LED output in the console
    always @(posedge led or negedge led) begin
        $display("Time: %0t ns | LED changed state to: %b", $time, led);
    end

endmodule