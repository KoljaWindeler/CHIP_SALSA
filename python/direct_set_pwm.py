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
# setup pin 9 as pwm
pin = 9

bus.transaction(i2c.writing_bytes(address, START_BYTE, CMD_CONFIG, pin, MODE_PWM))


# set duty cycle to 50% = 128/255
pwm_value = 128
bus.transaction(i2c.writing_bytes(address, START_BYTE, CMD_SET, pin, pwm_value))

print("GPIO pin "+str(pin)+" set to "+str(pwm_value))
