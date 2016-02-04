import arduino_bridge

pin = 8

arduino = arduino_bridge.connection()
arduino.setup_analog_input(pin)
analog_value = arduino.analogRead(pin)

print("GPIO pin "+str(pin)+" read as "+str(analog_value)+" ~ "+str(analog_value*5.0/1024)+"V")
