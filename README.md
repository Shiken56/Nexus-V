# Nexus-V: RISC-V SoC Implementation

**Nexus-V** is an open-source System-on-Chip (SoC) designed for the Xilinx Artix-7 FPGA (Nexys 4 DDR). It integrates the PicoRV32 RISC-V CPU with custom memory-mapped peripherals to provide a lightweight, modular environment for embedded applications.

---

## Key Features ( In progress )

- **CPU Core:** PicoRV32 (RV32I ISA)  
- **Memory:** 4KB On-chip Block RAM (BRAM)  
- **UART:** High-speed AXI-Stream UART (115200 Baud)  
- **GPIO:** 8-bit memory-mapped LED controller  
- **Interconnect:** Simple Bus MMIO with request/acknowledge handshake  

---

## System Architecture

The SoC routes traffic between the CPU and peripherals based on a 32-bit address space.

### Memory Map

| Peripheral        | Base Address  | Range | Description                          |
|------------------|--------------|-------|--------------------------------------|
| Instruction RAM  | 0x0000_0000  | 4 KB  | Program code and stack space         |
| LED GPIO         | 0x1000_0000  | 4 B   | 8-bit output for onboard LEDs        |
| UART Data        | 0x2000_0000  | 4 B   | TX/RX data buffer                    |
| UART Status      | 0x2000_0004  | 4 B   | Bit 0: TX Ready, Bit 1: RX Valid     |


## Project Structure

```text
RISC-V-SoC-on-FPGA/
├── RVSoC/                  # Vivado Project
│   ├── sources/            # Verilog RTL (top.v, picorv32.v)
│   └── constraints/        # XDC files for Nexys 4 DDR
│
├── firmware/               # C Code & Toolchain
│   ├── main.c              # Application logic
│   └── firmware.hex        # Compiled binary
│
└── README.md               # Project Documentation
```



## Getting Started

### Prerequisites

- **Hardware:** Nexys 4 DDR (Artix-7 XC7A100T)  
- **Tools:** Vivado Design Suite (2023.1+)  
- **Toolchain:** `riscv32-unknown-elf-gcc`  

---

### Build Instructions

1. **Compile Firmware**  
   Navigate to the `firmware/` directory and run:
   ```bash
   make clean
   ```
   
   ```bash
   make
   ```

## Hardware Developer Workflow

This project uses a version-controlled Tcl workflow to keep the repository clean. **Do not push `.xpr`, `.cache`, or `.runs` folders to GitHub.**

### How to Build the Project
When you pull new code from GitHub, you must regenerate your local Vivado project:
1. Delete the inner `RVSoC/RVSoC` folder if it exists (this clears old caches).
2. Open Vivado.
3. Open the Tcl Console at the bottom of the welcome screen.
4. Run: `cd [Your-Path]/Nexus-V/RVSoC`
5. Run: `source build_project.tcl`

### How to Add New Hardware Files
Do not use Vivado's "Create File" button, as it will bury your code in ignored folders.
1. Create your new `.v` or `.sv` file directly inside the `RVSoC.srcs/` folder using VS Code.
2. In Vivado, click **Add Sources -> Add Files**.
3. **CRITICAL:** Uncheck the *"Copy sources into project"* box before clicking Finish.
4. Before you commit your code to Git, run `write_project_tcl -force build_project.tcl` in the Vivado console so the build script knows about your new file!


