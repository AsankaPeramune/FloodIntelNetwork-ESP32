#include <Arduino.h>
#include <SX126x-RAK4630.h>
#include <Adafruit_TinyUSB.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

static RadioEvents_t RadioEvents;

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET    -1 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define RF_FREQUENCY            916000000
#define LORA_BANDWIDTH          0   // 125 kHz
#define LORA_SPREADING_FACTOR   7
#define LORA_CODINGRATE         1   // 4/5
#define LORA_PREAMBLE_LENGTH    8
#define LORA_SYMBOL_TIMEOUT     0
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON    false
#define RX_TIMEOUT_VALUE        0   // continuous RX

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
{
  Serial.print("Received: ");

   //  Prepare OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Message:");
  display.setCursor(0,10);
  display.setTextSize(2);

  for (int i = 0; i < size; i++)
  {
    Serial.print((char)payload[i]);
    display.print((char)payload[i]);
  }
  Serial.print(" | RSSI: ");
  Serial.print(rssi);
  Serial.print(" | SNR: ");
  Serial.println(snr);

  display.setTextSize(1);
  display.setCursor(0,45);
  display.print("RSSI: ");
  display.print(rssi);
  display.print(" SNR: ");
  display.println(snr);

  display.display(); // Push data to screen

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
    // Initialize OLED with I2C address 0x3C
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); 
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  Serial.begin(115200);
  delay(2000);
  Serial.println("LoRa Receiver");
 
  lora_rak4630_init();

  RadioEvents.TxDone = NULL;
  RadioEvents.TxTimeout = NULL;
  RadioEvents.RxDone = OnRxDone;
  RadioEvents.RxTimeout = OnRxTimeout;
  RadioEvents.RxError = OnRxError;
  RadioEvents.CadDone = NULL;

  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);

  Radio.SetRxConfig(
    MODEM_LORA,
    LORA_BANDWIDTH,
    LORA_SPREADING_FACTOR,
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

  Serial.println("Receiver ready");
  Radio.Rx(RX_TIMEOUT_VALUE);
}

void loop()
{
  Radio.IrqProcess();
  delay(1);
}
