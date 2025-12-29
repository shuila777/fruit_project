# Multi-Sensor Measurement Platform - Specifications

## 1. 系統目的
本系統用於整合多顆氣體感測器，透過 MCP3008 ADC 將類比訊號集中讀取，
提供後續資料分析與水果熟成度判斷使用。

## 2. 硬體架構
- 主控制器：Arduino Mega
- ADC：MCP3008 (SPI 介面)
- 感測器：MQ 系列 / TGS 系列氣體感測器

## 3. MCP3008 Channel 對應表

| MCP3008 Channel | 感測器 | 備註 |
|----------------|--------|------|
| CH0 | MQ-3 | 乙醇 |
| CH1 | MQ-7 | CO |
| CH2 | MQ-9 | CH4 |
| CH3 | MQ-135 | VOC |
| CH4 | TGS2602 | 預留 |
| CH5–CH7 | 未使用 | 預留 |

## 4. 資料輸出格式 (CSV)
- timestamp_ms：系統啟動後毫秒數
- 感測器數值：ADC raw value (0–1023)

## 5. 取樣規格
- 取樣間隔：1 秒
- 同一時間點讀取所有 channel
- 後處理前不做校正或濾波

## 6. 備註
- 各感測器接法與 load resistor 請參考各 Uno 測試記錄
- 預留通道供未來擴充
