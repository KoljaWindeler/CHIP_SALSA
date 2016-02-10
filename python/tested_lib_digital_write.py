import time
import arduino_bridge

pin=7

arduino = arduino_bridge.connection()
arduino.setup_digital_output(pin)

for i in range(0,10):
	arduino.digitalWrite(pin,1)
	print("GPIO pin "+str(pin)+" set HIGH")
	time.sleep(0.5)
	arduino.digitalWrite(pin,0)
	print("GPIO pin "+str(pin)+" set LOW")
	time.sleep(0.5)
