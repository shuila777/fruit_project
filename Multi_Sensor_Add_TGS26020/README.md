# Multi Sensor Fruit Project (TGS2602 + TGS2620)

## 專案簡介
本專案為多感測器氣體量測系統，使用 Arduino 搭配 MQ 系列感測器、
TGS2602 以及額外新增之 TGS2620。

本程式碼係基於  
**「 Multi_Sensor_Integration_Code(BME280_MQ_TGS_MCP3008) 」**  
專案進行延伸，主要新增 **TGS2620 感測器之讀取與資料紀錄功能**，
用以驗證 MQ-3 於酒精/揮發性氣體量測上的數據可靠性。

本專案整體目的為量測：
- 空氣背景（baseline）
- 鳳梨揮發性氣體

並即時輸出 CSV 格式資料，作為後續資料分析與成熟度研究使用。

---

## 使用感測器
- MQ-2  
- MQ-3  
- MQ-9  
- MQ-135  
- TGS2602  
- TGS2620  
  - ⚠️ 若未使用 TGS2620，可改用原始專案  
    `Multi_Sensor_Integration_Code(BME280_MQ_TGS_MCP3008) `
- BME280（溫度 / 濕度 / 氣壓）

---

## 檔案說明

### `air_all_sensors_add_tgs2620.ino`
- 用途：量測「空氣背景」
- 說明：
  - 容器中 **不放鳳梨**
  - 用於建立各感測器 baseline
  - 輸出資料包含 raw / log ratio / delta / moving average

### `pineapple_all_sensors_add_tgs2620.ino`
- 用途：量測「鳳梨樣本」
- 說明：
  - 容器中放入鳳梨
  - 與 air 版本流程相同，僅量測對象不同
  - 可直接與 air baseline 進行比較分析

---

## 輸出資料格式
Serial Monitor 會輸出 CSV 格式資料，包含：
- 原始 ADC 值（raw）
- Log ratio（相對 baseline）
- Delta（時間變化）
- MA5（移動平均）
- 溫度 / 濕度 / 氣壓

---

## 專案用途
本程式為課程專題與實驗資料蒐集使用，
後續資料可應用於：
- 鳳梨成熟度分析
- 感測器反應比較
- 機器學習 / SVM 特徵萃取

---

## 建議操作流程
1. 量測空氣背景（baseline）
2. 量測第一顆鳳梨
3. 再次量測空氣背景
4. 量測下一顆鳳梨
