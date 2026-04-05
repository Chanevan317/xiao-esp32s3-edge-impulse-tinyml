import cv2
import serial
import struct
import numpy as np
import os

# Configuration
SERIAL_PORT = '/dev/ttyACM0' 
BAUD_RATE = 2000000
BASE_DATASET_PATH = "dataset"
START_MARKER = b'START'

def get_frame(ser):
    # Search for the sync marker
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
    print("--- RECORD MODE ---")

    while True:
        label = input("\nEnter label name (or 'q' to quit): ").strip()
        if label.lower() == 'q': break
        
        label_path = os.path.join(BASE_DATASET_PATH, label)
        os.makedirs(label_path, exist_ok=True)

        print(f"Ready for '{label}'. Press 'S' to capture, 'C' to skip.")
        samples_taken = 0
        
        while samples_taken < 10:
            img = get_frame(ser)
            if img is None: continue

            display_img = img.copy()
            cv2.putText(display_img, f"Capture: {label}_{samples_taken}.jpg", 
                        (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
            cv2.imshow("Recorder", display_img)

            key = cv2.waitKey(1) & 0xFF
            if key == ord('s'):
                # Sequential naming: label_0.jpg, label_1.jpg...
                fname = os.path.join(label_path, f"{label}_{samples_taken}.jpg")
                cv2.imwrite(fname, img)
                print(f"Saved {samples_taken+1}/10: {fname}")
                samples_taken += 1
            elif key == ord('c'):
                break
        
    ser.close()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()