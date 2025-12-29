#include <SPI.h>

const int CS_PIN = 53;   // Mega 的 SPI CS 腳位

// 讀取 MCP3008 指定 channel (0~7)
int readMCP3008(int channel) {
  if (channel < 0 || channel > 7) return -1;

  digitalWrite(CS_PIN, LOW);

  // MCP3008 通訊格式
  byte command1 = 0b00000001;                 // Start bit
  byte command2 = 0b10000000 | (channel << 4); // Single-ended + channel
  byte command3 = 0b00000000;

  SPI.transfer(command1);
  byte highByte = SPI.transfer(command2);
  byte lowByte  = SPI.transfer(command3);

  digitalWrite(CS_PIN, HIGH);

  int value = ((highByte & 0x03) << 8) | lowByte;
  return value;
}

void setup() {
  Serial.begin(9600);

  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);

  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV16); // 穩定一點
  SPI.setDataMode(SPI_MODE0);

  Serial.println("timestamp_ms,CH0,CH1,CH2,CH3");
}

void loop() {
  unsigned long timestamp = millis();

  int ch0 = readMCP3008(0);
  int ch1 = readMCP3008(1);
  int ch2 = readMCP3008(2);
  int ch3 = readMCP3008(3);

  Serial.print(timestamp);
  Serial.print(",");
  Serial.print(ch0);
  Serial.print(",");
  Serial.print(ch1);
  Serial.print(",");
  Serial.print(ch2);
  Serial.print(",");
  Serial.println(ch3);

  delay(1000);
}
