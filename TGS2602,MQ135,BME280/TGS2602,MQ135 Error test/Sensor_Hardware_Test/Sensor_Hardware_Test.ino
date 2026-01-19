/*
 * ============================================================================
 * TGS-2602 & MQ-135 硬體測試程式
 * ============================================================================
 * 
 * 用途: 檢測感測器基準值是否正常
 * 測試: 只測空氣,不放鳳梨
 * 
 * 正常範圍:
 * - TGS-2602: 12-18 kΩ
 * - MQ-135:   40-80 kΩ (視環境而定)
 * 
 * ============================================================================
 */

// ============================================================================
// 硬體設定
// ============================================================================
const int TGS_PIN = A0;      // TGS-2602 接 A0
const int MQ_PIN = A2;       // MQ-135 接 A2 (修正!)

const float VCC = 5.0;       // Arduino 供電電壓
const float RL_TGS = 10.0;   // TGS-2602 負載電阻 (kΩ)
const float RL_MQ = 10.0;    // MQ-135 負載電阻 (kΩ)

// ============================================================================
// 測試參數
// ============================================================================
const int PREHEAT_TIME = 10;      // 預熱時間 (秒) - 改成10秒確保穩定
const int BASELINE_SAMPLES = 30;  // 基準值採樣次數
const int TEST_DURATION = 60;     // 測試時間 (秒)
const int SAMPLE_INTERVAL = 1000; // 採樣間隔 (毫秒)

// ============================================================================
// 全域變數
// ============================================================================
float tgsBaseline = 0.0;
float mqBaseline = 0.0;

// ============================================================================
// 主程式
// ============================================================================

void setup() {
  Serial.begin(9600);
  
  Serial.println(F("============================================"));
  Serial.println(F("  TGS-2602 & MQ-135 硬體測試程式"));
  Serial.println(F("============================================"));
  Serial.println();
  
  // 預熱階段
  Serial.println(F("【預熱階段】"));
  Serial.print(F("預熱時間: "));
  Serial.print(PREHEAT_TIME);
  Serial.println(F(" 秒"));
  Serial.println(F("請等待感測器穩定..."));
  Serial.println();
  
  for (int i = PREHEAT_TIME; i > 0; i--) {
    Serial.print(F("剩餘: "));
    Serial.print(i);
    Serial.println(F(" 秒"));
    delay(1000);
  }
  
  Serial.println(F("預熱完成!"));
  Serial.println();
  
  // 建立基準值
  Serial.println(F("【建立基準值】"));
  Serial.print(F("採樣次數: "));
  Serial.println(BASELINE_SAMPLES);
  Serial.println(F("測量中..."));
  Serial.println();
  
  establishBaseline();
  
  // 顯示基準值
  Serial.println(F("============================================"));
  Serial.println(F("【基準值結果】"));
  Serial.println(F("============================================"));
  Serial.print(F("TGS-2602 基準: "));
  Serial.print(tgsBaseline, 3);
  Serial.print(F(" kΩ  "));
  
  // 判斷 TGS 是否正常
  if (tgsBaseline >= 12.0 && tgsBaseline <= 18.0) {
    Serial.println(F("✓ 正常範圍"));
  } else if (tgsBaseline > 25.0) {
    Serial.println(F("✗ 異常偏高! (可能接線鬆脫)"));
  } else if (tgsBaseline < 10.0) {
    Serial.println(F("✗ 異常偏低! (可能短路)"));
  } else {
    Serial.println(F("⚠ 略微偏離正常範圍"));
  }
  
  Serial.print(F("MQ-135 基準:  "));
  Serial.print(mqBaseline, 3);
  Serial.print(F(" kΩ  "));
  
  // 判斷 MQ 是否正常
  if (mqBaseline >= 40.0 && mqBaseline <= 80.0) {
    Serial.println(F("✓ 正常範圍"));
  } else if (mqBaseline < 30.0) {
    Serial.println(F("✗ 異常偏低! (可能短路或損壞)"));
  } else if (mqBaseline > 100.0) {
    Serial.println(F("✗ 異常偏高! (可能接線鬆脫)"));
  } else {
    Serial.println(F("⚠ 略微偏離正常範圍"));
  }
  
  Serial.println(F("============================================"));
  Serial.println();
  
  // 準備開始測試
  Serial.println(F("【穩定性測試】"));
  Serial.print(F("測試時間: "));
  Serial.print(TEST_DURATION);
  Serial.println(F(" 秒"));
  Serial.println(F("觀察 Change% 是否接近 0%..."));
  Serial.println();
  
  Serial.println(F("Time(s)\tTGS_Rs(kΩ)\tTGS_Ch(%)\tMQ_Rs(kΩ)\tMQ_Ch(%)"));
  Serial.println(F("---------------------------------------------------------------"));
}

