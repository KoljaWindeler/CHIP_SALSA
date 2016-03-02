import time
import arduino_bridge


arduino = arduino_bridge.connection()
arduino.setup_digital_output(0)
arduino.setup_digital_output(1)
arduino.setup_digital_output(2)
arduino.setup_digital_output(3)
arduino.setup_digital_output(4)
arduino.setup_digital_output(6)
arduino.setup_digital_output(7)

for i in range(0,10000):
	arduino.digitalWrite(0,1)
	arduino.digitalWrite(1,1)
	arduino.digitalWrite(2,1)
	arduino.digitalWrite(3,1)
	arduino.digitalWrite(4,1)
	arduino.digitalWrite(6,1)
	arduino.digitalWrite(7,1)
	print("GPIO pins set HIGH")
	time.sleep(0.5)
	arduino.digitalWrite(0,0)
	arduino.digitalWrite(1,0)
	arduino.digitalWrite(2,0)
	arduino.digitalWrite(3,0)
	arduino.digitalWrite(4,0)
	arduino.digitalWrite(6,0)
	arduino.digitalWrite(7,0)
	print("GPIO pins set LOW")
	time.sleep(0.5)
