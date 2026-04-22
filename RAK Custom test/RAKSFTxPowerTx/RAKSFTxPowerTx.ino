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
#define RF_FREQUENCY            922000000
#define LORA_BANDWIDTH          0   // 125 kHz
#define LORA_CODINGRATE         1   // 4/5
#define LORA_PREAMBLE_LENGTH    8
#define TX_TIMEOUT_VALUE        3000

// Button
#define BUTTON_PIN 9

static RadioEvents_t RadioEvents;

volatile bool txDone = true;
int currentSF = 7;
int currentPower = 14;
int rawADC = 0;

unsigned long lastTxTime = 0;
const unsigned long txInterval = 2000;   // send every 2 seconds

bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

void OnTxDone(void)
{
  Serial.println("TX Done");
  txDone = true;
}

void OnTxTimeout(void)
{
  Serial.println("TX Timeout");
  txDone = true;
}

void updateDisplay()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.print("SF: ");
  display.print(currentSF);

  display.setCursor(70, 0);
  display.print("ADC:");
  display.print(rawADC);

  display.drawLine(0, 12, 127, 12, SSD1306_WHITE);

  display.setCursor(0, 25);
  display.setTextSize(2);
  display.print(currentPower);
  display.print(" dBm");

  display.setTextSize(1);
  display.setCursor(0, 54);
  display.print("TX:");
  display.print(txDone ? "Ready" : "Busy");

  display.display();
}

void setup()
{
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.begin(115200);
  delay(100);

  Wire.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println("OLED failed");
    while (1);
  }

  display.clearDisplay();
  display.display();

  lora_rak4630_init();

  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;

  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);

  Serial.println("System Ready");
  updateDisplay();
}

void loop()
{
  Radio.IrqProcess();

  // -------- Button handling with debounce --------
  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    static bool buttonHandled = false;

    if (reading == LOW && !buttonHandled) {
      currentSF++;
      if (currentSF > 12) currentSF = 7;

      Serial.print("SF Switched to: ");
      Serial.println(currentSF);

      updateDisplay();
      buttonHandled = true;
    }

    if (reading == HIGH) {
      buttonHandled = false;
    }
  }

  lastButtonState = reading;

  // -------- Read ADC and update display --------
  rawADC = analogRead(WB_A1);
  currentPower = map(rawADC, 0, 900, -9, 22);
  currentPower = constrain(currentPower, -9, 22);

  updateDisplay();

  // -------- Send packet periodically --------
  if (txDone && (millis() - lastTxTime >= txInterval)) {
    Radio.SetTxConfig(
      MODEM_LORA,
      currentPower,
      0,
      LORA_BANDWIDTH,
      currentSF,
      LORA_CODINGRATE,
      LORA_PREAMBLE_LENGTH,
      false,
      true,
      0,
      0,
      false,
      TX_TIMEOUT_VALUE
    );

    uint8_t msg[] = "HELLO";

    txDone = false;
    lastTxTime = millis();

    Serial.print("Sending @ SF");
    Serial.print(currentSF);
    Serial.print(" / ");
    Serial.print(currentPower);
    Serial.println(" dBm");

    Radio.Send(msg, sizeof(msg) - 1);
  }

  delay(10);
}