import cv2
import serial
import struct
import numpy as np
import os
import argparse

# Configuration
SERIAL_PORT = '/dev/ttyACM0' # Change to your port (e.g., /dev/ttyACM0 on Fedora)
BAUD_RATE = 2000000
BASE_DATASET_PATH = "dataset"

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--mode", choices=['record', 'detect'], required=True)
    args = parser.parse_args()

    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    print(f"Connected to {SERIAL_PORT}")

    try:
        while True:
            if args.mode == 'record':
                label = input("\nEnter label name (or 'q' to quit): ").strip()
                if label.lower() == 'q': break
                
                # Create directory for this specific label
                label_path = os.path.join(BASE_DATASET_PATH, label)
                if not os.path.exists(label_path):
                    os.makedirs(label_path)

                print(f"Ready to take 10 samples for '{label}'. Press 's' to capture.")
                
                samples_taken = 0
                while samples_taken < 10:
                    # Sync with Start Marker from XIAO
                    if ser.read(5) == b'START':
                        size_data = ser.read(4)
                        if len(size_data) < 4: continue
                        img_size = struct.unpack('<I', size_data)[0]
                        img_bytes = ser.read(img_size)
                        
                        nparr = np.frombuffer(img_bytes, np.uint8)
                        img = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
                        if img is None: continue

                        # Display preview
                        display_img = img.copy()
                        cv2.putText(display_img, f"Label: {label} | Samples: {samples_taken}/10", 
                                    (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
                        cv2.imshow("Data Collection", display_img)

                        key = cv2.waitKey(1) & 0xFF
                        if key == ord('s'):
                            img_filename = os.path.join(label_path, f"{label}_{samples_taken}.jpg")
                            cv2.imwrite(img_filename, img)
                            samples_taken += 1
                            print(f"Captured {samples_taken}/10: {img_filename}")
                        elif key == ord('q'):
                            print("Aborting current label...")
                            break
                
                print(f"Finished 10 samples for {label}!")

            elif args.mode == 'detect':
                # (Detection logic remains same as before)
                if ser.read(5) == b'START':
                    # ... (Size and Image reading logic) ...
                    cv2.imshow("Detection Mode", img)
                    if cv2.waitKey(1) & 0xFF == ord('q'): break

    except KeyboardInterrupt:
        print("\nExiting...")
    finally:
        ser.close()
        cv2.destroyAllWindows()

if __name__ == "__main__":
    main()