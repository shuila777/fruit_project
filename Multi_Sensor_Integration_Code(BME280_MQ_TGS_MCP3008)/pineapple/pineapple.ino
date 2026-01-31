#include <SPI.h>
#include <Wire.h>
#include <math.h>
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

// ---------- (2) baseline 設定：用前 N 筆當 baseline ----------
const int BASELINE_SAMPLES = 30; // 建議 20~60：你可調整

// ---------- (4) rolling 平滑：移動平均窗口 ----------
const int MA_WIN = 5;

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
unsigned long recordStartTime = 0;
bool preheatDone = false;
bool recordingStarted = false;
int sampleCount = 0;

// -------- 最終統計用（全程累積，不印在中間）--------
long sum_mq2 = 0, sum_mq3 = 0, sum_mq9 = 0, sum_mq135 = 0, sum_tgs = 0;
float sum_temp = 0, sum_hum = 0, sum_press = 0;

int min_mq2 = 9999, min_mq3 = 9999, min_mq9 = 9999, min_mq135 = 9999, min_tgs = 9999;
int max_mq2 = 0,    max_mq3 = 0,    max_mq9 = 0,    max_mq135 = 0,    max_tgs = 0;

float min_temp = 9999, min_hum = 9999, min_press = 999999;
float max_temp = -9999, max_hum = -9999, max_press = -999999;

// -------- (2) baseline 累積（前 BASELINE_SAMPLES 筆）--------
bool baselineReady = false;
int baselineCount = 0;

long base_sum_mq2 = 0, base_sum_mq3 = 0, base_sum_mq9 = 0, base_sum_mq135 = 0, base_sum_tgs = 0;
float base_sum_temp = 0, base_sum_hum = 0, base_sum_press = 0;

float base_mq2 = 0, base_mq3 = 0, base_mq9 = 0, base_mq135 = 0, base_tgs = 0;
float base_temp = 0, base_hum = 0, base_press = 0;

// -------- (3) delta：前一筆 logr（用來算 dlogr）--------
bool hasPrev = false;
float prev_l_mq2 = 0, prev_l_mq3 = 0, prev_l_mq9 = 0, prev_l_mq135 = 0, prev_l_tgs = 0;

// -------- (4) rolling MA5：logr buffer --------
float buf_l_mq2[MA_WIN], buf_l_mq3[MA_WIN], buf_l_mq9[MA_WIN], buf_l_mq135[MA_WIN], buf_l_tgs[MA_WIN];
int bufIdx = 0;
int bufFilled = 0;

float meanBuf(const float *buf, int n) {
  if (n <= 0) return NAN;
  float s = 0;
  for (int i = 0; i < n; i++) s += buf[i];
  return s / (float)n;
}

