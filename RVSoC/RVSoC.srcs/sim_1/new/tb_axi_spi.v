`timescale 1ns / 1ps

module tb_axi_spi();

    // --- Clock and Reset ---
    reg clk;
    reg resetn;

    // --- AXI-Lite Signals ---
    reg  [31:0] awaddr;
    reg         awvalid;
    wire        awready;
    reg  [31:0] wdata;
    reg  [3:0]  wstrb;
    reg         wvalid;
    wire        wready;
    wire [1:0]  bresp;
    wire        bvalid;
    reg         bready;
    reg  [31:0] araddr;
    reg         arvalid;
    wire        arready;
    wire [31:0] rdata;
    wire [1:0]  rresp;
    wire        rvalid;
    reg         rready;

    // --- SPI Signals ---
    wire spi_clk;
    wire spi_mosi;
    wire spi_miso;
    wire spi_cs_n;

    // THE LOOPBACK TRICK: Connect Output directly to Input
    assign spi_miso = spi_mosi; 

    // --- Instantiate the Wrapper ---
    axi_spi_ctrl #(
        .C_S_AXIL_DATA_WIDTH(32),
        .C_S_AXIL_ADDR_WIDTH(32),
        .CLKS_PER_HALF_BIT(2) // Speed up simulation massively!
    ) uut (
        .clk(clk),
        .resetn(resetn),
        
        .s_axil_awaddr(awaddr), .s_axil_awprot(3'b0), .s_axil_awvalid(awvalid), .s_axil_awready(awready),
        .s_axil_wdata(wdata),   .s_axil_wstrb(wstrb), .s_axil_wvalid(wvalid),   .s_axil_wready(wready),
        .s_axil_bresp(bresp),   .s_axil_bvalid(bvalid), .s_axil_bready(bready),
        .s_axil_araddr(araddr), .s_axil_arprot(3'b0), .s_axil_arvalid(arvalid), .s_axil_arready(arready),
        .s_axil_rdata(rdata),   .s_axil_rresp(rresp),   .s_axil_rvalid(rvalid),   .s_axil_rready(rready),
        
        .o_SPI_Clk(spi_clk),
        .i_SPI_MISO(spi_miso),
        .o_SPI_MOSI(spi_mosi),
        .o_SPI_CS_n(spi_cs_n)
    );

    // --- Clock Generation (100MHz) ---
    always #5 clk = ~clk;

    // ==========================================
    // AXI-LITE HELPER TASKS (CYCLE-ACCURATE)
    // ==========================================
    reg [31:0] tb_captured_data; // Global TB register to safely store read data

    task axi_write(input [31:0] addr, input [31:0] data);
        reg aw_done, w_done, b_done;
        begin
            @(posedge clk);
            awaddr  <= addr;
            awvalid <= 1'b1;
            wdata   <= data;
            wstrb   <= 4'hF;
            wvalid  <= 1'b1;
            bready  <= 1'b1;
            
            aw_done = 0; w_done  = 0; b_done  = 0;

            while (!aw_done || !w_done) begin
                @(posedge clk);
                if (awready) aw_done = 1;
                if (wready)  w_done  = 1;
                if (aw_done) awvalid <= 1'b0;
                if (w_done)  wvalid  <= 1'b0;
            end
            
            while (!b_done) begin
                @(posedge clk);
                if (bvalid) begin
                    b_done = 1;
                    // Linter fix: Check for AXI errors
                    if (bresp != 2'b00) $display("ERROR: Write to 0x%0h returned error %b", addr, bresp);
                end
            end
            bready <= 1'b0;
        end
    endtask

    task axi_read(input [31:0] addr);
        reg ar_done, r_done;
        begin
            @(posedge clk);
            araddr  <= addr;
            arvalid <= 1'b1;
            rready  <= 1'b1;
            
            ar_done = 0; r_done  = 0;

            while (!ar_done) begin
                @(posedge clk);
                if (arready) ar_done = 1;
            end
            arvalid <= 1'b0;
            
            while (!r_done) begin
                @(posedge clk);
                if (rvalid) begin
                    r_done = 1;
                    tb_captured_data = rdata; // Linter fix: Capture the live data safely!
                    // Linter fix: Check for AXI errors
                    if (rresp != 2'b00) $display("ERROR: Read from 0x%0h returned error %b", addr, rresp);
                end
            end
            rready  <= 1'b0;
        end
    endtask

    // ==========================================
    // WATCHDOG TIMER (Linter Fix: Prevent silent hangs)
    // ==========================================
    initial begin
        #100000; 
        $display("FATAL: Simulation Watchdog Timeout! A state machine is stuck.");
        $finish;
    end

    // ==========================================
    // MAIN TEST SEQUENCE
    // ==========================================
    reg [7:0] final_rx_byte;

    initial begin
        // 1. Initialize ALL Signals (Linter fix: No X's on buses)
        clk = 0; resetn = 0;
        awvalid = 0; wvalid = 0; bready = 0;
        arvalid = 0; rready = 0;
        awaddr = 0; wdata = 0; wstrb = 0; araddr = 0;
        tb_captured_data = 0;

        #100;
        @(posedge clk); resetn = 1; @(posedge clk);

        $display("--- Starting Manual CS SPI Test ---");

        $display("Pulling CS LOW...");
        axi_write(32'h0000_0004, 32'h0000_0000); 

        $display("Writing 0xAB to TX Register...");
        axi_write(32'h0000_0000, 32'h0000_00AB);

        $display("Polling Status Register until SPI finishes...");
        axi_read(32'h0000_0004); 
        while (tb_captured_data[0] == 0) begin
            axi_read(32'h0000_0004); // Linter fix: Removed stray posedge clk
        end 

        $display("Reading RX Register...");
        axi_read(32'h0000_0000);
        final_rx_byte = tb_captured_data[7:0]; // Linter fix: Save data before doing next AXI transaction

        $display("Pulling CS HIGH...");
        axi_write(32'h0000_0004, 32'h0000_0002);

        // Linter fix: Pass/Fail check happens safely at the end using the preserved variable
        if (final_rx_byte == 8'hAB)
            $display("SUCCESS! Loopback data matches.");
        else
            $display("FAILED! Expected 0xAB, got 0x%0h", final_rx_byte);

        #100;
        $finish;
    end

endmodule