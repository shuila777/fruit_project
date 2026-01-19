#include <SPI.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

Adafruit_BME280 bme;

// ---------- MCP3008 SPI 設定 ----------
const int CS_PIN = 53;

// ---------- MCP3008 Channel 定義 ----------
#define MQ2_CH      0
#define MQ3_CH      1
#define MQ9_CH      2
#define MQ135_CH    3
#define TGS2602_CH  4

const unsigned long SAMPLE_INTERVAL_MS = 1000;
const unsigned long PREHEAT_TIME_MS = 5UL * 60UL * 1000UL; // 5 分鐘

// ---------- 感測器資料結構 ----------
struct SensorData {
  int mq2_raw;
  int mq3_raw;
  int mq9_raw;
  int mq135_raw;
  int tgs2602_raw;
  float temperature;
  float humidity;
  float pressure;
};

// ---------- MCP3008 單一 Channel 讀取 ----------
int readMCP3008(int channel) {
  if (channel < 0 || channel > 7) return -1;

  digitalWrite(CS_PIN, LOW);
  SPI.transfer(0x01);
  byte highByte = SPI.transfer(0x80 | (channel << 4));
  byte lowByte  = SPI.transfer(0x00);
  digitalWrite(CS_PIN, HIGH);

  return ((highByte & 0x03) << 8) | lowByte;
}

// ---------- 一次讀取所有感測器 ----------
SensorData readAllSensors() {
  SensorData d;
  d.mq2_raw     = readMCP3008(MQ2_CH);
  d.mq3_raw     = readMCP3008(MQ3_CH);
  d.mq9_raw     = readMCP3008(MQ9_CH);
  d.mq135_raw   = readMCP3008(MQ135_CH);
  d.tgs2602_raw = readMCP3008(TGS2602_CH);

  d.temperature = bme.readTemperature();
  d.humidity    = bme.readHumidity();
  d.pressure    = bme.readPressure() / 100.0F;

  return d;
}

// ---------- 初始化 ----------
unsigned long startTime;
bool preheatDone = false;

void setup() {
  Serial.begin(9600);
  delay(1000);

  Wire.begin();  // ⭐ 非常重要

  if (!bme.begin(0x76)) {
    Serial.println("BME280 not found!");
    while(1);
  }

  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);

  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV16);
  SPI.setDataMode(SPI_MODE0);

  startTime = millis();

  Serial.println("=== 感測器預熱中（5 分鐘） ===");
}

// ---------- 主迴圈 ----------
void loop() {
  static unsigned long lastSampleTime = 0;
  unsigned long now = millis();

  // ---------- 預熱階段 ----------
  if (!preheatDone) {
    unsigned long elapsed = now - startTime;

    if (elapsed >= PREHEAT_TIME_MS) {
      preheatDone = true;
      Serial.println("✅ 預熱完成，開始正式讀值");
      Serial.println(
        "timestamp_ms,"
        "MQ2_raw,"
        "MQ3_raw,"
        "MQ9_raw,"
        "MQ135_raw,"
        "TGS2602_raw,"
        "Temp_C,"
        "Humidity_pct,"
        "Pressure_hPa"
      );
    } else {
      unsigned long remain = (PREHEAT_TIME_MS - elapsed) / 1000;
      int min = remain / 60;
      int sec = remain % 60;
      Serial.print("預熱中，剩餘時間：");
      Serial.print(min); Serial.print(" 分 ");
      Serial.print(sec); Serial.println(" 秒");
    }

    delay(1000);
    return;
  }

  // ---------- 正常讀值 ----------
  if (now - lastSampleTime < SAMPLE_INTERVAL_MS) return;
  lastSampleTime = now;

  SensorData d = readAllSensors();

  Serial.print(now);              
  Serial.print(",");
  Serial.print(d.mq2_raw);        
  Serial.print(",");
  Serial.print(d.mq3_raw);        Serial.print(",");
  Serial.print(d.mq9_raw);        Serial.print(",");
  Serial.print(d.mq135_raw);      Serial.print(",");
  Serial.print(d.tgs2602_raw);    Serial.print(",");
  Serial.print(d.temperature);    Serial.print(",");
  Serial.print(d.humidity);       Serial.print(",");
  Serial.println(d.pressure);
}
