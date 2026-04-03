module top (
    input  clk,           // 100MHz clock from Nexys 4 DDR
    input  resetn,        // Active-low reset (CPU_RESET button)
    output [7:0] led,     // 8 LEDs on the board
    output uart_tx,       // FPGA TX Pin
    input  uart_rx,        // FPGA RX Pin
 
    // --- NEW: ADXL362 SPI Pins ---
    output spi_clk,
    output spi_mosi,
    input  spi_miso,
    output spi_cs_n
);
    
    
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

    // --- Slave 3: SPI Wires ---
    wire [31:0] spi_awaddr;  wire [2:0] spi_awprot;  wire spi_awvalid; wire spi_awready;
    wire [31:0] spi_wdata;   wire [3:0] spi_wstrb;   wire spi_wvalid;  wire spi_wready;
    wire [1:0]  spi_bresp;   wire spi_bvalid;        wire spi_bready;
    wire [31:0] spi_araddr;  wire [2:0] spi_arprot;  wire spi_arvalid; wire spi_arready;
    wire [31:0] spi_rdata;   wire [1:0] spi_rresp;   wire spi_rvalid;  wire spi_rready;

    // --- Instantiate Alex Forencich's AXI-Lite Crossbar ---
    axil_crossbar #(
        .S_COUNT(1), // 1 Master (CPU)
        .M_COUNT(4), // 4 Slaves (SPI, UART, LED, RAM)
        .DATA_WIDTH(32),
        .ADDR_WIDTH(32),
        // Define the memory map: {Slave 3, Slave 2, Slave 1, Slave 0}
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

        // Slave Outputs (Concatenated as {SPI, UART, LED, RAM})
        .m_axil_awaddr ({spi_awaddr,  uart_awaddr,  led_awaddr,  ram_awaddr}),
        .m_axil_awprot ({spi_awprot,  uart_awprot,  led_awprot,  ram_awprot}),
        .m_axil_awvalid({spi_awvalid, uart_awvalid, led_awvalid, ram_awvalid}),
        .m_axil_awready({spi_awready, uart_awready, led_awready, ram_awready}),
        
        .m_axil_wdata  ({spi_wdata,   uart_wdata,   led_wdata,   ram_wdata}),
        .m_axil_wstrb  ({spi_wstrb,   uart_wstrb,   led_wstrb,   ram_wstrb}),
        .m_axil_wvalid ({spi_wvalid,  uart_wvalid,  led_wvalid,  ram_wvalid}),
        .m_axil_wready ({spi_wready,  uart_wready,  led_wready,  ram_wready}),
        
        .m_axil_bresp  ({spi_bresp,   uart_bresp,   led_bresp,   ram_bresp}),
        .m_axil_bvalid ({spi_bvalid,  uart_bvalid,  led_bvalid,  ram_bvalid}),
        .m_axil_bready ({spi_bready,  uart_bready,  led_bready,  ram_bready}),
        
        .m_axil_araddr ({spi_araddr,  uart_araddr,  led_araddr,  ram_araddr}),
        .m_axil_arprot ({spi_arprot,  uart_arprot,  led_arprot,  ram_arprot}),
        .m_axil_arvalid({spi_arvalid, uart_arvalid, led_arvalid, ram_arvalid}),
        .m_axil_arready({spi_arready, uart_arready, led_arready, ram_arready}),
        
        .m_axil_rdata  ({spi_rdata,   uart_rdata,   led_rdata,   ram_rdata}),
        .m_axil_rresp  ({spi_rresp,   uart_rresp,   led_rresp,   ram_rresp}),
        .m_axil_rvalid ({spi_rvalid,  uart_rvalid,  led_rvalid,  ram_rvalid}),
        .m_axil_rready ({spi_rready,  uart_rready,  led_rready,  ram_rready})
    );

   
    // --- UART AXI-Stream Bridge Wires ---
    wire [7:0] uart_rx_tdata;
    wire       uart_rx_tvalid;
    reg        uart_rx_tready;
    wire       uart_tx_tready;
    reg        uart_tx_tvalid;
    
    // Prescale for 115200 baud: 100MHz / (115200 * 8) = 108
    wire [15:0] uart_prescale = 16'd108; 

    reg [7:0]  tx_data_reg; // Holds data for the physical UART IP

    // --- Instantiate Alex Forencich UART ---
    uart #( .DATA_WIDTH(8) ) uart_inst (
        .clk(clk),
        .rst(!resetn), // Active-high reset for the UART module
        .s_axis_tdata(tx_data_reg),  
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

    // --- 8-Bit LED Register ---
    reg [7:0] led_reg;
    assign led = led_reg;

    // ==========================================
    // PHASE 3: BULLETPROOF AXI-LITE WRAPPERS
    // ==========================================

    // ------------------------------------------
    // 1. AXI-Lite RAM (8KB)
    // ------------------------------------------
    reg [31:0] memory [0:2047];
    initial begin
        $readmemh("D:/Projects/Nexus-V/firmware/firmware.hex", memory);
    end

    // RAM Write Channel (Independent Latching)
    reg ram_aw_latched, ram_w_latched, ram_bvalid_reg;
    reg [31:0] ram_awaddr_reg, ram_wdata_reg;
    reg [3:0]  ram_wstrb_reg;

    assign ram_awready = !ram_aw_latched && !ram_bvalid_reg;
    assign ram_wready  = !ram_w_latched  && !ram_bvalid_reg;
    assign ram_bvalid  = ram_bvalid_reg;
    assign ram_bresp   = 2'b00;

    always @(posedge clk) begin
        if (!resetn) begin
            ram_aw_latched <= 0; ram_w_latched <= 0; ram_bvalid_reg <= 0;
        end else begin
            // 1. Latch Address
            if (ram_awvalid && ram_awready) begin
                ram_awaddr_reg <= ram_awaddr;
                ram_aw_latched <= 1;
            end
            // 2. Latch Data
            if (ram_wvalid && ram_wready) begin
                ram_wdata_reg <= ram_wdata;
                ram_wstrb_reg <= ram_wstrb;
                ram_w_latched <= 1;
            end
            // 3. Execute Write & Send Response
            if (ram_aw_latched && ram_w_latched && !ram_bvalid_reg) begin
                if (ram_wstrb_reg[0]) memory[ram_awaddr_reg[12:2]][7:0]   <= ram_wdata_reg[7:0];
                if (ram_wstrb_reg[1]) memory[ram_awaddr_reg[12:2]][15:8]  <= ram_wdata_reg[15:8];
                if (ram_wstrb_reg[2]) memory[ram_awaddr_reg[12:2]][23:16] <= ram_wdata_reg[23:16];
                if (ram_wstrb_reg[3]) memory[ram_awaddr_reg[12:2]][31:24] <= ram_wdata_reg[31:24];
                ram_bvalid_reg <= 1;
            end else if (ram_bvalid_reg && ram_bready) begin
                // 4. Clear everything when Master accepts response
                ram_bvalid_reg <= 0;
                ram_aw_latched <= 0;
                ram_w_latched  <= 0;
            end
        end
    end

    // RAM Read Channel
    reg ram_rvalid_reg;
    assign ram_arready = !ram_rvalid_reg || ram_rready;
    assign ram_rvalid  = ram_rvalid_reg;
    assign ram_rresp   = 2'b00;

    always @(posedge clk) begin
        if (!resetn) ram_rvalid_reg <= 1'b0;
        else begin
            if (ram_arvalid && ram_arready) begin
                ram_rdata <= memory[ram_araddr[12:2]];
                ram_rvalid_reg <= 1'b1;                
            end else if (ram_rready) begin
                ram_rvalid_reg <= 1'b0;                
            end
        end
    end 

    // ------------------------------------------
    // 2. AXI-Lite LED Register
    // ------------------------------------------
    reg led_aw_latched, led_w_latched, led_bvalid_reg;
    reg [31:0] led_wdata_reg;
    reg [3:0]  led_wstrb_reg;

    assign led_awready = !led_aw_latched && !led_bvalid_reg;
    assign led_wready  = !led_w_latched  && !led_bvalid_reg;
    assign led_bvalid  = led_bvalid_reg;
    assign led_bresp   = 2'b00;

    always @(posedge clk) begin
        if (!resetn) begin
            led_reg <= 8'b0; led_aw_latched <= 0; led_w_latched <= 0; led_bvalid_reg <= 0;
        end else begin
            // Latch Write Channels
            if (led_awvalid && led_awready) led_aw_latched <= 1;
            if (led_wvalid && led_wready) begin
                led_wdata_reg <= led_wdata;
                led_wstrb_reg <= led_wstrb;
                led_w_latched <= 1;
            end
            
            // Execute Write
            if (led_aw_latched && led_w_latched && !led_bvalid_reg) begin
                if (led_wstrb_reg != 0) led_reg <= led_wdata_reg[7:0];
                led_bvalid_reg <= 1;
            end else if (led_bvalid_reg && led_bready) begin
                led_bvalid_reg <= 0; led_aw_latched <= 0; led_w_latched <= 0;
            end
        end
    end

    // LED Read Channel (Fixed Compliance)
    reg led_rvalid_reg;
    assign led_arready = !led_rvalid_reg || led_rready;
    assign led_rvalid  = led_rvalid_reg;
    assign led_rresp   = 2'b00;

    always @(posedge clk) begin
        if (!resetn) led_rvalid_reg <= 0;
        else begin
            if (led_arvalid && led_arready) begin
                led_rdata <= {24'b0, led_reg};
                led_rvalid_reg <= 1;
            end else if (led_rready) begin
                led_rvalid_reg <= 0;
            end
        end
    end

    // ------------------------------------------
    // 3. AXI-Lite UART Wrapper
    // ------------------------------------------
    reg uart_aw_latched, uart_w_latched, uart_bvalid_reg;
    reg [31:0] uart_awaddr_reg, uart_wdata_reg;
    

    assign uart_awready = !uart_aw_latched && !uart_bvalid_reg;
    assign uart_wready  = !uart_w_latched  && !uart_bvalid_reg;
    assign uart_bvalid  = uart_bvalid_reg;
    assign uart_bresp   = 2'b00;

    always @(posedge clk) begin
        if (!resetn) begin
            uart_aw_latched <= 0; uart_w_latched <= 0; uart_bvalid_reg <= 0;
            uart_tx_tvalid <= 0;
        end else begin
            uart_tx_tvalid <= 0; // Default off
            
            // Latch Write Channels
            if (uart_awvalid && uart_awready) begin
                uart_awaddr_reg <= uart_awaddr;
                uart_aw_latched <= 1;
            end
            if (uart_wvalid && uart_wready) begin
                uart_wdata_reg <= uart_wdata;
                uart_w_latched <= 1;
            end
            
            // Execute Write
            if (uart_aw_latched && uart_w_latched && !uart_bvalid_reg) begin
                if (uart_awaddr_reg[3:0] == 4'h0) begin 
                    // Writing to TX Data: ONLY finish if UART is ready!
                    if (uart_tx_tready) begin
                        tx_data_reg <= uart_wdata_reg[7:0]; 
                        uart_tx_tvalid <= 1;                
                        uart_bvalid_reg <= 1; // Send AXI OKAY
                    end
                    // If NOT ready, we do nothing! The bus stalls and waits.
                end else begin
                    // If writing to a read-only status register, just ACK and discard
                    uart_bvalid_reg <= 1;
                end
            end else if (uart_bvalid_reg && uart_bready) begin
                uart_bvalid_reg <= 0; uart_aw_latched <= 0; uart_w_latched <= 0;
            end
        end
    end

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
    // 4. AXI-Lite SPI Wrapper (Address 0x3000_0000)
    // ------------------------------------------
    axi_spi_ctrl #(
        .C_S_AXIL_DATA_WIDTH(32),
        .C_S_AXIL_ADDR_WIDTH(32),
        .CLKS_PER_HALF_BIT(50) // 100MHz / (2 * 1MHz SPI Clock) = 50        
    ) spi_subsystem (
        .clk(clk),
        .resetn(resetn),

        // Connect to the Crossbar Wires
        .s_axil_awaddr (spi_awaddr), 
        .s_axil_awprot (spi_awprot), 
        .s_axil_awvalid(spi_awvalid), 
        .s_axil_awready(spi_awready),
        
        .s_axil_wdata  (spi_wdata), 
        .s_axil_wstrb  (spi_wstrb), 
        .s_axil_wvalid (spi_wvalid), 
        .s_axil_wready (spi_wready),
        
        .s_axil_bresp  (spi_bresp), 
        .s_axil_bvalid (spi_bvalid), 
        .s_axil_bready (spi_bready),
        
        .s_axil_araddr (spi_araddr), 
        .s_axil_arprot (spi_arprot), 
        .s_axil_arvalid(spi_arvalid), 
        .s_axil_arready(spi_arready),
        
        .s_axil_rdata  (spi_rdata), 
        .s_axil_rresp  (spi_rresp), 
        .s_axil_rvalid (spi_rvalid), 
        .s_axil_rready (spi_rready),

        // Physical Output Wires
        .o_SPI_Clk  (spi_clk),
        .i_SPI_MISO (spi_miso),
        .o_SPI_MOSI (spi_mosi),
        .o_SPI_CS_n (spi_cs_n)
    );

    
endmodule


