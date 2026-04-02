`timescale 1ns / 1ps

module axi_spi_ctrl #(
    parameter integer C_S_AXIL_DATA_WIDTH = 32,
    parameter integer C_S_AXIL_ADDR_WIDTH = 32,
    parameter integer CLKS_PER_HALF_BIT = 50
)(
    input wire  clk,
    input wire  resetn,

    // AXI-Lite Slave Interface
    input wire [C_S_AXIL_ADDR_WIDTH-1 : 0] s_axil_awaddr,
    input wire [2:0] s_axil_awprot,
    input wire  s_axil_awvalid,
    output wire s_axil_awready,
    input wire [C_S_AXIL_DATA_WIDTH-1 : 0] s_axil_wdata,
    input wire [(C_S_AXIL_DATA_WIDTH/8)-1 : 0] s_axil_wstrb,
    input wire  s_axil_wvalid,
    output wire s_axil_wready,
    output wire [1:0] s_axil_bresp,
    output wire s_axil_bvalid,
    input wire  s_axil_bready,
    input wire [C_S_AXIL_ADDR_WIDTH-1 : 0] s_axil_araddr,
    input wire [2:0] s_axil_arprot,
    input wire  s_axil_arvalid,
    output wire s_axil_arready,
    output reg [C_S_AXIL_DATA_WIDTH-1 : 0] s_axil_rdata,
    output wire [1:0] s_axil_rresp,
    output wire s_axil_rvalid,
    input wire  s_axil_rready,

    // SPI Physical Wires
    output wire o_SPI_Clk,
    input  wire i_SPI_MISO,
    output wire o_SPI_MOSI,
    output wire o_SPI_CS_n
);

    // --- Internal Signals ---
    wire       spi_tx_ready;
    wire       spi_rx_dv;
    wire [7:0] spi_rx_byte;
    
    reg        tx_dv_reg;
    reg  [7:0] tx_data_reg;
    reg  [7:0] rx_data_hold;
    reg        rx_unread_flag;
    reg        spi_tx_ready_reg;

    // --- Manual CS Register ---
    reg spi_cs_reg;
    assign o_SPI_CS_n = spi_cs_reg;

    // --- Instantiate the RAW Nandland SPI Master ---
    SPI_Master #(
        .SPI_MODE(0),
        .CLKS_PER_HALF_BIT(CLKS_PER_HALF_BIT)
    ) spi_inst (
        .i_Rst_L(resetn),
        .i_Clk(clk),
        .i_TX_Byte(tx_data_reg),
        .i_TX_DV(tx_dv_reg),
        .o_TX_Ready(spi_tx_ready),
        .o_RX_DV(spi_rx_dv),
        .o_RX_Byte(spi_rx_byte),
        .o_SPI_Clk(o_SPI_Clk),
        .i_SPI_MISO(i_SPI_MISO),
        .o_SPI_MOSI(o_SPI_MOSI)
    );

    // Buffer the combinatorial ready signal
    always @(posedge clk) begin
        if (!resetn) spi_tx_ready_reg <= 1'b0;
        else         spi_tx_ready_reg <= spi_tx_ready;
    end

    // ==========================================
    // BULLETPROOF AXI-LITE WRITE CHANNEL
    // ==========================================
    reg aw_latched, w_latched, bvalid_reg;
    reg [C_S_AXIL_ADDR_WIDTH-1:0] awaddr_reg;
    reg [C_S_AXIL_DATA_WIDTH-1:0] wdata_reg;
    reg tx_issued; 

    assign s_axil_awready = !aw_latched && !bvalid_reg;
    assign s_axil_wready  = !w_latched  && !bvalid_reg;
    assign s_axil_bvalid  = bvalid_reg;
    assign s_axil_bresp   = 2'b00; 

    always @(posedge clk) begin
        if (!resetn) begin
            aw_latched <= 0; w_latched <= 0; bvalid_reg <= 0;
            tx_dv_reg <= 0; tx_data_reg <= 8'b0; tx_issued <= 0;
            spi_cs_reg <= 1'b1; // Default CS HIGH (Inactive)
        end else begin
            tx_dv_reg <= 0; 
            
            if (s_axil_awvalid && s_axil_awready) begin
                awaddr_reg <= s_axil_awaddr; aw_latched <= 1;
            end
            if (s_axil_wvalid && s_axil_wready) begin
                wdata_reg <= s_axil_wdata; w_latched <= 1;
            end
            
            if (aw_latched && w_latched && !bvalid_reg) begin
                if (awaddr_reg[3:0] == 4'h0) begin 
                    if (spi_tx_ready_reg && !tx_issued) begin
                        tx_data_reg <= wdata_reg[7:0]; 
                        tx_dv_reg <= 1'b1; tx_issued <= 1'b1;    
                    end else if (tx_issued) begin
                        bvalid_reg <= 1; tx_issued <= 1'b0;
                    end
                end else if (awaddr_reg[3:0] == 4'h4) begin
                    // Write to 0x4: Bit 1 controls the CS pin!
                    spi_cs_reg <= wdata_reg[1];
                    bvalid_reg <= 1;
                end else begin
                    bvalid_reg <= 1;
                end
            end else if (bvalid_reg && s_axil_bready) begin
                bvalid_reg <= 0; aw_latched <= 0; w_latched <= 0;
            end
        end
    end

    // ==========================================
    // BULLETPROOF AXI-LITE READ CHANNEL
    // ==========================================
    reg rvalid_reg;
    reg [3:0] araddr_reg; 

    assign s_axil_arready = !rvalid_reg || s_axil_rready;
    assign s_axil_rvalid  = rvalid_reg;
    assign s_axil_rresp   = 2'b00;

    always @(posedge clk) begin
        if (!resetn) begin
            rvalid_reg <= 1'b0; s_axil_rdata <= 32'b0; araddr_reg <= 4'b0;
        end else begin
            if (s_axil_arvalid && s_axil_arready) begin
                rvalid_reg <= 1'b1; araddr_reg <= s_axil_araddr[3:0]; 
                
                if (s_axil_araddr[3:0] == 4'h0)
                    s_axil_rdata <= {24'b0, rx_data_hold};
                else if (s_axil_araddr[3:0] == 4'h4)
                    // Return Status: [Bit 2: spi_cs_reg, Bit 1: rx_unread_flag, Bit 0: spi_tx_ready_reg]
                    s_axil_rdata <= {29'b0, spi_cs_reg, rx_unread_flag, spi_tx_ready_reg}; 
                else
                    s_axil_rdata <= 32'h0;
                    
            end else if (s_axil_rready && rvalid_reg) begin
                rvalid_reg <= 1'b0;
            end
        end
    end

    // --- Safely Catch and Clear RX Data ---
    always @(posedge clk) begin
        if (!resetn) begin
            rx_data_hold <= 8'b0;
            rx_unread_flag <= 1'b0;
        end else begin
            if (spi_rx_dv) begin
                rx_data_hold <= spi_rx_byte;
                rx_unread_flag <= 1'b1;
            end else if (s_axil_rvalid && s_axil_rready && araddr_reg == 4'h0) begin
                rx_unread_flag <= 1'b0;
            end
        end
    end
    
endmodule