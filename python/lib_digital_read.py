import arduino_bridge

pin=4

arduino = arduino_bridge.connection()
arduino.setup_digital_input(pin)

print("GPIO pin "+str(pin)+" read as "+str(arduino.digitalRead(pin)))
