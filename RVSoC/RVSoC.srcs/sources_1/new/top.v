`timescale 1ns / 1ps

module top (
    input  clk,           // 100MHz clock from Nexys 4 DDR
    input  resetn,        // Active-low reset (CPU_RESET button)
    output [7:0] led,     // 8 LEDs on the board
    output uart_tx,       // FPGA TX Pin
    input  uart_rx        // FPGA RX Pin
);

    // --- PicoRV32 Native Interface Signals ---
    wire        mem_valid;
    wire        mem_instr;
    reg         mem_ready;
    wire [31:0] mem_addr;
    wire [31:0] mem_wdata;
    wire [3:0]  mem_wstrb;
    reg  [31:0] mem_rdata;

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

endmodule