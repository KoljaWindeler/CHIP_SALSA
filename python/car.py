import arduino_bridge

pin=4
value=122

arduino = arduino_bridge.connection()
arduino.setup_pwm_output(1)
arduino.setup_pwm_output(2)
arduino.setup_pwm_output(3)
arduino.setup_pwm_output(4)

arduino.setPWM(1,255)
arduino.setPWM(2,255)
arduino.setPWM(3,255)
arduino.setPWM(4,255)



print("GPIO pin "+str(pin)+" set duty cycle to "+str(value))
