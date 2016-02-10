import time
import arduino_bridge

# set pin 11 as 20 ws2812, all with unique colors
ledcount=18
rounds=10
delay=0.03
pin = 6

arduino = arduino_bridge.connection()
arduino.setup_ws2812_common_color_output(pin,ledcount)

# create a color array, rainbow for demonstration
colorArray = []
for i in range(0,256):
	colorArray.append(arduino_bridge.Color(255-i, i, 0))
for i in range(0,256):
	colorArray.append(arduino_bridge.Color(0, 255-i, i))
for i in range(0,256):
	colorArray.append(arduino_bridge.Color(i, 0, 255-i))

# send it
for ii in range(0,rounds*3*255):
	print("Round "+str(ii))
	# set the current array
	arduino.ws2812set(pin,colorArray[0])
		
	# rotate all colors by one
	temp = colorArray[0]
	for i in range(0,len(colorArray)-1):
		colorArray[i]=colorArray[i+1]
	colorArray[len(colorArray)-2]=temp
		
	# wait a little
	time.sleep(delay)
