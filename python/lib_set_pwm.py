import arduino_bridge

pin=4
value=122

arduino = arduino_bridge.connection()
arduino.setup_pwm_output(pin)

arduino.setPWM(pin,value)
print("GPIO pin "+str(pin)+" set duty cycle to "+str(value))
