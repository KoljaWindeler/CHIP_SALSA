import arduino_bridge

pin=4

arduino = arduino_bridge.connection()
arduino.setup_digital_output(pin)

arduino.digitalWrite(pin,1)
print("GPIO pin "+str(pin)+" set HIGH")
