import quick2wire.i2c as i2c
import time

# some constants
START_BYTE 	= 0xCE
CMD_SET  	= 0xF0
CMD_CONFIG	= 0xF1
CMD_READ	= 0xF2
CMD_RESET	= 0xF3

MODE_PWM		 = 0x01
MODE_INPUT		 = 0x02
MODE_SINGLE_COLOR_WS2812 = 0x03
MODE_MULTI_COLOR_WS2812	 = 0x04
address = 0x04

# create connection
bus = i2c.I2CMaster(1)

#######################################################################################################
# setup pin 4 as pwm output, we don't care about color here, just set it to 80/256 dutycycle
#bus.transaction(i2c.writing_bytes(address, START_BYTE, CMD_CONFIG, 0, MODE_PWM))
#for ii in range(0,256):
#	bus.transaction(i2c.writing_bytes(address, START_BYTE, CMD_SET, 0, ii))
#	print("dimming on at "+str(ii))
#	time.sleep(0.5)
#######################################################################################################

#######################################################################################################
# setup pin 9 as 5 ws2812 leds, all with the same color
# and set this color to r,g,b = 255,0,0  -> super red :)
bus.transaction(i2c.writing_bytes(address, START_BYTE, CMD_CONFIG, 9, MODE_SINGLE_COLOR_WS2812, 5))
bus.transaction(i2c.writing_bytes(address, START_BYTE, CMD_SET, 9, 255,0,0))
#######################################################################################################

#######################################################################################################
# set pin as 20 ws2812, all with unique colors
bus.transaction(i2c.writing_bytes(address, START_BYTE, CMD_CONFIG,  11, MODE_MULTI_COLOR_WS2812, 20))
# create a color array, rainbow for demonstration
c = []
for ii in range(0,2):
	for i in range(0,5):
		d=[]
		d.append(255-25*(i+2*ii))
		d.append(25*(i+2*ii))
		d.append(0)
		c.append(d)
for ii in range(0,2):
	for i in range(0,5):
		d=[]
		d.append(0)
		d.append(255-25*(i+2*ii))
		d.append(25*(i+2*ii))
		c.append(d)

# send it as packs of 8 LEDs at the time, the arduino can't handle more without modification (32byte I2C buffer)
a=0
for ii in range(0,3):
	msg = []
	msg.append(START_BYTE)
	msg.append(CMD_SET)
	msg.append(11)
	msg.append(ii*8)
	for i in range(0,min(20-ii*8,8)):
		msg.append(c[a][0])
		msg.append(c[a][1])
		msg.append(c[a][2])
		a=a+1
	bus.transaction(i2c.writing(address,msg))
#######################################################################################################

#######################################################################################################
# configure pin 10 with just 5 individual LEDs and send some strange colors
time.sleep(1)
bus.transaction(i2c.writing_bytes(address, START_BYTE, CMD_CONFIG, 10, MODE_MULTI_COLOR_WS2812, 5))
msg = []
msg.append(START_BYTE)
msg.append(CMD_SET)
msg.append(10)
msg.append(0)
for i in range(0,5):
	msg.append(((i+1)%3)*127)
	msg.append((i%2)*255)
	msg.append((i%3)*127)
bus.transaction(i2c.writing(address,msg))
time.sleep(1)
#######################################################################################################
