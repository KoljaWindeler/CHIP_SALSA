import arduino_bridge

pin = 4

arduino = arduino_bridge.connection()
arduino.setup_analog_input(pin)
analog_value = arduino.readAnalog(pin)

print("GPIO pin "+str(pin)+" read as "+str(analog_value)+" ~ "+str(analog_value*5.0/1024)+"V")
