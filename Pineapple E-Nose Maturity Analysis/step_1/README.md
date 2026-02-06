# Pineapple E-Nose – Step 1 資料清洗與初步分析

本專案為「鳳梨氣味感測成熟度分析」之 **Step 1：自動化資料清洗與初步分析（EDA）**，  
目標是確認感測資料品質、可用性，並為後續成熟度建模（Step 3 / Step 4）做準備。

---

## 專案目錄結構

```text
workspace/
├─ data/
│  ├─ raw/                # 原始感測資料（Excel / CSV）
│  │   ├─ pineapple_01_20250201.xlsx
│  │   ├─ pineapple_01_20250201_air.xlsx
│  │   ├─ pineapple_02_20250203.xlsx
│  │   └─ ...
│  │
│  └─ processed/          # （預留）清洗後或特徵化資料
│
├─ reports/
│  ├─ figures/            # 所有視覺化圖表（HTML）
│  │   ├─ raw_timeseries/        # 原始 raw 時序圖
│  │   ├─ delta_timeseries/      # 鳳梨 − air 差值時序圖
│  │   ├─ arduino_features/      # Arduino 預處理特徵圖（log ratio / MA5）
│  │   ├─ distributions/         # 感測器數值分布圖
│  │   └─ correlation/           # 感測器相關性矩陣
│  │
│  └─ summary/
│     ├─ summary_report.md       # 自動生成的文字分析摘要
│     └─ full_report.json        # 完整分析結果（供後續模型使用）
│
└─ Step1.ipynb            # Step 1 主程式（資料載入、清洗、EDA）
