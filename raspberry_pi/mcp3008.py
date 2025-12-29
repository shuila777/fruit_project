import time
import board
import busio
import digitalio
import adafruit_mcp3xxx.mcp3008 as MCP
from adafruit_mcp3xxx.analog_in import AnalogIn

spi = busio.SPI(clock=board.SCK, MISO=board.MISO, MOSI=board.MOSI)
cs = digitalio.DigitalInOut(board.D8)

mcp = MCP.MCP3008(spi, cs)

ch0 = AnalogIn(mcp, MCP.P0)
ch1 = AnalogIn(mcp, MCP.P1)
ch2 = AnalogIn(mcp, MCP.P2)
ch3 = AnalogIn(mcp, MCP.P3)

print("timestamp,ch0,ch1,ch2,ch3")

while True:
    print(f"{int(time.time())},{ch0.value},{ch1.value},{ch2.value},{ch3.value}")
    time.sleep(1)
