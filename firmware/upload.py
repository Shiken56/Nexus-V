import serial
import sys
import time

def upload_firmware(com_port, bin_file):
    try:
        with open(bin_file, 'rb') as f:
            firmware_data = f.read()
        
        file_size = len(firmware_data)
        print(f"[*] Loaded {bin_file} ({file_size} bytes)")

        print(f"[*] Opening {com_port} at 115200 baud...")
        ser = serial.Serial(com_port, 115200, timeout=5)
        time.sleep(1)

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

        ser.close()

    except Exception as e:
        print(f"[-] FAILED: {e}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python upload.py <COM_PORT> <FIRMWARE.BIN>")
        sys.exit(1)
    
    upload_firmware(sys.argv[1], sys.argv[2])
