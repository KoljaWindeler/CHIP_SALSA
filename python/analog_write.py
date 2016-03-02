import arduino_bridge
import time

pin=1
value=122

arduino = arduino_bridge.connection()
arduino.setup_pwm_output(pin)

for i in range(0,25):
	arduino.setPWM(pin,i*10)
	time.sleep(0.05)
	print("GPIO pin "+str(pin)+" set duty cycle to "+str(i))
