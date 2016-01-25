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
# set pin as 20 ws2812, all with unique colors
LC=20
COLORS=256
ROUNDS=1
DELAY=0.02

bus.transaction(i2c.writing_bytes(address, START_BYTE, CMD_CONFIG,  11, MODE_MULTI_COLOR_WS2812, LC))
# create a color array, rainbow for demonstration
c = []
MAX=255
STEP=1
for i in range(0,COLORS):
	d=[]
	d.append(MAX-STEP*i)
	d.append(STEP*i)
	d.append(0)
	c.append(d)
for i in range(0,COLORS):
	d=[]
	d.append(0)
	d.append(MAX-STEP*i)
	d.append(STEP*i)
	c.append(d)
for i in range(0,COLORS):
	d=[]
	d.append(STEP*i)
	d.append(0)
	d.append(MAX-STEP*i)
	c.append(d)


# send it as packs of 8 LEDs at the time, the arduino can't handle more without modification (32byte I2C buffer)
AL=0
DIR=1
a=0
for iii in range(0,3*COLORS*ROUNDS):
	print("Round "+str(iii))
	L=0
	for ii in range(0,LC//8+1):
		msg = []
		msg.append(START_BYTE)
		msg.append(CMD_SET)
		msg.append(11)
		msg.append(ii*8)
		for i in range(0,min(LC-ii*8,8)):
			if(L==AL):
				msg.append(c[a][0])
				msg.append(c[a][1])
				msg.append(c[a][2])
			else:
				msg.append(0)
				msg.append(0)
				msg.append(0)
			a=(a+1)%COLORS
			L=(L+1)%LC
		bus.transaction(i2c.writing(address,msg))
	time.sleep(0.05)
	AL=AL+DIR
	if(AL>=LC):
		DIR=-1
	elif(AL<=0):
		DIR=1
	#######################################################################################################