// -------- (5) 異常檢查 --------
bool isValidInt(int v) { return v >= 0; }
bool isValidFloat(float x) { return isfinite(x); }

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin();

  if (!bme.begin(0x76) && !bme.begin(0x77)) {
    Serial.println("BME280 not found at 0x76 or 0x77!");
    while (1);
  }

  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);

  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV16);
  SPI.setDataMode(SPI_MODE0);

  startTime = millis();

  Serial.println("====================================");
  Serial.println("🍍 模式：測量鳳梨");
  Serial.println("====================================");
  Serial.println("=== 感測器預熱中（5 分鐘） ===");
}

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
      Serial.println("📋 模式：鳳梨測量");
      Serial.println("📝 請確認：");
      Serial.println("   - 鳳梨已放入容器");
      Serial.println("   - 蓋子已密封");
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

      for (int i = DELAY_START_SEC; i > 0; i--) {
        Serial.print("⏱️  倒數：");
        Serial.print(i);
        Serial.println(" 秒");
        delay(1000);
      }

      recordingStarted = true;
      recordStartTime = millis();

      Serial.println("");
      Serial.println("# 🚀 開始記錄！");
      Serial.println("# ### MODE: PINEAPPLE ###");
      Serial.println(
        "timestamp_ms,"
        "timestamp_s,"
        "MQ2_raw,MQ3_raw,MQ9_raw,MQ135_raw,TGS2602_raw,"
        "Temp_C,Humidity_pct,Pressure_hPa,"
        "MQ2_logr,MQ3_logr,MQ9_logr,MQ135_logr,TGS2602_logr,"
        "MQ2_dlogr,MQ3_dlogr,MQ9_dlogr,MQ135_dlogr,TGS2602_dlogr,"
        "MQ2_logr_ma5,MQ3_logr_ma5,MQ9_logr_ma5,MQ135_logr_ma5,TGS2602_logr_ma5"
      );
      Serial.print("# baseline will be computed from first ");
      Serial.print(BASELINE_SAMPLES);
      Serial.println(" samples.");
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

  if (!recordingStarted) return;

  // ---------- 取樣節奏 ----------
  if (now - lastSampleTime < SAMPLE_INTERVAL_MS) return;
  lastSampleTime = now;

  // ---------- 讀值 ----------
  SensorData d = readAllSensors();

  // ---------- (5) 異常值保護：任何 -1 或 NaN 就跳過不寫 CSV ----------
  if (!isValidInt(d.mq2_raw) || !isValidInt(d.mq3_raw) || !isValidInt(d.mq9_raw) ||
      !isValidInt(d.mq135_raw) || !isValidInt(d.tgs2602_raw) ||
      !isValidFloat(d.temperature) || !isValidFloat(d.humidity) || !isValidFloat(d.pressure)) {

    Serial.print("# WARN invalid read, skipped. raw=");
    Serial.print(d.mq2_raw); Serial.print(",");
    Serial.print(d.mq3_raw); Serial.print(",");
    Serial.print(d.mq9_raw); Serial.print(",");
    Serial.print(d.mq135_raw); Serial.print(",");
    Serial.print(d.tgs2602_raw);
    Serial.println();
    return;
  }

  // 到這裡才算有效樣本
  sampleCount++;

  // ---------- (2) baseline 累積 ----------
  if (!baselineReady) {
    base_sum_mq2 += d.mq2_raw;
    base_sum_mq3 += d.mq3_raw;
    base_sum_mq9 += d.mq9_raw;
    base_sum_mq135 += d.mq135_raw;
    base_sum_tgs += d.tgs2602_raw;

    base_sum_temp += d.temperature;
    base_sum_hum  += d.humidity;
    base_sum_press += d.pressure;

    baselineCount++;

    if (baselineCount >= BASELINE_SAMPLES) {
      baselineReady = true;

      base_mq2 = base_sum_mq2 / (float)baselineCount;
      base_mq3 = base_sum_mq3 / (float)baselineCount;
      base_mq9 = base_sum_mq9 / (float)baselineCount;
      base_mq135 = base_sum_mq135 / (float)baselineCount;
      base_tgs = base_sum_tgs / (float)baselineCount;

      base_temp = base_sum_temp / (float)baselineCount;
      base_hum  = base_sum_hum  / (float)baselineCount;
      base_press = base_sum_press / (float)baselineCount;

      // (2) baseline 印出來（# 開頭，不污染 CSV）
      Serial.print("# BASELINE_READY n="); Serial.print(baselineCount);
      Serial.print(" MQ2="); Serial.print(base_mq2, 2);
      Serial.print(" MQ3="); Serial.print(base_mq3, 2);
      Serial.print(" MQ9="); Serial.print(base_mq9, 2);
      Serial.print(" MQ135="); Serial.print(base_mq135, 2);
      Serial.print(" TGS="); Serial.print(base_tgs, 2);
      Serial.print(" T="); Serial.print(base_temp, 2);
      Serial.print(" H="); Serial.print(base_hum, 2);
      Serial.print(" P="); Serial.println(base_press, 2);
    }
  }

  // ---------- 全程最終統計累積（不在中間印） ----------
  sum_mq2 += d.mq2_raw;
  sum_mq3 += d.mq3_raw;
  sum_mq9 += d.mq9_raw;
  sum_mq135 += d.mq135_raw;
  sum_tgs += d.tgs2602_raw;

  sum_temp += d.temperature;
  sum_hum += d.humidity;
  sum_press += d.pressure;

  min_mq2 = min(min_mq2, d.mq2_raw);  max_mq2 = max(max_mq2, d.mq2_raw);
  min_mq3 = min(min_mq3, d.mq3_raw);  max_mq3 = max(max_mq3, d.mq3_raw);
  min_mq9 = min(min_mq9, d.mq9_raw);  max_mq9 = max(max_mq9, d.mq9_raw);
  min_mq135 = min(min_mq135, d.mq135_raw);  max_mq135 = max(max_mq135, d.mq135_raw);
  min_tgs = min(min_tgs, d.tgs2602_raw);    max_tgs = max(max_tgs, d.tgs2602_raw);

  min_temp = min(min_temp, d.temperature);  max_temp = max(max_temp, d.temperature);
  min_hum  = min(min_hum,  d.humidity);     max_hum  = max(max_hum,  d.humidity);
  min_press = min(min_press, d.pressure);   max_press = max(max_press, d.pressure);

  // ---------- (1) timestamp_s ----------
  unsigned long elapsed_ms = now - recordStartTime;
  float elapsed_s = elapsed_ms / 1000.0f;

  // ---------- 特徵：log ratio / delta / rolling ----------
  // baseline 未 ready 時：用當下值當 baseline（讓前幾筆也能輸出，不會 NaN）
  float b2 = baselineReady ? base_mq2 : (float)d.mq2_raw;
  float b3 = baselineReady ? base_mq3 : (float)d.mq3_raw;
  float b9 = baselineReady ? base_mq9 : (float)d.mq9_raw;
  float b135 = baselineReady ? base_mq135 : (float)d.mq135_raw;
  float btgs = baselineReady ? base_tgs : (float)d.tgs2602_raw;

  float l_mq2   = logf(((float)d.mq2_raw + 1.0f) / (b2 + 1.0f));
  float l_mq3   = logf(((float)d.mq3_raw + 1.0f) / (b3 + 1.0f));
  float l_mq9   = logf(((float)d.mq9_raw + 1.0f) / (b9 + 1.0f));
  float l_mq135 = logf(((float)d.mq135_raw + 1.0f) / (b135 + 1.0f));
  float l_tgs   = logf(((float)d.tgs2602_raw + 1.0f) / (btgs + 1.0f));

  // (3) delta：第一筆沒有 prev 就給 0
  float dl_mq2 = 0, dl_mq3 = 0, dl_mq9 = 0, dl_mq135 = 0, dl_tgs = 0;
  if (hasPrev) {
    dl_mq2 = l_mq2 - prev_l_mq2;
    dl_mq3 = l_mq3 - prev_l_mq3;
    dl_mq9 = l_mq9 - prev_l_mq9;
    dl_mq135 = l_mq135 - prev_l_mq135;
    dl_tgs = l_tgs - prev_l_tgs;
  } else {
    hasPrev = true;
  }
  prev_l_mq2 = l_mq2; prev_l_mq3 = l_mq3; prev_l_mq9 = l_mq9; prev_l_mq135 = l_mq135; prev_l_tgs = l_tgs;

  // (4) rolling MA5：把 logr 放進 buffer
  buf_l_mq2[bufIdx] = l_mq2;
  buf_l_mq3[bufIdx] = l_mq3;
  buf_l_mq9[bufIdx] = l_mq9;
  buf_l_mq135[bufIdx] = l_mq135;
  buf_l_tgs[bufIdx] = l_tgs;

  bufIdx = (bufIdx + 1) % MA_WIN;
  if (bufFilled < MA_WIN) bufFilled++;

  // 注意：buffer 是循環的，這裡用簡單方式：只對已填滿的部分做平均（可接受）
  float ma_mq2   = meanBuf(buf_l_mq2, bufFilled);
  float ma_mq3   = meanBuf(buf_l_mq3, bufFilled);
  float ma_mq9   = meanBuf(buf_l_mq9, bufFilled);
  float ma_mq135 = meanBuf(buf_l_mq135, bufFilled);
  float ma_tgs   = meanBuf(buf_l_tgs, bufFilled);

  // ---------- 輸出 CSV（只印純數據，不印任何提示） ----------
  Serial.print(elapsed_ms); Serial.print(",");
  Serial.print(elapsed_s, 3); Serial.print(",");

  Serial.print(d.mq2_raw); Serial.print(",");
  Serial.print(d.mq3_raw); Serial.print(",");
  Serial.print(d.mq9_raw); Serial.print(",");
  Serial.print(d.mq135_raw); Serial.print(",");
  Serial.print(d.tgs2602_raw); Serial.print(",");

  Serial.print(d.temperature, 2); Serial.print(",");
  Serial.print(d.humidity, 2); Serial.print(",");
  Serial.print(d.pressure, 2); Serial.print(",");

  Serial.print(l_mq2, 6); Serial.print(",");
  Serial.print(l_mq3, 6); Serial.print(",");
  Serial.print(l_mq9, 6); Serial.print(",");
  Serial.print(l_mq135, 6); Serial.print(",");
  Serial.print(l_tgs, 6); Serial.print(",");

  Serial.print(dl_mq2, 6); Serial.print(",");
  Serial.print(dl_mq3, 6); Serial.print(",");
  Serial.print(dl_mq9, 6); Serial.print(",");
  Serial.print(dl_mq135, 6); Serial.print(",");
  Serial.print(dl_tgs, 6); Serial.print(",");

  Serial.print(ma_mq2, 6); Serial.print(",");
  Serial.print(ma_mq3, 6); Serial.print(",");
  Serial.print(ma_mq9, 6); Serial.print(",");
  Serial.print(ma_mq135, 6); Serial.print(",");
  Serial.println(ma_tgs, 6);

  // ---------- 停止條件 ----------
  if (sampleCount >= MAX_SAMPLES) {
    Serial.println("# ====================================");
    Serial.println("# ✅ 記錄完成！");
    Serial.print("# 📊 samples="); Serial.println(sampleCount);
    Serial.println("# ====================================");

    // 最終統計（# 開頭，不污染 CSV）
    float avg_mq2   = sum_mq2 / (float)sampleCount;
    float avg_mq3   = sum_mq3 / (float)sampleCount;
    float avg_mq9   = sum_mq9 / (float)sampleCount;
    float avg_mq135 = sum_mq135 / (float)sampleCount;
    float avg_tgs   = sum_tgs / (float)sampleCount;

    float avg_temp  = sum_temp / (float)sampleCount;
    float avg_hum   = sum_hum / (float)sampleCount;
    float avg_press = sum_press / (float)sampleCount;

    Serial.println("# ==================== SUMMARY ====================");
    Serial.print("# MQ2   avg="); Serial.print(avg_mq2, 2); Serial.print(" min="); Serial.print(min_mq2); Serial.print(" max="); Serial.println(max_mq2);
    Serial.print("# MQ3   avg="); Serial.print(avg_mq3, 2); Serial.print(" min="); Serial.print(min_mq3); Serial.print(" max="); Serial.println(max_mq3);
    Serial.print("# MQ9   avg="); Serial.print(avg_mq9, 2); Serial.print(" min="); Serial.print(min_mq9); Serial.print(" max="); Serial.println(max_mq9);
    Serial.print("# MQ135 avg="); Serial.print(avg_mq135, 2); Serial.print(" min="); Serial.print(min_mq135); Serial.print(" max="); Serial.println(max_mq135);
    Serial.print("# TGS   avg="); Serial.print(avg_tgs, 2); Serial.print(" min="); Serial.print(min_tgs); Serial.print(" max="); Serial.println(max_tgs);
    Serial.print("# Temp  avg="); Serial.print(avg_temp, 2); Serial.print(" min="); Serial.print(min_temp, 2); Serial.print(" max="); Serial.println(max_temp, 2);
    Serial.print("# Hum   avg="); Serial.print(avg_hum, 2); Serial.print(" min="); Serial.print(min_hum, 2); Serial.print(" max="); Serial.println(max_hum, 2);
    Serial.print("# Press avg="); Serial.print(avg_press, 2); Serial.print(" min="); Serial.print(min_press, 2); Serial.print(" max="); Serial.println(max_press, 2);
    Serial.println("# =================================================");

    while (1);
  }
}
