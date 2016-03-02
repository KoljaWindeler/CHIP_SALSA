import arduino_bridge

pin = 8

arduino = arduino_bridge.connection()
arduino.setup_digital_input(pin)
digital_value = arduino.digitalRead(pin)

print("GPIO pin "+str(pin)+" read as "+str(digital_value)+" ~ "+str(digital_value*5.0)+"V")
