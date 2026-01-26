import pandas as pd
import numpy as np
import os

# =====================
# 自動切換到程式檔案所在資料夾
# =====================
script_dir = os.path.dirname(os.path.abspath(__file__))
os.chdir(script_dir)

# =====================
# 基本參數
# =====================
VREF = 5.0
ADC_MAX = 1023.0

RL_MQ135 = 10.0
RL_TGS2602 = 10.0
RL_MQ3 = 200.0

# =====================
# 1. 讀取資料
# =====================
file_name = "data.csv"
if not os.path.exists(file_name):
    print(f"❌ 錯誤：在 {script_dir} 找不到 {file_name}")
    exit()

df = pd.read_csv(file_name)
df.columns = df.columns.str.strip()

print("✅ 讀取成功！欄位如下：")
print(list(df.columns))

# =====================
# 2. ADC → Rs 轉換函式
# =====================
def adc_to_rs(adc, rl_kohm):
    adc = np.clip(adc, 1, 1022)  # 避免除零
    v = adc / ADC_MAX * VREF
    return rl_kohm * (VREF - v) / v

# =====================
# 3. 感測器原始資料轉換
# =====================
df["MQ2_v"] = df["MQ2_raw"] * (VREF / ADC_MAX)
df["MQ9_v"] = df["MQ9_raw"] * (VREF / ADC_MAX)

df["MQ3_Rs"] = adc_to_rs(df["MQ3_raw"], RL_MQ3)
df["MQ135_Rs"] = adc_to_rs(df["MQ135_raw"], RL_MQ135)
df["TGS2602_Rs"] = adc_to_rs(df["TGS2602_raw"], RL_TGS2602)

# =====================
# 4. 自動取前 60 筆資料平均作為 Baseline
# =====================
baseline_df = df.head(60)

MQ2_base_v = baseline_df["MQ2_v"].mean()
MQ9_base_v = baseline_df["MQ9_v"].mean()

MQ3_base_Rs = baseline_df["MQ3_Rs"].mean()
MQ135_base_Rs = baseline_df["MQ135_Rs"].mean()
TGS_base_Rs = baseline_df["TGS2602_Rs"].mean()

print("✅ 已計算前 60 筆平均作為 baseline")
print(f"MQ2_base_v: {MQ2_base_v:.3f}, MQ9_base_v: {MQ9_base_v:.3f}")
print(f"MQ3_base_Rs: {MQ3_base_Rs:.3f}, MQ135_base_Rs: {MQ135_base_Rs:.3f}, TGS_base_Rs: {TGS_base_Rs:.3f}")

# =====================
# 5. 刪掉前 60 筆資料（只保留測鳳梨或後續資料）
# =====================
df = df.iloc[60:].reset_index(drop=True)

# =====================
# 6. Ratio 計算（濃度上升 → ratio 正）
# =====================

# MQ2、MQ9 用電壓（電壓上升 → ratio 正）
df["MQ2_ratio"] = (df["MQ2_v"] - MQ2_base_v) / (MQ2_base_v + 1e-9)
df["MQ9_ratio"] = (df["MQ9_v"] - MQ9_base_v) / (MQ9_base_v + 1e-9)

# MQ3、MQ135、TGS2602 用 Rs，但反向計算：Rs 降低 → ratio 正
df["MQ3_ratio"] = (MQ3_base_Rs - df["MQ3_Rs"]) / (MQ3_base_Rs + 1e-9)
df["MQ135_ratio"] = (MQ135_base_Rs - df["MQ135_Rs"]) / (MQ135_base_Rs + 1e-9)
df["TGS2602_ratio"] = (TGS_base_Rs - df["TGS2602_Rs"]) / (TGS_base_Rs + 1e-9)

# =====================
# 7. 氣味距離（Euclidean distance）
# =====================
df["odor_distance"] = np.sqrt(
    df["MQ2_ratio"]**2 +
    df["MQ3_ratio"]**2 +
    df["MQ9_ratio"]**2 +
    df["MQ135_ratio"]**2 +
    df["TGS2602_ratio"]**2
)

# =====================
# 8. 時間平滑（讓圖變好看）
# =====================
df["odor_distance_smooth"] = df["odor_distance"].rolling(window=10, center=True).mean()

# =====================
# 9. 輸出結果
# =====================
output_file = "gas_all_processed_.csv"
df.to_csv(output_file, index=False)

print("-" * 40)
print("✅ 方法 A 處理完成！前 60 筆環境資料已刪除")
print(f"📁 輸出檔案：{os.path.join(script_dir, output_file)}")
