#include <Arduino.h>
#include <SX126x-RAK4630.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED Setup
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET    -1 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// LoRa Setup
#define RF_FREQUENCY            916000000
#define LORA_BANDWIDTH          0   // 125 kHz
#define LORA_SPREADING_FACTOR   7
#define LORA_CODINGRATE         1   // 4/5
#define LORA_PREAMBLE_LENGTH    8
#define TX_TIMEOUT_VALUE        3000

static RadioEvents_t RadioEvents;
bool txDone = true;
int currentPower = 14; // Default starting power

void OnTxDone(void) {
  Serial.println("TX Done");
  txDone = true;
}

void OnTxTimeout(void) {
  Serial.println("TX Timeout");
  txDone = true;
}

void setup() {
  Serial.begin(115200);
  
  // Initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("OLED failed"));
    for(;;); 
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Initialize LoRa
  lora_rak4630_init();
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  
  Serial.println("System Ready");
}

void loop() {
  Radio.IrqProcess();

  if (txDone) {
    // 1. Read Potentiometer
    int rawADC = analogRead(WB_A1);
    
    // 2. Map ADC (0-1023) to TX Power (-9 to 22 dBm)
    currentPower = map(rawADC, 0, 940, -9, 22);

    // 3. Update OLED Display
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0,0);
    display.println("Transmit Power");
    
    display.setCursor(0,15);
    display.setTextSize(2);
    display.print(currentPower);
    display.print(" dBm");

    display.setTextSize(1);
    display.setCursor(0,45);
    display.print("ADC: "); display.print(rawADC);
    display.display();

    // 4. Update Radio Config with NEW Power setting
    Radio.SetTxConfig(
      MODEM_LORA, currentPower, 0, LORA_BANDWIDTH,
      LORA_SPREADING_FACTOR, LORA_CODINGRATE,
      LORA_PREAMBLE_LENGTH, false, true, 0, 0, false, TX_TIMEOUT_VALUE
    );

    // 5. Send Message
    uint8_t msg[] = "HELLO";
    txDone = false;
    Serial.print("Sending at "); Serial.print(currentPower); Serial.println(" dBm");
    Radio.Send(msg, sizeof(msg) - 1);

    // Short delay before next loop
    delay(2000); 
  }
}
