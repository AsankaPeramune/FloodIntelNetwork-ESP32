#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <SX126x-RAK4630.h>
#include <SPI.h>

uint8_t frame[4];

static RadioEvents_t RadioEvents;
bool txDone = true;

#define RF_FREQUENCY            915000000
#define TX_OUTPUT_POWER         14
#define LORA_BANDWIDTH          0
#define LORA_SPREADING_FACTOR   7
#define LORA_CODINGRATE         1
#define LORA_PREAMBLE_LENGTH    8
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON    false
#define TX_TIMEOUT_VALUE        3000

#define SEND_INTERVAL_MS        5000

int latest_distance_mm = -1;
unsigned long lastSendTime = 0;

void OnTxDone(void)
{
  Serial.println("Send done");
  txDone = true;
}

void OnTxTimeout(void)
{
  Serial.println("TX timeout");
  txDone = true;
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("A01NYUB LoRa Sender");

  Serial1.setPins(15, 16);
  Serial1.begin(9600);
  Serial.println("Listening on Serial1...");

  lora_rak4630_init();

  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  RadioEvents.RxDone = NULL;
  RadioEvents.RxTimeout = NULL;
  RadioEvents.RxError = NULL;
  RadioEvents.CadDone = NULL;

  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);

  Radio.SetTxConfig(
    MODEM_LORA,
    TX_OUTPUT_POWER,
    0,
    LORA_BANDWIDTH,
    LORA_SPREADING_FACTOR,
    LORA_CODINGRATE,
    LORA_PREAMBLE_LENGTH,
    LORA_FIX_LENGTH_PAYLOAD_ON,
    true,
    0,
    0,
    LORA_IQ_INVERSION_ON,
    TX_TIMEOUT_VALUE
  );

  Serial.println("LoRa sender ready");
}

void loop()
{
  Radio.IrqProcess();

  // Read sensor continuously
  while (Serial1.available() >= 4)
  {
    if (Serial1.read() == 0xFF)
    {
      frame[0] = 0xFF;
      frame[1] = Serial1.read();
      frame[2] = Serial1.read();
      frame[3] = Serial1.read();

      uint8_t checksum = (frame[0] + frame[1] + frame[2]) & 0xFF;

      if (checksum == frame[3])
      {
        latest_distance_mm = (frame[1] << 8) | frame[2];

        Serial.print("Distance: ");
        Serial.print(latest_distance_mm);
        Serial.println(" mm");
      }
      else
      {
        Serial.println("Checksum error");
      }
    }
  }

  // Send only once every 5 seconds
  if (latest_distance_mm >= 0 && txDone && (millis() - lastSendTime >= SEND_INTERVAL_MS))
  {
    char msg[32];
    snprintf(msg, sizeof(msg), "DIST:%d", latest_distance_mm);

    txDone = false;
    lastSendTime = millis();

    Serial.print("Sending: ");
    Serial.println(msg);

    Radio.Send((uint8_t *)msg, strlen(msg));

    unsigned long start = millis();
    while (!txDone && millis() - start < 2000)
    {
      Radio.IrqProcess();
      delay(1);
    }
  }
}