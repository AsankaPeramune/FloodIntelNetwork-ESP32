#include <Arduino.h>
#include <SX126x-RAK4630.h>
#include <SPI.h>

static RadioEvents_t RadioEvents;
bool txDone = true;

#define RF_FREQUENCY            916000000
#define TX_OUTPUT_POWER         22
#define LORA_BANDWIDTH          0   // 125 kHz
#define LORA_SPREADING_FACTOR   7
#define LORA_CODINGRATE         1   // 4/5
#define LORA_PREAMBLE_LENGTH    8
#define LORA_SYMBOL_TIMEOUT     0
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON    false
#define TX_TIMEOUT_VALUE        3000

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
  delay(2000);
  Serial.println("LoRa Sender");

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
    true,   // CRC ON
    0,
    0,
    LORA_IQ_INVERSION_ON,
    TX_TIMEOUT_VALUE
  );

  Serial.println("Sender ready");
}

void loop()
{
  Radio.IrqProcess();

  if (txDone)
  {
    uint8_t msg[] = "HELLO";
    txDone = false;
    Serial.println("Sending HELLO");
    Radio.Send(msg, sizeof(msg) - 1);

    unsigned long start = millis();
    while (!txDone && millis() - start < 2000)
    {
      Radio.IrqProcess();
      delay(1);
    }

    delay(5000);
  }
}