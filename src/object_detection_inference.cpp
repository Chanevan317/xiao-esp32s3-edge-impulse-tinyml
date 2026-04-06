#include <Arduino.h>
#include <object_detection_xiaoesp32s3_inferencing.h>
#include <eloquent_esp32cam.h>
#include <eloquent_esp32cam/edgeimpulse/fomo.h>

using eloq::camera;
using eloq::ei::fomo;

void setup() {
    Serial.begin(115200);
    delay(3000);
    Serial.println("___ XIAO S3 FOMO: Component Detection ___");

    camera.pinout.xiao();
    camera.resolution.face();  // 240x240
    camera.quality.high();

    while (!camera.begin().isOk()) {
        Serial.println(camera.exception.toString());
        delay(1000);
    }

    Serial.println("Camera ready. Starting inference...");
}

void loop() {
    if (!camera.capture().isOk()) {
        Serial.println(camera.exception.toString());
        return;
    }

    if (!fomo.run().isOk()) {
        Serial.println(fomo.exception.toString());
        return;
    }

    Serial.printf("Found %d objects\n", fomo.count());

    fomo.forEach([](int i, eloq::ei::bbox_t bbox) {
        Serial.printf("  [%d] %s (%.2f) at [x:%d y:%d w:%d h:%d]\n",
            i,
            bbox.label,
            bbox.proba,
            bbox.x, bbox.y,
            bbox.width, bbox.height
        );
    });

    if (fomo.count() == 0)
        Serial.println("No objects detected.");

    delay(100);
}