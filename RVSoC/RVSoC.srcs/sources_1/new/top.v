`timescale 1ns / 1ps

module top (
    input  clk,           // 100MHz clock from Nexys 4 DDR
    input  resetn,        // Active-low reset (CPU_RESET button)
    output [7:0] led,     // 8 LEDs on the board
    output uart_tx,       // FPGA TX Pin
    input  uart_rx        // FPGA RX Pin

    // I2C Ports
    inout sda,
    inout scl
);

    // --- PicoRV32 Native Interface Signals ---
    wire        mem_valid;
    wire        mem_instr;
    reg         mem_ready;
    wire [31:0] mem_addr;
    wire [31:0] mem_wdata;
    wire [3:0]  mem_wstrb;
    reg  [31:0] mem_rdata;

<<<<<<< Updated upstream
=======

    // --- PicoRV32 AXI4-Lite Master Signals ---
    // 1. AW Channel (Address Write)
    wire        mem_axi_awvalid;
    wire        mem_axi_awready;
    wire [31:0] mem_axi_awaddr;
    wire [ 2:0] mem_axi_awprot;

    // 2. W Channel (Write Data)
    wire        mem_axi_wvalid;
    wire        mem_axi_wready;
    wire [31:0] mem_axi_wdata;
    wire [ 3:0] mem_axi_wstrb;

    // 3. B Channel (Write Response)
    wire        mem_axi_bvalid;
    wire        mem_axi_bready;

    // 4. AR Channel (Address Read)
    wire        mem_axi_arvalid;
    wire        mem_axi_arready;
    wire [31:0] mem_axi_araddr;
    wire [ 2:0] mem_axi_arprot;

    // 5. R Channel (Read Data)
    wire        mem_axi_rvalid;
    wire        mem_axi_rready;
    wire [31:0] mem_axi_rdata;

    // --- Instantiate the PicoRV32 AXI Core ---
    picorv32_axi #(
        .PROGADDR_RESET(32'h0000_0000), 
        .STACKADDR(32'h0000_1FFC)       
    ) cpu (
        .clk            (clk),
        .resetn         (resetn),
        .trap           (),

        // AXI AW
        .mem_axi_awvalid(mem_axi_awvalid),
        .mem_axi_awready(mem_axi_awready),
        .mem_axi_awaddr (mem_axi_awaddr),
        .mem_axi_awprot (mem_axi_awprot),

        // AXI W
        .mem_axi_wvalid (mem_axi_wvalid),
        .mem_axi_wready (mem_axi_wready),
        .mem_axi_wdata  (mem_axi_wdata),
        .mem_axi_wstrb  (mem_axi_wstrb),

        // AXI B
        .mem_axi_bvalid (mem_axi_bvalid),
        .mem_axi_bready (mem_axi_bready),

        // AXI AR
        .mem_axi_arvalid(mem_axi_arvalid),
        .mem_axi_arready(mem_axi_arready),
        .mem_axi_araddr (mem_axi_araddr),
        .mem_axi_arprot (mem_axi_arprot),

        // AXI R
        .mem_axi_rvalid (mem_axi_rvalid),
        .mem_axi_rready (mem_axi_rready),
        .mem_axi_rdata  (mem_axi_rdata)
    );



    // --- Slave 0: RAM Wires ---
    wire [31:0] ram_awaddr;  wire [2:0] ram_awprot;  wire ram_awvalid; wire ram_awready;
    wire [31:0] ram_wdata;   wire [3:0] ram_wstrb;   wire ram_wvalid;  wire ram_wready;
    wire [1:0]  ram_bresp;   wire ram_bvalid;        wire ram_bready;
    wire [31:0] ram_araddr;  wire [2:0] ram_arprot;  wire ram_arvalid; wire ram_arready;
    reg  [31:0] ram_rdata;   wire [1:0] ram_rresp;   wire ram_rvalid;  wire ram_rready;

    // --- Slave 1: LED Wires ---
    wire [31:0] led_awaddr;  wire [2:0] led_awprot;  wire led_awvalid; wire led_awready;
    wire [31:0] led_wdata;   wire [3:0] led_wstrb;   wire led_wvalid;  wire led_wready;
    wire [1:0]  led_bresp;   wire led_bvalid;        wire led_bready;
    wire [31:0] led_araddr;  wire [2:0] led_arprot;  wire led_arvalid; wire led_arready;
    reg  [31:0] led_rdata;   wire [1:0] led_rresp;   wire led_rvalid;  wire led_rready;

    // --- Slave 2: UART Wires ---
    wire [31:0] uart_awaddr; wire [2:0] uart_awprot; wire uart_awvalid; wire uart_awready;
    wire [31:0] uart_wdata;  wire [3:0] uart_wstrb;  wire uart_wvalid;  wire uart_wready;
    wire [1:0]  uart_bresp;  wire uart_bvalid;       wire uart_bready;
    wire [31:0] uart_araddr; wire [2:0] uart_arprot; wire uart_arvalid; wire uart_arready;
    reg  [31:0] uart_rdata;  wire [1:0] uart_rresp;  wire uart_rvalid;  wire uart_rready;

    // --- Slave 3: I2C Wires ---
    wire [31:0] i2c_awaddr;  wire [2:0] i2c_awprot;  wire i2c_awvalid;  wire i2c_awready;
    wire [31:0] i2c_wdata;   wire [3:0] i2c_wstrb;   wire i2c_wvalid;   wire i2c_wready;
    wire [1:0]  i2c_bresp;   wire i2c_bvalid;        wire i2c_bready;
    wire [31:0] i2c_araddr;  wire [2:0] i2c_arprot;  wire i2c_arvalid;  wire i2c_arready;
    reg  [31:0] i2c_rdata;   wire [1:0] i2c_rresp;   wire i2c_rvalid;   wire i2c_rready;

    // --- Instantiate Alex Forencich's AXI-Lite Crossbar ---
    axil_crossbar #(
        .S_COUNT(1), // 1 Master (CPU)
        .M_COUNT(4), // 4 Slaves (I2C, UART, LED, RAM)
        .DATA_WIDTH(32),
        .ADDR_WIDTH(32),
        // Define the memory map: {Slave 2, Slave 1, Slave 0}
        .M_BASE_ADDR  ({32'h3000_0000, 32'h2000_0000, 32'h1000_0000, 32'h0000_0000}),
        .M_ADDR_WIDTH ({32'd16,        32'd16,        32'd16,        32'd16}) // Give each slave a 64KB address space
    ) axi_router (
        .clk(clk),
        .rst(!resetn), // Crossbar usually takes active-high reset

        // CPU Inputs (Connect to the wires coming out of picorv32_axi)
        .s_axil_awaddr (mem_axi_awaddr),  .s_axil_awprot (mem_axi_awprot),
        .s_axil_awvalid(mem_axi_awvalid), .s_axil_awready(mem_axi_awready),
        .s_axil_wdata  (mem_axi_wdata),   .s_axil_wstrb  (mem_axi_wstrb),
        .s_axil_wvalid (mem_axi_wvalid),  .s_axil_wready (mem_axi_wready),
        .s_axil_bresp  (),                .s_axil_bvalid (mem_axi_bvalid), // CPU ignores bresp
        .s_axil_bready (mem_axi_bready),
        .s_axil_araddr (mem_axi_araddr),  .s_axil_arprot (mem_axi_arprot),
        .s_axil_arvalid(mem_axi_arvalid), .s_axil_arready(mem_axi_arready),
        .s_axil_rdata  (mem_axi_rdata),   .s_axil_rresp  (),               // CPU ignores rresp
        .s_axil_rvalid (mem_axi_rvalid),  .s_axil_rready (mem_axi_rready),

        // Slave Outputs (Concatenated as {I2C, UART, LED, RAM})
        .m_axil_awaddr ({i2c_awaddr,  uart_awaddr,  led_awaddr,  ram_awaddr}),
        .m_axil_awprot ({i2c_awprot,  uart_awprot,  led_awprot,  ram_awprot}),
        .m_axil_awvalid({i2c_awvalid, uart_awvalid, led_awvalid, ram_awvalid}),
        .m_axil_awready({i2c_awready, uart_awready, led_awready, ram_awready}),
        
        .m_axil_wdata  ({i2c_wdata,  uart_wdata,   led_wdata,   ram_wdata}),
        .m_axil_wstrb  ({i2c_wstrb,  uart_wstrb,   led_wstrb,   ram_wstrb}),
        .m_axil_wvalid ({i2c_wvalid, uart_wvalid,  led_wvalid,  ram_wvalid}),
        .m_axil_wready ({i2c_wready, uart_wready,  led_wready,  ram_wready}),
        
        .m_axil_bresp  ({i2c_bresp,  uart_bresp,   led_bresp,   ram_bresp}),
        .m_axil_bvalid ({i2c_bvalid, uart_bvalid,  led_bvalid,  ram_bvalid}),
        .m_axil_bready ({i2c_bready, uart_bready,  led_bready,  ram_bready}),
        
        .m_axil_araddr ({i2c_araddr,  uart_araddr,  led_araddr,  ram_araddr}),
        .m_axil_arprot ({i2c_arprot,  uart_arprot,  led_arprot,  ram_arprot}),
        .m_axil_arvalid({i2c_arvalid, uart_arvalid, led_arvalid, ram_arvalid}),
        .m_axil_arready({i2c_arready, uart_arready, led_arready, ram_arready}),
        
        .m_axil_rdata  ({i2c_rdata,  uart_rdata,   led_rdata,   ram_rdata}),
        .m_axil_rresp  ({i2c_rresp, uart_rresp,   led_rresp,   ram_rresp}),
        .m_axil_rvalid ({i2c_rvalid, uart_rvalid,  led_rvalid,  ram_rvalid}),
        .m_axil_rready ({i2c_rready, uart_rready,  led_rready,  ram_rready})
    );

   
>>>>>>> Stashed changes
    // --- UART AXI-Stream Bridge Wires ---
    wire [7:0] uart_rx_tdata;
    wire       uart_rx_tvalid;
    reg        uart_rx_tready;
    wire       uart_tx_tready;
    reg        uart_tx_tvalid;
    
    // Prescale for 115200 baud: 100MHz / (115200 * 8) = 108
    wire [15:0] uart_prescale = 16'd108; 

    // --- Instantiate the PicoRV32 Core ---
    picorv32 #(
        .PROGADDR_RESET(32'h 0000_0000), 
        .STACKADDR(32'h 0000_1000)       
    ) cpu (
        .clk(clk),
        .resetn(resetn),
        .trap(), 
        .mem_valid(mem_valid),
        .mem_instr(mem_instr),
        .mem_ready(mem_ready),
        .mem_addr(mem_addr),
        .mem_wdata(mem_wdata),
        .mem_wstrb(mem_wstrb),
        .mem_rdata(mem_rdata)
    );

    // --- 4KB Block RAM (1024 words x 32 bits) ---
    reg [31:0] memory [0:1023];
    initial begin
        $readmemh("firmware.hex", memory);
    end

    // --- 8-Bit LED Register ---
    reg [7:0] led_reg;
    assign led = led_reg;

    // --- Instantiate Alex Forencich UART ---
    // Ensure uart.v, uart_tx.v, and uart_rx.v are in your project
    uart #( .DATA_WIDTH(8) ) uart_inst (
        .clk(clk),
        .rst(!resetn), // Active-high reset for the UART module
        .s_axis_tdata(mem_wdata[7:0]),
        .s_axis_tvalid(uart_tx_tvalid),
        .s_axis_tready(uart_tx_tready),
        .m_axis_tdata(uart_rx_tdata),
        .m_axis_tvalid(uart_rx_tvalid),
        .m_axis_tready(uart_rx_tready),
        .rxd(uart_rx),
        .txd(uart_tx),
        .tx_busy(),
        .rx_busy(),
        .prescale(uart_prescale)
    );

    // --- Memory Routing & Control Logic ---
    always @(posedge clk) begin
        if (!resetn) begin
            mem_ready <= 0;
            led_reg   <= 8'b00000000;
            uart_tx_tvalid <= 0;
            uart_rx_tready <= 0;
        end else begin
            mem_ready <= 0;
            uart_tx_tvalid <= 0;
            uart_rx_tready <= 0;

            if (mem_valid && !mem_ready) begin
                
                // 1. RAM Access (0x0000_0000 to 0x0000_0FFF)
                if (mem_addr < 32'h0000_1000) begin
                    mem_ready <= 1;
                    if (mem_wstrb[0]) memory[mem_addr[11:2]][7:0]   <= mem_wdata[7:0];
                    if (mem_wstrb[1]) memory[mem_addr[11:2]][15:8]  <= mem_wdata[15:8];
                    if (mem_wstrb[2]) memory[mem_addr[11:2]][23:16] <= mem_wdata[23:16];
                    if (mem_wstrb[3]) memory[mem_addr[11:2]][31:24] <= mem_wdata[31:24];
                    mem_rdata <= memory[mem_addr[11:2]];
                end
                
                // 2. LED Address (0x1000_0000)
                else if (mem_addr == 32'h1000_0000) begin
                    mem_ready <= 1;
                    if (mem_wstrb != 0) begin
                        led_reg <= mem_wdata[7:0]; // FIXED: Capture all 8 bits
                    end
                    mem_rdata <= {24'b0, led_reg};
                end
                
                // 3. UART Peripheral (0x2000_0000)
                else if (mem_addr[31:4] == 28'h2000000) begin
                    case (mem_addr[3:0])
                        4'h0: begin // UART DATA
                            if (mem_wstrb != 0) begin
                                if (uart_tx_tready) begin
                                    uart_tx_tvalid <= 1;
                                    mem_ready <= 1;
                                end
                            end else begin
                                mem_rdata <= {24'b0, uart_rx_tdata};
                                uart_rx_tready <= 1; // Signal that we've read the byte
                                mem_ready <= 1;
                            end
                        end
                        4'h4: begin // UART STATUS
                            mem_ready <= 1;
                            // Bit 0: TX Ready, Bit 1: RX Data Available
                            mem_rdata <= {30'b0, uart_rx_tvalid, uart_tx_tready};
                        end
                    endcase
                end

                // 4. Out of bounds catch-all
                else begin
                    mem_ready <= 1;
                    mem_rdata <= 32'hFFFF_FFFF;
                end
            end
        end
    end

<<<<<<< Updated upstream
endmodule
=======
    // UART Read Channel
    reg uart_rvalid_reg;
    assign uart_arready = !uart_rvalid_reg || uart_rready;
    assign uart_rvalid  = uart_rvalid_reg;
    assign uart_rresp   = 2'b00;

    always @(posedge clk) begin
        if (!resetn) begin
            uart_rvalid_reg <= 1'b0;
            uart_rx_tready  <= 1'b0;
        end else begin
            uart_rx_tready <= 1'b0;
            if (uart_arvalid && uart_arready) begin
                uart_rvalid_reg <= 1'b1;
                if (uart_araddr[3:0] == 4'h0) begin
                    uart_rdata <= {24'b0, uart_rx_tdata};
                    uart_rx_tready <= 1'b1; 
                end else if (uart_araddr[3:0] == 4'h4) begin
                    uart_rdata <= {30'b0, uart_rx_tvalid, uart_tx_tready};
                end else begin
                    uart_rdata <= 32'h0;
                end
            end else if (uart_rready) begin
                uart_rvalid_reg <= 1'b0;
            end
        end
    end

    // ------------------------------------------
    // 4. AXI-Lite I2C Wrapper
    // ------------------------------------------
    
    reg i2c_aw_latched, i2c_w_latched, i2c_bvalid_reg;
    reg [31:0] i2c_awaddr_reg, i2c_wdata_reg;
    
    // I2C Master Registers
    reg [7:0]  i_addr_w_rw_reg;
    reg [15:0] i_sub_addr_reg;
    reg        i_sub_len_reg;
    reg [23:0] i_byte_len_reg;
    reg [7:0]  i_data_write_reg;
    reg        req_trans_reg;
    
    // I2C Master Wires (Outputs from IP)
    wire [7:0] data_out;
    wire       valid_out;
    wire       req_data_chunk;
    wire       busy;
    wire       nack;

    assign i2c_awready = !i2c_aw_latched && !i2c_bvalid_reg;
    assign i2c_wready  = !i2c_w_latched  && !i2c_bvalid_reg;
    assign i2c_bvalid  = i2c_bvalid_reg;
    assign i2c_bresp   = 2'b00;

    // I2C Write Channel
    always @(posedge clk) begin
        if (!resetn) begin
            i2c_aw_latched <= 0; i2c_w_latched <= 0; i2c_bvalid_reg <= 0;
            req_trans_reg <= 0;
            i_addr_w_rw_reg <= 0; i_sub_addr_reg <= 0; i_sub_len_reg <= 0; 
            i_byte_len_reg <= 0; i_data_write_reg <= 0;
        end else begin
            req_trans_reg <= 0; // Default off to create a 1-clock cycle pulse
            
            // Latch Write Channels
            if (i2c_awvalid && i2c_awready) begin
                i2c_awaddr_reg <= i2c_awaddr;
                i2c_aw_latched <= 1;
            end
            if (i2c_wvalid && i2c_wready) begin
                i2c_wdata_reg <= i2c_wdata;
                i2c_w_latched <= 1;
            end
            
            // Execute Write
            if (i2c_aw_latched && i2c_w_latched && !i2c_bvalid_reg) begin
                if (i2c_awaddr_reg[7:0] == 8'h00) begin 
                    req_trans_reg <= i2c_wdata_reg[0]; // Bit 0 starts the transaction
                    i_sub_len_reg <= i2c_wdata_reg[1]; // Bit 1 sets sub_addr length mode
                    i2c_bvalid_reg <= 1;
                end else if (i2c_awaddr_reg[7:0] == 8'h04) begin
                    i_addr_w_rw_reg <= i2c_wdata_reg[7:0];
                    i2c_bvalid_reg <= 1;
                end else if (i2c_awaddr_reg[7:0] == 8'h08) begin
                    i_sub_addr_reg <= i2c_wdata_reg[15:0];
                    i2c_bvalid_reg <= 1;
                end else if (i2c_awaddr_reg[7:0] == 8'h0C) begin
                    i_byte_len_reg <= i2c_wdata_reg[23:0];
                    i2c_bvalid_reg <= 1;
                end else if (i2c_awaddr_reg[7:0] == 8'h10) begin
                    i_data_write_reg <= i2c_wdata_reg[7:0];
                    i2c_bvalid_reg <= 1;
                end else begin
                    i2c_bvalid_reg <= 1; // ACK unknown addresses to prevent bus hang
                end
            end else if (i2c_bvalid_reg && i2c_bready) begin
                i2c_bvalid_reg <= 0; i2c_aw_latched <= 0; i2c_w_latched <= 0;
            end
        end
    end

    // I2C Read Channel
    reg i2c_rvalid_reg;
    assign i2c_arready = !i2c_rvalid_reg || i2c_rready;
    assign i2c_rvalid  = i2c_rvalid_reg;
    assign i2c_rresp   = 2'b00;

    always @(posedge clk) begin
        if (!resetn) begin
            i2c_rvalid_reg <= 1'b0;
        end else begin
            if (i2c_arvalid && i2c_arready) begin
                i2c_rvalid_reg <= 1'b1;
                if (i2c_araddr[7:0] == 8'h00) begin
                    // Read Status: Bit 3=req_chunk, Bit 2=valid_out, Bit 1=nack, Bit 0=busy
                    i2c_rdata <= {28'b0, req_data_chunk, valid_out, nack, busy};
                end else if (i2c_araddr[7:0] == 8'h14) begin
                    // Read Data Output
                    i2c_rdata <= {24'b0, data_out};
                end else begin
                    i2c_rdata <= 32'h0;
                end
            end else if (i2c_rready) begin
                i2c_rvalid_reg <= 1'b0;
            end
        end
    end

    // --- Instantiate the I2C Master ---
    i2c_master i2c_inst (
        .i_clk          (clk),
        .reset_n        (resetn),        // Uses active-low reset from your top module
        .i_addr_w_rw    (i_addr_w_rw_reg),
        .i_sub_addr     (i_sub_addr_reg),
        .i_sub_len      (i_sub_len_reg),
        .i_byte_len     (i_byte_len_reg),
        .i_data_write   (i_data_write_reg),
        .req_trans      (req_trans_reg), // Pulsed by AXI write
        
        .data_out       (data_out),
        .valid_out      (valid_out),
        
        .scl_o          (scl),           // Tied to top-level inout pin
        .sda_o          (sda),           // Tied to top-level inout pin
        
        .req_data_chunk (req_data_chunk),
        .busy           (busy),
        .nack           (nack)
    );

endmodule

>>>>>>> Stashed changes
