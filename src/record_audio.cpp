#include <Arduino.h>
#include <I2S.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// Settings
#define RECORD_TIME      10     // Seconds per recording
#define SAMPLE_RATE      16000U
#define SAMPLE_BITS      16
#define WAV_HEADER_SIZE  44
#define VOLUME_GAIN      2      // Boost the digital mic signal
#define SD_CS_PIN        21

// Variables for Serial control
String baseFileName = "";
int fileNumber = 1;

// Function Prototypes
void record_wav(String fileName);
void generate_wav_header(uint8_t *wav_header, uint32_t wav_size, uint32_t sample_rate);

void setup() {
    Serial.begin(115200);
    while (!Serial);
    delay(1000);

    Serial.println("\n--- XIAO S3 Recorder (PlatformIO 2.0.11) ---");

    // Initialize I2S (Pins: SD, SCK, WS, MCLK, DIN)
    // For XIAO S3 Sense Mic: SCK=42, DIN=41. Others -1.
    I2S.setAllPins(-1, 42, 41, -1, -1);
    if (!I2S.begin(PDM_MONO_MODE, SAMPLE_RATE, SAMPLE_BITS)) {
        Serial.println("Failed to initialize I2S!");
        while (1);
    }

    // Initialize SD
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("Failed to mount SD Card!");
        while (1);
    }

    Serial.println("System Ready.");
    Serial.println("1. Type a label and press Enter.");
    Serial.println("2. Type 'rec' to start recording.");
}

void loop() {
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        input.trim();

        if (input == "rec") {
        if (baseFileName == "") {
            Serial.println("Error: Set a label first!");
        } else {
            String fullPath = "/" + baseFileName + "." + String(fileNumber++) + ".wav";
            record_wav(fullPath);
        }
        } else if (input != "") {
        baseFileName = input;
        fileNumber = 1;
        Serial.printf("Label set to: %s. Ready to record.\n", baseFileName.c_str());
        }
    }
}

void record_wav(String fileName) {
    uint32_t record_size = (SAMPLE_RATE * SAMPLE_BITS / 8) * RECORD_TIME;
    
    Serial.printf("Opening file: %s\n", fileName.c_str());
    File file = SD.open(fileName, FILE_WRITE);
    if (!file) {
        Serial.println("File Open Failed!");
        return;
    }

    // Write placeholder header
    uint8_t wav_header[WAV_HEADER_SIZE];
    generate_wav_header(wav_header, record_size, SAMPLE_RATE);
    file.write(wav_header, WAV_HEADER_SIZE);

    // Allocate PSRAM buffer
    uint8_t *rec_buffer = (uint8_t *)ps_malloc(record_size);
    if (rec_buffer == NULL) {
        Serial.println("PSRAM Malloc Failed!");
        file.close();
        return;
    }

    Serial.println("Recording Started...");
    
    // TIMER LOGIC: Start a background task or simple loop check? 
    // Since i2s_read is blocking, we print the start time and 
    // we use a simple non-blocking read loop to show the timer.
    
    uint32_t bytes_read_total = 0;
    uint32_t start_ms = millis();
    uint32_t last_timer_update = 0;

    while (bytes_read_total < record_size) {
        size_t i2s_bytes_read = 0;
        // Read in chunks of 100ms to keep the timer responsive
        uint32_t chunk_size = (SAMPLE_RATE * SAMPLE_BITS / 8) * 0.1; 
        
        esp_i2s::i2s_read(esp_i2s::I2S_NUM_0, 
                        rec_buffer + bytes_read_total, 
                        chunk_size, 
                        &i2s_bytes_read, 
                        100);

        bytes_read_total += i2s_bytes_read;

        // Update Timer every second
        uint32_t elapsed = (millis() - start_ms) / 1000;
        if (elapsed != last_timer_update) {
        Serial.printf("Time remaining: %d s\n", RECORD_TIME - elapsed);
        last_timer_update = elapsed;
        }
        
        if (millis() - start_ms > (RECORD_TIME * 1000) + 500) break; // Safety timeout
    }

    // Apply Volume Gain
    for (uint32_t i = 0; i < bytes_read_total; i += 2) {
        (*(uint16_t *)(rec_buffer + i)) <<= VOLUME_GAIN;
    }

    Serial.println("Writing to SD card...");
    file.write(rec_buffer, bytes_read_total);
    
    free(rec_buffer);
    file.close();
    Serial.println("Recording Complete!\n");
    Serial.println("Enter 'rec' for next or type a new label.");
}

void generate_wav_header(uint8_t *wav_header, uint32_t wav_size, uint32_t sample_rate) {
    uint32_t file_size = wav_size + WAV_HEADER_SIZE - 8;
    uint32_t byte_rate = SAMPLE_RATE * SAMPLE_BITS / 8;
    const uint8_t set_wav_header[] = {
        'R', 'I', 'F', 'F',
        (uint8_t)file_size, (uint8_t)(file_size >> 8), (uint8_t)(file_size >> 16), (uint8_t)(file_size >> 24),
        'W', 'A', 'V', 'E',
        'f', 'm', 't', ' ',
        0x10, 0x00, 0x00, 0x00,
        0x01, 0x00,
        0x01, 0x00,
        (uint8_t)sample_rate, (uint8_t)(sample_rate >> 8), (uint8_t)(sample_rate >> 16), (uint8_t)(sample_rate >> 24),
        (uint8_t)byte_rate, (uint8_t)(byte_rate >> 8), (uint8_t)(byte_rate >> 16), (uint8_t)(byte_rate >> 24),
        0x02, 0x00,
        0x10, 0x00,
        'd', 'a', 't', 'a',
        (uint8_t)wav_size, (uint8_t)(wav_size >> 8), (uint8_t)(wav_size >> 16), (uint8_t)(wav_size >> 24),
    };
    memcpy(wav_header, set_wav_header, sizeof(set_wav_header));
}