/*
 * 鳳梨電子鼻 - 記憶體優化版
 * 
 * 優化重點:
 * - 移除 String 類別 (超耗記憶體!)
 * - 用 char[] 取代
 * - 減少全域變數
 * - 優化 Serial.print
 * 
 * 硬體接線:
 * BME280: VCC->3.3V, GND->GND, SDA->A4, SCL->A5
 * TGS-2602: VCC->5V, GND->GND, AO->A0
 * MQ-135: VCC->5V, GND->GND, AO->A2
 */

#include <BME280I2C.h>
#include <Wire.h>

// ========== 設定參數 ==========
#define SERIAL_BAUD 115200

// 感測器腳位
#define TGS2602_PIN A0
#define MQ135_PIN   A2

// 負載電阻 (kΩ)
#define RL_TGS2602  10.0
#define RL_MQ135    10.0

// 測試參數
#define PREHEAT_TIME 180000      // 3 分鐘
#define BASELINE_SAMPLES 60      // 60 秒
#define BASELINE_INTERVAL 1000   
#define TEST_DURATION 900000     // 15 分鐘
#define TEST_INTERVAL 1000       

// ========== ⚠️ 每次測試前修改這裡 ========== 
// 測試對象: 0=空間, 1=鳳梨A, 2=鳳梨B
#define TEST_PINEAPPLE 2

// 測試天數: 0-5
#define TEST_DAY 5

// 測試時段: 0=基準, 1=早, 2=中, 3=晚
#define TEST_TIME 3

// ========== 全域變數 (最小化) ==========
BME280I2C bme;

float baseline_TGS;
float baseline_MQ;

unsigned long testStart;
int sampleNum;

// ========== 初始化 ==========
void setup() {
  Serial.begin(SERIAL_BAUD);
  while(!Serial) {}
  
  // 標題
  Serial.println(F("========================================"));
  Serial.println(F("  Pineapple E-Nose v2.0"));
  Serial.println(F("========================================"));
  Serial.println();
  
  // 測試資訊
  Serial.print(F("Pineapple: "));
  if (TEST_PINEAPPLE == 0) Serial.println(F("Baseline"));
  else if (TEST_PINEAPPLE == 1) Serial.println(F("A"));
  else Serial.println(F("B"));
  
  Serial.print(F("Day: "));
  Serial.println(TEST_DAY);
  
  Serial.print(F("Time: "));
  if (TEST_TIME == 0) Serial.println(F("Baseline"));
  else if (TEST_TIME == 1) Serial.println(F("Morning"));
  else if (TEST_TIME == 2) Serial.println(F("Noon"));
  else Serial.println(F("Evening"));
  Serial.println();
  
  // 初始化 BME280
  Wire.begin();
  if(!bme.begin()) {
    Serial.println(F("ERROR: BME280!"));
    while(1) delay(1000);
  }
  Serial.println(F("[OK] BME280"));
  
  // 初始化感測器
  pinMode(TGS2602_PIN, INPUT);
  pinMode(MQ135_PIN, INPUT);
  Serial.println(F("[OK] Sensors"));
  Serial.println();
  
  // 模式提示
  if (TEST_PINEAPPLE == 0) {
    Serial.println(F("BASELINE MODE"));
    Serial.println(F("No fruit nearby!"));
  } else {
    Serial.println(F("TEST MODE"));
    Serial.println(F("Put pineapple in!"));
  }
  Serial.println();
  
  // 預熱
  Serial.println(F("=== PHASE 1: Preheat ==="));
  Serial.print(F("Wait "));
  Serial.print(PREHEAT_TIME / 60000);
  Serial.println(F(" min..."));
  
  unsigned long t0 = millis();
  while(millis() - t0 < PREHEAT_TIME) {
    if ((millis() - t0) % 30000 < 100) {
      Serial.print(F("  "));
      Serial.print((float)(millis() - t0) / PREHEAT_TIME * 100, 1);
      Serial.println(F("%"));
    }
    delay(100);
  }
  Serial.println(F("[OK] Preheated"));
  Serial.println();
  
  // 校正
  Serial.println(F("=== PHASE 2: Calibrate ==="));
  Serial.print(F("Sampling "));
  Serial.print(BASELINE_SAMPLES);
  Serial.println(F(" times..."));
  
  float sum_TGS = 0;
  float sum_MQ = 0;
  
  for(int i = 0; i < BASELINE_SAMPLES; i++) {
    sum_TGS += readSensor(TGS2602_PIN, RL_TGS2602);
    sum_MQ += readSensor(MQ135_PIN, RL_MQ135);
    
    if (i % 10 == 0) {
      Serial.print(F("  "));
      Serial.print(i);
      Serial.print(F("/"));
      Serial.println(BASELINE_SAMPLES);
    }
    
    delay(BASELINE_INTERVAL);
  }
  
  baseline_TGS = sum_TGS / BASELINE_SAMPLES;
  baseline_MQ = sum_MQ / BASELINE_SAMPLES;
  
  Serial.println();
  Serial.print(F("[OK] TGS="));
  Serial.print(baseline_TGS, 2);
  Serial.print(F(" MQ="));
  Serial.print(baseline_MQ, 2);
  Serial.println(F(" kOhm"));
  Serial.println();
  
  // 準備測試
  Serial.println(F("=== PHASE 3: Test ==="));
  Serial.println(F("Start in 3s..."));
  Serial.println();
  delay(3000);
  
  // 表頭
  printHeader();
  
  testStart = millis();
  sampleNum = 0;
}

