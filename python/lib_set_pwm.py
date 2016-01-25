import arduino_bridge

pin=4
value=128

arduino = arduino_bridge.connection()
arduino.setup_digital_output(pin)

arduino.setPWM(pin,value)
print("GPIO pin "+str(pin)+" set duty cycle to "+str(value))