void loop() {
  static unsigned long startTime = millis();
  static int sampleCount = 0;
  static float tgsSum = 0.0;
  static float mqSum = 0.0;
  
  unsigned long currentTime = millis();
  int elapsedSeconds = (currentTime - startTime) / 1000;
  
  if (elapsedSeconds < TEST_DURATION) {
    // 每秒採樣一次
    if (currentTime - startTime >= sampleCount * SAMPLE_INTERVAL) {
      float tgsRs = readTGS();
      float mqRs = readMQ();
      
      float tgsChange = ((tgsRs - tgsBaseline) / tgsBaseline) * 100.0;
      float mqChange = ((mqRs - mqBaseline) / mqBaseline) * 100.0;
      
      tgsSum += tgsChange;
      mqSum += mqChange;
      sampleCount++;
      
      // 輸出數據
      Serial.print(elapsedSeconds);
      Serial.print(F("\t"));
      Serial.print(tgsRs, 3);
      Serial.print(F("\t\t"));
      Serial.print(tgsChange, 3);
      Serial.print(F("\t\t"));
      Serial.print(mqRs, 3);
      Serial.print(F("\t\t"));
      Serial.println(mqChange, 3);
    }
  } else if (elapsedSeconds == TEST_DURATION && sampleCount > 0) {
    // 測試結束,顯示統計
    Serial.println(F("---------------------------------------------------------------"));
    Serial.println();
    Serial.println(F("============================================"));
    Serial.println(F("【測試結果統計】"));
    Serial.println(F("============================================"));
    
    float tgsAvgChange = tgsSum / sampleCount;
    float mqAvgChange = mqSum / sampleCount;
    
    Serial.print(F("TGS-2602 平均 Change%: "));
    Serial.print(tgsAvgChange, 3);
    Serial.print(F("%  "));
    
    if (abs(tgsAvgChange) <= 0.5) {
      Serial.println(F("✓ 優良 (±0.5%)"));
    } else if (abs(tgsAvgChange) <= 2.0) {
      Serial.println(F("⚠ 可接受 (±2%)"));
    } else {
      Serial.println(F("✗ 異常! (>±2%)"));
    }
    
    Serial.print(F("MQ-135 平均 Change%:  "));
    Serial.print(mqAvgChange, 3);
    Serial.print(F("%  "));
    
    if (abs(mqAvgChange) <= 2.0) {
      Serial.println(F("✓ 優良 (±2%)"));
    } else if (abs(mqAvgChange) <= 5.0) {
      Serial.println(F("⚠ 可接受 (±5%)"));
    } else {
      Serial.println(F("✗ 異常! (>±5%)"));
    }
    
    Serial.println(F("============================================"));
    Serial.println();
    
    // 最終判斷
    Serial.println(F("【最終判斷】"));
    
    boolean tgsOK = (tgsBaseline >= 12.0 && tgsBaseline <= 18.0) && (abs(tgsAvgChange) <= 2.0);
    boolean mqOK = (mqBaseline >= 40.0 && mqBaseline <= 80.0) && (abs(mqAvgChange) <= 5.0);
    
    if (tgsOK && mqOK) {
      Serial.println(F("✓ 硬體狀態: 正常"));
      Serial.println(F("✓ 可以開始測試鳳梨!"));
    } else if (tgsOK && !mqOK) {
      Serial.println(F("⚠ TGS-2602: 正常"));
      Serial.println(F("✗ MQ-135: 異常"));
      Serial.println(F("⚠ 建議: 只使用 TGS-2602 數據"));
    } else if (!tgsOK && mqOK) {
      Serial.println(F("✗ TGS-2602: 異常"));
      Serial.println(F("⚠ MQ-135: 正常"));
      Serial.println(F("✗ 建議: 檢查 TGS-2602 接線"));
    } else {
      Serial.println(F("✗ 硬體狀態: 異常"));
      Serial.println(F("✗ 需要檢查接線或更換感測器"));
    }
    
    Serial.println(F("============================================"));
    Serial.println();
    Serial.println(F("測試完成! 請記錄結果。"));
    Serial.println(F("重新上電可再次測試。"));
    
    // 停止測試
    while(1);
  }
}

// ============================================================================
// 讀取 TGS-2602
// ============================================================================
float readTGS() {
  int adcValue = analogRead(TGS_PIN);
  float voltage = (adcValue / 1023.0) * VCC;
  
  // 避免除以零
  if (voltage <= 0.01) voltage = 0.01;
  if (voltage >= 4.99) voltage = 4.99;
  
  float rs = RL_TGS * ((VCC / voltage) - 1.0);
  return rs;
}

// ============================================================================
// 讀取 MQ-135
// ============================================================================
float readMQ() {
  int adcValue = analogRead(MQ_PIN);
  float voltage = (adcValue / 1023.0) * VCC;
  
  // 避免除以零
  if (voltage <= 0.01) voltage = 0.01;
  if (voltage >= 4.99) voltage = 4.99;
  
  float rs = RL_MQ * ((VCC / voltage) - 1.0);
  return rs;
}

// ============================================================================
// 建立基準值
// ============================================================================
void establishBaseline() {
  float tgsSum = 0.0;
  float mqSum = 0.0;
  
  for (int i = 0; i < BASELINE_SAMPLES; i++) {
    tgsSum += readTGS();
    mqSum += readMQ();
    
    // 顯示進度
    if ((i + 1) % 10 == 0) {
      Serial.print(F("進度: "));
      Serial.print(i + 1);
      Serial.print(F("/"));
      Serial.println(BASELINE_SAMPLES);
    }
    
    delay(100);  // 100ms 間隔
  }
  
  tgsBaseline = tgsSum / BASELINE_SAMPLES;
  mqBaseline = mqSum / BASELINE_SAMPLES;
  
  Serial.println(F("基準值建立完成!"));
  Serial.println();
}
