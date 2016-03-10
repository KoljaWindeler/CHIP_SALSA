import arduino_bridge
import time

salsa = arduino_bridge.connection()

#-----------------------------------------------------------------------------------------#
print("Lets do a fancy lightshow: Do you have a few ws2812 in a string?\
This demo will use 24, but it doesn't really matter if you have more or less connected.\
connect the power to the Vcc pin on the SALSA, GND to GND and the ws2812 Data in pin to P6")

in = "a"
while str.lower(in)!="y" and str.lower(in)!="s"  and str.lower(in)!="o":
	in = input("Ready? (yes = y, skip = s, use onboard ws2812 = o)")

if str.lower(in)=="y" or str.lower(in)!="o":
	LC=24
	ROUNDS=100
	DELAY=0.02
	pin = 6
	if str.lower(in)!="o":
		pin = 7

	salsa.setup_ws2812_unique_color_output(pin,LC)

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
	for ii in range(0,ROUNDS):
		#print("Round "+str(ii))
		for iii in range(0,LC):
			# set the current array
			salsa.ws2812set(pin,colorArray)
		
			# rotate all colors by one
			temp = colorArray[0]
			for i in range(0,len(colorArray)-1):
				colorArray[i]=colorArray[i+1]
			colorArray[len(colorArray)-2]=temp
		
			# wait a little
			time.sleep(DELAY)

	print("Neat, right?")

#-----------------------------------------------------------------------------------------#

print("Alright, lets do some dimming with the MOSFETs, connect a few LED's or\
what ever you have handy. Connect their '+' pin to a power supply with the required\
voltage (e.g. 12V). Connect the '-' pin to P1 on the SALSA. Also connect the '-' pin\
of your power supply to the GND pin on the salsa.")

in = "a"
while str.lower(in)!="y" and str.lower(in)!="s":
	in = input("Ready? (yes = y, skip = s)")

if str.lower(in)=="y":
	pin=1
	value=100
	salsa.setup_pwm_output(pin)
	salsa.setPWM(pin,0)
	for i in range(0,10):
		time.sleep(1)
		salsa.dimmTo(pin,value,10) #  dimming is in %
		time.sleep(1) # 100 steps, 10ms
		salsa.dimmTo(pin,0,10)

	print("That was dimming, lets move on")

#-----------------------------------------------------------------------------------------#

print("Next up: Analog reading. Do you have a poti? Connect one end to Vcc (lower left corner),\
the other end goes to gnd. The center pin shall be connected to P4!")

in = "a"
while in!="" and str.lower(in)!="y" and str.lower(in)!="s":
	in = input("Ready? (yes = y, skip = s)")

if str.lower(in)=="y":
	pin = 4
	salsa.setup_analog_input(pin)
	analog_value = salsa.analogRead(pin)
	print("GPIO pin "+str(pin)+" read as "+str(analog_value)+" ~ "+str(analog_value*5.0/1024)+"V")

#-----------------------------------------------------------------------------------------#


print("Well, thats about it. There are a few more examples in this directory.")
print("Happy making! :) JKW")