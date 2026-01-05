#include <SPI.h>

/* =========================
   MCP3008 SPI 設定
   ========================= */
const int CS_PIN = 53;   // Arduino Mega SPI CS 腳位

/* =========================
   MCP3008 Channel 定義
   （語意層，整合重點）
   ========================= */
#define MQ135_CH    0
#define TGS2602_CH  1
#define MQ3_CH      2
#define CH3_CH      3   // 預留（可接其他類比感測器）

/* =========================
   取樣設定
   ========================= */
const unsigned long SAMPLE_INTERVAL_MS = 1000;

/* =========================
   MCP3008 讀取函式（原樣保留）
   ========================= */
int readMCP3008(int channel) {
  if (channel < 0 || channel > 7) return -1;

  digitalWrite(CS_PIN, LOW);

  byte command1 = 0b00000001;                  // Start bit
  byte command2 = 0b10000000 | (channel << 4); // Single-ended + channel
  byte command3 = 0b00000000;

  SPI.transfer(command1);
  byte highByte = SPI.transfer(command2);
  byte lowByte  = SPI.transfer(command3);

  digitalWrite(CS_PIN, HIGH);

  int value = ((highByte & 0x03) << 8) | lowByte;
  return value;
}

/* =========================
   感測器資料結構（系統整合）
   ========================= */
struct SensorData {
  int mq135_raw;
  int tgs2602_raw;
  int mq3_raw;
  int ch3_raw;
};

/* =========================
   統一讀取所有感測器
   ========================= */
SensorData readAllSensors() {
  SensorData d;
  d.mq135_raw   = readMCP3008(MQ135_CH);
  d.tgs2602_raw = readMCP3008(TGS2602_CH);
  d.mq3_raw     = readMCP3008(MQ3_CH);
  d.ch3_raw     = readMCP3008(CH3_CH);
  return d;
}

/* =========================
   初始化
   ========================= */
void setup() {
  Serial.begin(9600);

  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);

  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV16);
  SPI.setDataMode(SPI_MODE0);

  // CSV Header（系統輸出格式）
  Serial.println("timestamp_ms,MQ135_raw,TGS2602_raw,MQ3_raw,CH3_raw");
}

/* =========================
   主迴圈（系統層）
   ========================= */
void loop() {
  static unsigned long lastSampleTime = 0;
  unsigned long now = millis();

  if (now - lastSampleTime < SAMPLE_INTERVAL_MS) return;
  lastSampleTime = now;

  SensorData data = readAllSensors();

  Serial.print(now);
  Serial.print(",");
  Serial.print(data.mq135_raw);
  Serial.print(",");
  Serial.print(data.tgs2602_raw);
  Serial.print(",");
  Serial.print(data.mq3_raw);
  Serial.print(",");
  Serial.println(data.ch3_raw);
}
