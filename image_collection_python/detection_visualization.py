import cv2
import serial
import struct
import numpy as np

# Configuration
SERIAL_PORT = '/dev/ttyACM0' 
BAUD_RATE = 2000000
START_MARKER = b'START'

def get_frame(ser):
    ser.read_until(START_MARKER)
    size_data = ser.read(4)
    if len(size_data) < 4: return None
    img_size = struct.unpack('<I', size_data)[0]
    img_bytes = ser.read(img_size)
    if len(img_bytes) < img_size: return None
    nparr = np.frombuffer(img_bytes, np.uint8)
    return cv2.imdecode(nparr, cv2.IMREAD_COLOR)

def main():
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    ser.reset_input_buffer()
    print("--- DETECTION MODE ---")

    try:
        while True:
            img = get_frame(ser)
            if img is None: continue

            # Zoomed in view for easier monitoring
            display_img = cv2.resize(img, (480, 480), interpolation=cv2.INTER_NEAREST)
            
            cv2.imshow("Confidence Bot Detection", display_img)
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break
    except KeyboardInterrupt:
        pass
    finally:
        ser.close()
        cv2.destroyAllWindows()

if __name__ == "__main__":
    main()