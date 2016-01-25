import quick2wire.i2c as i2c
import time

# some constants
START_BYTE 	= 0xCE
CMD_SET  	= 0xF0
CMD_CONFIG	= 0xF1
CMD_READ	= 0xF2
CMD_RESET	= 0xF3

MODE_PWM		 = 0x01
MODE_ANALOG_INPUT	 = 0x02
MODE_SINGLE_COLOR_WS2812 = 0x03
MODE_MULTI_COLOR_WS2812	 = 0x04
MODE_DIGITAL_INPUT	 = 0x05
MODE_DIGITAL_OUTPUT      = 0x06


address = 0x04

# create connection
bus = i2c.I2CMaster(1)


#######################################################################################################
# setup pin 9 as digital read pin, just returning 0-255 for 0-5V
pin = 9

bus.transaction(i2c.writing_bytes(address, START_BYTE, CMD_CONFIG, pin, MODE_ANALOG_INPUT))
# read two bytes as the value can be > 256
analog_high, analog_low = bus.transaction(i2c.writing_bytes(address, START_BYTE,CMD_GET,pin), i2c.reading(address, 2))[0]
# calculate value
analog_value = analog_high<<8 | analog_low;


print("GPIO pin "+str(pin)+" read as "+str(analog_value)+" ~ "+str(analog_value*5.0/1024)+"V")
