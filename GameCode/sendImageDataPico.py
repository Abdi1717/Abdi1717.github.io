import subprocess
import serial
import re
import json
import time
import signal

def setup_serial_connection(port, baudrate, timeout):
    """Establishes the serial connection."""
    try:
        return serial.Serial(port, baudrate, timeout=timeout)
    except serial.SerialException as e:
        print(f"Error opening serial port: {e}")
        return None

def send_command(ser, command):
    """Send command to the microcontroller with error handling."""
    if ser:
        try:
            ser.write((command + '\r\n').encode())
            print(f"Sent: {command}")
        except serial.SerialException as e:
            print(f"Error sending command: {e}")
            # Try to re-establish connection
            print("Attempting to re-establish serial connection...")
            ser.close()
            time.sleep(1)
            ser.open()

def handle_exit(sig, frame):
    print("Exiting gracefully...")
    if process:
        process.terminate()
    if ser:
        ser.close()
    exit(0)

def main():
    global ser, process
    ser = setup_serial_connection('/dev/tty.usbmodem14201', 115200, 1)
    if not ser:
        return

    # Setup signal handler for graceful shutdown
    signal.signal(signal.SIGINT, handle_exit)

    try:
        process = subprocess.Popen(
            ['edge-impulse-linux-runner'],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True
        )

        pattern = re.compile(r'\{.*?\}')  # Regular expression to find JSON strings

        for line in iter(process.stdout.readline, ''):
            print(line, end='')  # Optional: Comment out to stop printing each line
            matches = pattern.findall(line)
            for match in matches:
                try:
                    data = json.loads(match)
                    if 'label' in data and 'value' in data and data['value'] > 0.2:
                        send_command(ser, data['label'])
                except json.JSONDecodeError as e:
                    print(f"Error decoding JSON from line: {line} - {e}")

    except subprocess.SubprocessError as e:
        print(f"Error with subprocess: {e}")
    finally:
        if ser:
            ser.close()
        if process:
            process.terminate()

if __name__ == "__main__":
    ser = None
    process = None
    main()

