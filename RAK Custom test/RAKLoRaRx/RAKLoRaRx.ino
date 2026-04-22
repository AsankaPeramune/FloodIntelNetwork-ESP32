#include <Arduino.h>
#include <SX126x-RAK4630.h>
#include <Adafruit_TinyUSB.h>
#include <SPI.h>

static RadioEvents_t RadioEvents;

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
  for (int i = 0; i < size; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.print(" | RSSI: ");
  Serial.print(rssi);
  Serial.print(" | SNR: ");
  Serial.println(snr);

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
  Serial.begin(115200);
  delay(2000);
  Serial.println("LoRa Receiver");
  Serial1.setPins(15, 16);
  Serial1.begin(38400);

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
