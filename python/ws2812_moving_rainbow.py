import arduino_bridge
import time


# set pin 11 as 20 ws2812, all with unique colors
LC=24
ROUNDS=100
DELAY=0.02
pin = 6

arduino = arduino_bridge.connection()
arduino.setup_ws2812_unique_color_output(pin,LC)

# create a color array, rainbow for demonstration
colorArray = []
MAX=(255//LC)*LC
STEP=MAX//LC
for i in range(0,LC//3):
	colorArray.append(arduino_bridge.Color(MAX-STEP*i, STEP*i, 0))
for i in range(0,LC//3):
	colorArray.append(arduino_bridge.Color(0, MAX-STEP*i, STEP*i))
for i in range(0,LC//3):
	colorArray.append(arduino_bridge.Color(STEP*i, 0, MAX-STEP*i))

# send it
a=0
for ii in range(0,ROUNDS):
	print("Round "+str(ii))
	for iii in range(0,LC):
		# set the current array
		arduino.ws2812set(pin,colorArray)
		
		# rotate all colors by one
		temp = colorArray[0]
		for i in range(0,len(colorArray)-1):
			colorArray[i]=colorArray[i+1]
		colorArray[len(colorArray)-2]=temp
		
		# wait a little
		time.sleep(DELAY)
