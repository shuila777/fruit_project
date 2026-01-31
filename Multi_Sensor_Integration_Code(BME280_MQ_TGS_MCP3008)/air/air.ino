#include <SPI.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

Adafruit_BME280 bme;

// ---------- MCP3008 SPI 設定 ----------
const int CS_PIN = 53;  // Mega 用 53，UNO 用 10

// ---------- MCP3008 Channel 定義 ----------
#define MQ2_CH      0
#define MQ3_CH      1
#define MQ9_CH      2
#define MQ135_CH    3
#define TGS2602_CH  4

const unsigned long SAMPLE_INTERVAL_MS = 1000;
const unsigned long PREHEAT_TIME_MS = 5UL * 60UL * 1000UL; // 5 分鐘
const unsigned long DELAY_START_SEC = 30; // 延遲啟動時間（秒）
const int MAX_SAMPLES = 900; // 最多記錄 900 筆

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
unsigned long recordStartTime = 0;  // 👈 新增：記錄開始時間
bool preheatDone = false;
bool recordingStarted = false;
int sampleCount = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin();

  if (!bme.begin(0x76) && !bme.begin(0x77)) {
    Serial.println("BME280 not found at 0x76 or 0x77!");
    while(1);
  }

  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);

  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV16);
  SPI.setDataMode(SPI_MODE0);

  startTime = millis();

  Serial.println("====================================");
  Serial.println("🌬️  模式：測量空氣背景");
  Serial.println("====================================");
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
      Serial.println("");
      Serial.println("✅ 預熱完成！");
      Serial.println("📋 模式：空氣背景測量");
      Serial.println("📝 請確認：");
      Serial.println("   - 容器是空的（沒有鳳梨）");
      Serial.println("   - 蓋子已按照實驗設計擺放");
      Serial.println("");
      Serial.print("⏰ ");
      Serial.print(DELAY_START_SEC);
      Serial.println(" 秒後自動開始記錄");
      Serial.print("📊 將記錄 ");
      Serial.print(MAX_SAMPLES);
      Serial.print(" 筆數據（約 ");
      Serial.print(MAX_SAMPLES / 60);
      Serial.println(" 分鐘）");
      Serial.println("👉 請快速離開房間！");
      Serial.println("");
      
      // 倒數計時
      for (int i = DELAY_START_SEC; i > 0; i--) {
        Serial.print("⏱️  倒數：");
        Serial.print(i);
        Serial.println(" 秒");
        delay(1000);
      }
      
      recordingStarted = true;
      recordStartTime = millis();  // 👈 記錄開始記錄的時間點
      
      Serial.println("");
      Serial.println("🚀 開始記錄！");
      Serial.println("### MODE: AIR ###");
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

  // ---------- 等待記錄開始 ----------
  if (!recordingStarted) {
    return;
  }

  // ---------- 檢查是否已達到最大筆數 ----------
  if (sampleCount >= MAX_SAMPLES) {
    Serial.println("");
    Serial.println("====================================");
    Serial.println("✅ 記錄完成！");
    Serial.print("📊 已記錄 ");
    Serial.print(sampleCount);
    Serial.println(" 筆數據");
    Serial.println("====================================");
    Serial.println("🛑 程式已自動停止");
    Serial.println("📋 請複製上方所有數據並存檔為 air_data.csv");
    while(1);
  }

  // ---------- 正常讀值 ----------
  if (now - lastSampleTime < SAMPLE_INTERVAL_MS) return;
  lastSampleTime = now;

  SensorData d = readAllSensors();
  sampleCount++;

  // 👇 計算從開始記錄到現在的時間（從 0 開始）
  unsigned long elapsed_ms = now - recordStartTime;

  // 輸出 CSV 數據
  Serial.print(elapsed_ms);  // 👈 改用相對時間
  Serial.print(",");
  Serial.print(d.mq2_raw);        
  Serial.print(",");
  Serial.print(d.mq3_raw);        
  Serial.print(",");
  Serial.print(d.mq9_raw);        
  Serial.print(",");
  Serial.print(d.mq135_raw);      
  Serial.print(",");
  Serial.print(d.tgs2602_raw);    
  Serial.print(",");
  Serial.print(d.temperature);    
  Serial.print(",");
  Serial.print(d.humidity);       
  Serial.print(",");
  Serial.println(d.pressure);

  // 每 10 筆顯示一次統計資訊
  if (sampleCount % 10 == 0) {
    Serial.print("# [");
    Serial.print(sampleCount);
    Serial.print("/");
    Serial.print(MAX_SAMPLES);
    Serial.print("] MQ2:");
    Serial.print(d.mq2_raw);
    Serial.print(" MQ3:");
    Serial.print(d.mq3_raw);
    Serial.print(" MQ9:");
    Serial.print(d.mq9_raw);
    Serial.print(" MQ135:");
    Serial.print(d.mq135_raw);
    Serial.print(" TGS:");
    Serial.print(d.tgs2602_raw);
    Serial.print(" T:");
    Serial.print(d.temperature, 1);
    Serial.print("°C H:");
    Serial.print(d.humidity, 1);
    Serial.println("%");
  }
}