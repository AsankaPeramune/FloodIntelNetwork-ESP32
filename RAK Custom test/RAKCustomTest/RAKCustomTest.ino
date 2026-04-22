#include <Arduino.h>
#include <Adafruit_TinyUSB.h>

void setup() {
  pinMode(LED_BLUE, OUTPUT);
}

void loop() {
  digitalWrite(LED_BLUE, HIGH);
  delay(500);
  digitalWrite(LED_BLUE, LOW);
  delay(500);
}
