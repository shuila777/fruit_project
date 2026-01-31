#include <SPI.h>

const int CS_PIN = 53;

int readMCP3008(int channel) {
  if (channel < 0 || channel > 7) return -1;

  digitalWrite(CS_PIN, LOW);
  SPI.transfer(0x01);
  byte highByte = SPI.transfer(0x80 | (channel << 4));
  byte lowByte  = SPI.transfer(0x00);
  digitalWrite(CS_PIN, HIGH);

  return ((highByte & 0x03) << 8) | lowByte;
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);
  
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV16);
  SPI.setDataMode(SPI_MODE0);
  
  Serial.println("====================================");
  Serial.println("🔧 MCP3008 診斷程式");
  Serial.println("====================================");
  Serial.println("");
  Serial.println("📋 測試步驟：");
  Serial.println("1. 先觀察所有通道的數值");
  Serial.println("2. 把 CH0 (Pin 1) 接到 5V");
  Serial.println("3. 看 CH0 會不會變成 1023");
  Serial.println("");
  Serial.println("開始測試...");
  Serial.println("====================================");
  delay(2000);
}

void loop() {
  Serial.println("");
  Serial.println("--- 讀取所有通道 ---");
  
  for (int ch = 0; ch < 8; ch++) {
    int value = readMCP3008(ch);
    
    Serial.print("CH");
    Serial.print(ch);
    Serial.print(": ");
    Serial.print(value);
    Serial.print(" (");
    
    float voltage = (value / 1023.0) * 5.0;
    Serial.print(voltage, 2);
    Serial.print("V)");
    
    if (value == 0) {
      Serial.print(" ← ⚠️ 可能沒接或浮空");
    } else if (value >= 1020) {
      Serial.print(" ← ✅ 接近 5V（正常）");
    } else if (value > 50) {
      Serial.print(" ← 🟢 有訊號");
    }
    
    Serial.println();
  }
  
  Serial.println("====================================");
  delay(2000);
}