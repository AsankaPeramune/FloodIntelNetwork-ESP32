#include <Arduino.h>
#include <Adafruit_TinyUSB.h>

uint8_t frame[4];

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("A01NYUB Test on RAK4631");

  // Sensor TX -> RAK RX pin
  // Sensor RX usually unused

  Serial1.setPins(15, 16);   // RX, TX
  Serial1.begin(9600);

  Serial.println("Listening on Serial1...");
}

void loop() {
  while (Serial1.available() >= 4) {
    if (Serial1.read() == 0xFF) {
      frame[0] = 0xFF;
      frame[1] = Serial1.read();
      frame[2] = Serial1.read();
      frame[3] = Serial1.read();

      uint8_t checksum = (frame[0] + frame[1] + frame[2]) & 0xFF;

      if (checksum == frame[3]) {
        int distance_mm = (frame[1] << 8) | frame[2];
        Serial.print("Distance: ");
        Serial.print(distance_mm);
        Serial.println(" mm");
      } else {
        Serial.println("Checksum error");
      }
    }
  }
}