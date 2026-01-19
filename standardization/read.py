import pandas as pd
import numpy as np
import os


# 自動切換到程式檔案所在的資料夾
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
MQ3_M = -0.77
MQ3_B = 1.60
RATIO_MIN = 0.05
RATIO_MAX = 20.0
BG_SAMPLES = 60


# =====================
# 1. 讀取並清理資料
# =====================
file_name = "data.csv"
if not os.path.exists(file_name):
    print(f"❌ 錯誤：在 {script_dir} 找不到 {file_name}")
    print("請確認檔案是否真的放在這個資料夾內。")
    exit()


df = pd.read_csv(file_name)
df.columns = df.columns.str.strip() # 去除標題空格
print("✅ 讀取成功！偵測到欄位：", list(df.columns))


# =====================
# 2. 定義轉換函數
# =====================
def adc_to_rs(adc, rl_kohm):
    adc = np.clip(adc, 1, 1022)
    v = adc / ADC_MAX * VREF
    return rl_kohm * (VREF - v) / v


# =====================
# 3. 數據計算 (對應你的新欄位名稱)
# =====================
# A. 感測器轉換 (使用 _raw 結尾的欄位)
df["MQ2_v"] = df["MQ2_raw"] * (VREF / ADC_MAX)
df["MQ9_v"] = df["MQ9_raw"] * (VREF / ADC_MAX)
df["MQ3_Rs"] = adc_to_rs(df["MQ3_raw"], RL_MQ3)
df["MQ135_Rs"] = adc_to_rs(df["MQ135_raw"], RL_MQ135)
df["TGS2602_Rs"] = adc_to_rs(df["TGS2602_raw"], RL_TGS2602)


# B. 標記 Phase
df["phase"] = "MEAS"
df.loc[:BG_SAMPLES-1, "phase"] = "BG"
bg = df[df["phase"] == "BG"]


# 如果資料太少不足以做基準，就用全部資料當基準預防出錯
if len(bg) == 0: bg = df


# C. 計算基準值
mq2_mean, mq2_std = bg["MQ2_v"].mean(), bg["MQ2_v"].std(ddof=0)
mq9_mean, mq9_std = bg["MQ9_v"].mean(), bg["MQ9_v"].std(ddof=0)
R0_MQ3 = bg["MQ3_Rs"].mean()
MQ135_base = bg["MQ135_Rs"].mean()
TGS_base = bg["TGS2602_Rs"].mean()


# D. 變化率計算
df["MQ2_norm"] = (df["MQ2_v"] - mq2_mean) / (mq2_std + 1e-9)
df["MQ9_norm"] = (df["MQ9_v"] - mq9_mean) / (mq9_std + 1e-9)
df["MQ3_ppm"] = df["MQ3_Rs"].apply(lambda x: 10**((np.log10(x/(R0_MQ3+1e-9))-MQ3_B)/MQ3_M) if (x/(R0_MQ3+1e-9)) > RATIO_MIN else np.nan)
df["MQ135_ch_pct"] = (df["MQ135_Rs"] - MQ135_base) / (MQ135_base + 1e-9) * 100
df["TGS2602_ch_pct"] = (df["TGS2602_Rs"] - TGS_base) / (TGS_base + 1e-9) * 100


# E. 環境修正 (對應你的 Temp_C, Humidity_pct, Pressure_hPa)
df["Temp_corr"] = df["Temp_C"] - bg["Temp_C"].mean()
df["Humidity_corr"] = df["Humidity_pct"] - bg["Humidity_pct"].mean()
df["Pressure_corr"] = df["Pressure_hPa"] - bg["Pressure_hPa"].mean()


# =====================
# 4. 輸出
# =====================
meas = df[df["phase"] == "MEAS"].reset_index(drop=True)
output_file = "gas_all_processed.csv"
meas.to_csv(output_file, index=False)


print("-" * 30)
print(f"✅ 轉換完成！")
print(f"結果已儲存至: {os.path.join(script_dir, output_file)}")
