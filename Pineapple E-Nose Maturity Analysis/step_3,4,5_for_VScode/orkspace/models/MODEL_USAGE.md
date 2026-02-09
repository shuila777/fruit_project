# 方案2 混合投票模型使用說明

## 模型信息
- 模型名稱: 混合投票集成
- LOSO準確率: 76.1%
- 訓練日期: 2026-02-09 19:38:25
- 訓練樣本數: 454

## 載入模型

```python
import pickle
with open('models/final_ensemble_model.pkl', 'rb') as f:
    model = pickle.load(f)
```

## Top-5 關鍵特徵
1. all_sensors_mean
2. MQ135_TGS2602_ratio
3. MQ135_auc
4. MQ135_mean
5. MQ135_max

## 性能指標
- LOSO準確率: 76.1%
- Stage 0 Recall: 91.4%
- Stage 1 Recall: 81.5%
- Stage 2 Recall: 14.7%
- Stage 3 Recall: 92.9%
