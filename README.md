# Nexus-V: RISC-V SoC Implementation

**Nexus-V** is an open-source System-on-Chip (SoC) designed for the Xilinx Artix-7 FPGA (Nexys 4 DDR). It integrates the PicoRV32_axi RISC-V core with custom memory-mapped peripherals to provide a lightweight, modular environment for embedded applications.

---



## System Architecture

* **Processor Core:** PicoRV32 (RV32I Base Integer Instruction Set, AXI4-Lite Master)
* **System Clock:** 100 MHz (Unified clock domain for CPU and AXI Bus)
* **System Bus:** AMBA AXI4-Lite Crossbar (1 Master, 5 Slaves)
* **Main Memory:** 32 KB Block RAM (Dual-purpose Instruction/Data, RWX permissions)
* **Video Memory:** ~75 KB (19,200 words) 32-bit Dual-Port VRAM mapped to a 100 MHz VGA Sync Driver

---

## AXI4-Lite Memory Map

The SoC utilizes a strict memory-mapped I/O approach. The AXI crossbar routes CPU read/write transactions to the following physical base addresses. 

| Peripheral | Base Address | Size | Access | Description |
| :--- | :--- | :--- | :--- | :--- |
| **Main RAM** | `0x0000_0000` | 64 KB | R/W | 32 KB physical BRAM instance. Contains Bootloader & Application logic. |
| **LED GPIO** | `0x1000_0000` | 64 KB | W | 8-bit output register mapped to onboard Artix-7 LEDs. |
| **UART Core** | `0x2000_0000` | 64 KB | R/W | 115200 Baud serial controller for PC communication & firmware flashing. |
| **SPI Master** | `0x3000_0000` | 64 KB | R/W | Communicates with the onboard ADXL362 accelerometer & external sensors. |
| **VGA VRAM** | `0x4000_0000` | 128 KB | W | 32-bit wide framebuffer for the custom external display driver. |
| **I2C Master** | `0x5000_0000` | 64 KB | R/W | Custom AXI-wrapped I2C controller for multi-device sensor polling. |

## Bootloader & Stack Memory Map
The 32 KB BRAM is partitioned to safely house both the hardware bootloader and the dynamic user applications. The stack pointer (SP) is initialized at the top of memory and grows downwards. When the bootloader jumps to the application offset, the application's startup assembly file resets the SP, providing a clean execution environment.

| Address | Section | Size | Description |
| :--- | :--- | :--- | :--- |
| `0x0000_7FFC` | **Stack Top** | - | Initial Stack Pointer (grows downwards). |
| `0x0000_1000` | **App Base** | 28 KB | `app.c` logic dynamically flashed via UART. |
| `0x0000_0000` | **Boot Base** | 4 KB | `boot.c` permanently baked into BRAM during synthesis. |

---


## Repository Structure

```text
Nexus-V/
├── RVSoC/                  # Vivado Hardware Project Workspace
│   ├── RVSoC.srcs/         # Golden Verilog/SystemVerilog RTL
│   │   ├── sources_1/new/  # Top module, PicoRV32, and custom AXI wrappers
│   │   └── sim_1/new/      # Testbenches (e.g., tb_bootloader.v)
│   └── build_project.tcl   # Hardware DevOps script for regenerating the project
│
├── firmware/               # C Code, Assembly, and Toolchain
│   ├── boot.c / boot.ld    # Stage 1: Hardware bootloader baked into BRAM
│   ├── app.c / app.ld      # Stage 2: User application logic
│   ├── Makefile            # GCC compilation targets (.elf, .hex, .bin)
│   └── upload.py           # Python script to stream firmware over UART
│
└── README.md               # Project Documentation
```

---



## Firmware Development (Two-Stage Boot)

Software development for Nexus-V does not require Vivado. The system uses a dual-firmware approach separated by custom Linker scripts.

1. **Write Code:** Modify your application logic in `firmware/app.c`.
2. **Compile:** Use the provided Makefile to compile the C code into a flat binary.
   ```bash
   cd firmware
   mingw32-make clean
   mingw32-make
   ```
3. **Flash to FPGA:** Ensure the FPGA is powered on and programmed with the base bitstream. Use the Python script to stream the new `firmware.bin` directly into the CPU's RAM over the serial connection.
   ```bash
   python upload.py COM3 firmware.bin
   ```
   *(Ensure `COM3` is replaced with the active serial port identified by your operating system).*

---

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

---

## Hardware Implementation & Resource Utilization
The Nexus-V architecture was successfully synthesized, implemented, and routed on the Nexys 4 DDR. 

### Device Utilization
| Resource | Utilization | Available | Percentage |
| :--- | :--- | :--- | :--- |
| **LUTs** | 1,969 | 63,400 | 3.1% |
| **Flip-Flops** | 1,912 | 126,800 | 1.5% |
| **BRAM Tiles** | 40 | 135 | 29.6% |


### RTL Netlist
Below is the synthesized RTL schematic highlighting the PicoRV32 core, the AXI-Lite Crossbar, and the peripheral routing:

![RTL Netlist Schematic](assets/netlist.png)

## System Showcase

**1. Dynamic Firmware Flashing over UART:**
![Firmware Upload Process](assets/upload_demo.gif)

**2. VGA Display & Sensor Integration:**
![VGA and Board Output](assets/board_working.jpg)