// ========== 主迴圈 ==========
void loop() {
  unsigned long elapsed = millis() - testStart;
  
  // 檢查完成
  if (elapsed >= TEST_DURATION) {
    Serial.println();
    Serial.println(F("========================================"));
    Serial.println(F("  Complete!"));
    Serial.println(F("========================================"));
    Serial.print(F("Samples: "));
    Serial.println(sampleNum);
    Serial.println();
    Serial.println(F("1. Copy CSV to Sheets"));
    Serial.println(F("2. Record evaluation"));
    if (TEST_DAY == 5 && TEST_TIME == 3) {
      Serial.println(F("3. Cut & measure TSS!"));
    }
    Serial.println();
    
    while(1) delay(1000);
  }
  
  // 採樣
  readAndPrint();
  sampleNum++;
  
  delay(TEST_INTERVAL);
}

// ========== 函式 ==========

float readSensor(int pin, float RL) {
  int adc = analogRead(pin);
  float v = (float)adc / 1023.0 * 5.0;
  return RL * (5.0 - v) / v;
}

void readAndPrint() {
  // 讀環境
  float t, h, p;
  BME280::TempUnit tu(BME280::TempUnit_Celsius);
  BME280::PresUnit pu(BME280::PresUnit_hPa);
  bme.read(p, t, h, tu, pu);
  
  // 讀氣體
  float rs_tgs = readSensor(TGS2602_PIN, RL_TGS2602);
  float rs_mq = readSensor(MQ135_PIN, RL_MQ135);
  
  // 算 Change%
  float ch_tgs = ((rs_tgs - baseline_TGS) / baseline_TGS) * 100.0;
  float ch_mq = ((rs_mq - baseline_MQ) / baseline_MQ) * 100.0;
  
  // 時間 (秒)
  unsigned long sec = (millis() - testStart) / 1000;
  
  // CSV
  Serial.print(sec);
  Serial.print(',');
  Serial.print(t, 2);
  Serial.print(',');
  Serial.print(h, 2);
  Serial.print(',');
  Serial.print(p, 2);
  Serial.print(',');
  Serial.print(rs_tgs, 3);
  Serial.print(',');
  Serial.print(ch_tgs, 3);
  Serial.print(',');
  Serial.print(rs_mq, 3);
  Serial.print(',');
  Serial.print(ch_mq, 3);
  Serial.println();
}

void printHeader() {
  // 資訊
  Serial.print(F("# Pineapple: "));
  Serial.println(TEST_PINEAPPLE);
  Serial.print(F("# Day: "));
  Serial.println(TEST_DAY);
  Serial.print(F("# Time: "));
  Serial.println(TEST_TIME);
  Serial.print(F("# TGS_Base: "));
  Serial.print(baseline_TGS, 3);
  Serial.println(F(" kOhm"));
  Serial.print(F("# MQ_Base: "));
  Serial.print(baseline_MQ, 3);
  Serial.println(F(" kOhm"));
  Serial.println(F("# ─────────────────"));
  
  // 表頭
  Serial.println(F("Time(s),Temp(C),RH(%),Press(hPa),TGS_Rs(kOhm),TGS_Ch(%),MQ_Rs(kOhm),MQ_Ch(%)"));
}
