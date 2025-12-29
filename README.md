# Fruit Gas Sensing Platform

本專案為氣體感測與資料蒐集平台，使用 Arduino 與 Raspberry Pi
搭配 MCP3008 ADC，作為多顆氣體感測器（MQ / TGS 系列）的資料擷取核心，
後續將進行資料分析與 AI 應用。

---

## 📌 Current Progress (Week X)

### Completed
- MCP3008 與 Arduino Mega 實體接線與讀值驗證
- MCP3008 與 Raspberry Pi（SPI）讀值驗證
- Python 環境與 SPI 通訊確認
- 專案 Git 結構建立（Arduino / Raspberry Pi / Docs）

### Pending
- MQ / TGS 感測器實際接入（硬體尚未到位）
- 多通道感測器同步量測測試
- 資料儲存與分析模組

---

## 🧱 Project Structure

fruit_project/
├─ arduino/ # Arduino 程式（感測器前端）
├─ raspberry_pi/ # Raspberry Pi Python 程式（ADC / 資料處理）
├─ docs/ # 感測器規格與實驗文件
└─ README.md

---

## 🛠 Hardware Used
- Arduino Mega 2560
- Raspberry Pi 3 / 4
- MCP3008 (10-bit ADC)
- Breadboard & Dupont wires

---

## 🚀 How to Run (Raspberry Pi)

```bash
python3 raspberry_pi/mcp3008.py
