#!/usr/bin/env python3
"""
Serial monitor for Daisy Seed
Usage: python3 monitor_serial.py [port]
If no port specified, will try to auto-detect
"""

import sys
import serial
import serial.tools.list_ports
import time

BAUD_RATE = 115200

def list_available_ports():
    """List all available serial ports"""
    ports = serial.tools.list_ports.comports()
    print("\nAvailable serial ports:")
    print("-" * 60)
    for port in ports:
        print(f"  {port.device:20} - {port.description}")
    print("-" * 60)
    return [p.device for p in ports]

def find_daisy_port():
    """Try to auto-detect Daisy Seed port"""
    ports = serial.tools.list_ports.comports()
    
    # Common Daisy Seed identifiers
    daisy_keywords = ['daisy', 'seed', 'stm', 'usbmodem', 'usbserial', 'SLAB']
    
    for port in ports:
        desc_lower = port.description.lower()
        device_lower = port.device.lower()
        
        for keyword in daisy_keywords:
            if keyword in desc_lower or keyword in device_lower:
                return port.device
    
    return None

def monitor_serial(port_name):
    """Monitor serial output from specified port"""
    try:
        ser = serial.Serial(
            port=port_name,
            baudrate=BAUD_RATE,
            timeout=1,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE
        )
        
        print("=" * 60)
        print("Daisy Seed Serial Monitor")
        print("=" * 60)
        print(f"Port: {port_name}")
        print(f"Baud Rate: {BAUD_RATE}")
        print("Press Ctrl+C to exit")
        print("=" * 60)
        print()
        
        # Clear any existing data
        ser.reset_input_buffer()
        
        # Monitor loop
        while True:
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        print(line)
                except UnicodeDecodeError:
                    # Skip invalid characters
                    pass
            
            time.sleep(0.01)  # Small delay to prevent CPU spinning
            
    except serial.SerialException as e:
        print(f"Error opening serial port: {e}")
        print("\nMake sure:")
        print("  1. Daisy Seed is connected via USB")
        print("  2. Daisy Seed is NOT in bootloader/DFU mode")
        print("  3. No other program is using the serial port")
        return 1
    except KeyboardInterrupt:
        print("\n\nMonitoring stopped.")
        if ser.is_open:
            ser.close()
        return 0
    except Exception as e:
        print(f"Unexpected error: {e}")
        return 1

def main():
    if len(sys.argv) > 1:
        port = sys.argv[1]
    else:
        print("Searching for Daisy Seed...")
        port = find_daisy_port()
        
        if not port:
            print("Could not auto-detect Daisy Seed port.")
            available = list_available_ports()
            if not available:
                print("\nNo serial ports found!")
                print("\nMake sure:")
                print("  1. Daisy Seed is connected via USB")
                print("  2. Daisy Seed is NOT in bootloader/DFU mode")
                print("  3. Daisy Seed firmware is running")
                return 1
            else:
                print("\nPlease specify the port manually:")
                print(f"  python3 {sys.argv[0]} <port_name>")
                print("\nExample:")
                print(f"  python3 {sys.argv[0]} {available[0]}")
                return 1
        else:
            print(f"Found port: {port}")
    
    return monitor_serial(port)

if __name__ == "__main__":
    sys.exit(main())
