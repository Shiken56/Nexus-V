import serial
import sys
import time

def upload_firmware(com_port, bin_file):
    ser = None  # Safely initialize this so the 'finally' block doesn't crash
    try:
        with open(bin_file, 'rb') as f:
            firmware_data = f.read()
        
        file_size = len(firmware_data)
        print(f"[*] Loaded {bin_file} ({file_size} bytes)")

        print(f"[*] Opening {com_port} at 115200 baud...")
        ser = serial.Serial(com_port, 115200, timeout=5)
        
        #Software Reset Sequence
        print("[*] Sending Soft-Reset command (0x7F)...")
        ser.write(b'\x7F') 
        time.sleep(0.5) # Give the FPGA 500ms to jump to 0x00000000 and initialize
        
        # Flush the buffer just in case the app sent any garbage while exiting
        ser.reset_input_buffer() 

        print("[*] Sending Upload Command ('U')...")
        ser.write(b'U')

        size_bytes = file_size.to_bytes(4, byteorder='little')
        ser.write(size_bytes)
        
        print("[*] Uploading firmware payload...")
        ser.write(firmware_data)

        print("[*] Waiting for FPGA acknowledgment...")
        ack = ser.read(1)
        
        if ack == b'K':
            print("[+] SUCCESS! Code is running!")
        else:
            print(f"[-] ERROR: Expected 'K', got: {ack}")

    except Exception as e:
        print(f"[-] FAILED: {e}")
        if "Access is denied" in str(e):
            print("    -> Make sure PuTTY or Vivado isn't holding COM6 hostage!")

    finally:
        # This guarantees the port is released, even if an error happened
        if ser is not None and ser.is_open:
            ser.close()

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python upload.py <COM_PORT> <FIRMWARE.BIN>")
        sys.exit(1)
    
    upload_firmware(sys.argv[1], sys.argv[2])
