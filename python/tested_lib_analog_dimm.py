import arduino_bridge
import time

pin=1
value=100

arduino = arduino_bridge.connection()
arduino.setup_pwm_output(pin)
arduino.setPWM(pin,0)
for i in range(0,10):
	time.sleep(1)
	arduino.dimmTo(pin,value,10) #  dimming is in %
	time.sleep(1) # 100 steps, 10ms
	arduino.dimmTo(pin,0,10)
