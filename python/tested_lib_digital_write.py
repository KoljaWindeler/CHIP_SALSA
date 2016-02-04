import time
import arduino_bridge

pin=3

arduino = arduino_bridge.connection()
arduino.setup_digital_output(pin)

for i in range(0,10):
	arduino.digitalWrite(pin,1)
	print("GPIO pin "+str(pin)+" set HIGH")
	time.sleep(1)
	arduino.digitalWrite(pin,0)
	print("GPIO pin "+str(pin)+" set LOW")
	time.sleep(1)
