#include <Arduino.h>
#include <SX126x-RAK4630.h>
#include <Adafruit_TinyUSB.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

static RadioEvents_t RadioEvents;

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// LoRa
#define RF_FREQUENCY            922000000
#define LORA_BANDWIDTH          0   // 125 kHz
#define LORA_CODINGRATE         1   // 4/5
#define LORA_PREAMBLE_LENGTH    8
#define LORA_SYMBOL_TIMEOUT     0
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON    false
#define RX_TIMEOUT_VALUE        0   // continuous RX

// User button
#define BUTTON_PIN 9

// SF control
int currentSF = 7;

// button debounce
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// latest received info
String lastMessage = "";
int lastRSSI = 0;
int lastSNR = 0;

// RX monitor
unsigned long lastRxTime = 0;
const unsigned long noRxDisplayTimeout = 4000;   // 4 seconds
bool noSignalShown = false;

void updateDisplay(bool showNoSignal = false)
{
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("SF: ");
  display.println(currentSF);

  display.setCursor(50, 0);
  display.print("RSSI:");
  display.println(lastRSSI);

  display.setCursor(0, 12);
  display.print("SNR: ");
  display.println(lastSNR);

  display.setCursor(0, 24);

  if (showNoSignal)
  {
    display.println("Status:");
    display.setTextSize(2);
    display.setCursor(0, 36);
    display.println("NO RX");
  }
  else
  {
    display.println("Message:");
    display.setTextSize(2);
    display.setCursor(0, 36);
    display.println(lastMessage);
  }

  display.display();
}

void applyRxConfig()
{
  Radio.Sleep();

  Radio.SetRxConfig(
    MODEM_LORA,
    LORA_BANDWIDTH,
    currentSF,
    LORA_CODINGRATE,
    0,
    LORA_PREAMBLE_LENGTH,
    LORA_SYMBOL_TIMEOUT,
    LORA_FIX_LENGTH_PAYLOAD_ON,
    0,
    true,   // CRC ON
    0,
    0,
    LORA_IQ_INVERSION_ON,
    true
  );

  Serial.print("Receiver switched to SF");
  Serial.println(currentSF);

  noSignalShown = false;
  updateDisplay(false);

  Radio.Rx(RX_TIMEOUT_VALUE);
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
{
  Serial.print("Received: ");

  lastMessage = "";
  for (int i = 0; i < size; i++)
  {
    Serial.print((char)payload[i]);
    lastMessage += (char)payload[i];
  }

  lastRSSI = rssi;
  lastSNR = snr;
  lastRxTime = millis();
  noSignalShown = false;

  Serial.print(" | RSSI: ");
  Serial.print(rssi);
  Serial.print(" | SNR: ");
  Serial.println(snr);

  updateDisplay(false);

  Radio.Rx(RX_TIMEOUT_VALUE);
}

void OnRxTimeout(void)
{
  Serial.println("RX timeout");
  Radio.Rx(RX_TIMEOUT_VALUE);
}

void OnRxError(void)
{
  Serial.println("RX error");
  Radio.Rx(RX_TIMEOUT_VALUE);
}

void setup()
{
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.begin(115200);
  delay(1000);
  Serial.println("LoRa Receiver");

  Wire.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  lastRxTime = millis();
  updateDisplay(false);

  lora_rak4630_init();

  RadioEvents.TxDone = NULL;
  RadioEvents.TxTimeout = NULL;
  RadioEvents.RxDone = OnRxDone;
  RadioEvents.RxTimeout = OnRxTimeout;
  RadioEvents.RxError = OnRxError;
  RadioEvents.CadDone = NULL;

  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);

  applyRxConfig();

  Serial.println("Receiver ready");
}

void loop()
{
  Radio.IrqProcess();

  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    static bool buttonHandled = false;

    if (reading == LOW && !buttonHandled) {
      currentSF++;
      if (currentSF > 12) {
        currentSF = 7;
      }

      applyRxConfig();
      buttonHandled = true;
    }

    if (reading == HIGH) {
      buttonHandled = false;
    }
  }

  lastButtonState = reading;

  // Show NO RX if no packet received for 4 seconds
  if (!noSignalShown && (millis() - lastRxTime >= noRxDisplayTimeout))
  {
    updateDisplay(true);
    noSignalShown = true;
    Serial.println("No packet received for 4 seconds");
  }

  delay(1);
}