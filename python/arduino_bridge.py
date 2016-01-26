import time
try:
	import quick2wire.i2c as i2c
except:
	print("Could not load the quickwire i2c lib, stopping")
	exit(0)

# upper line:			 5V,  		0 PWM,				1 PWM,					2 PWM,							3 PWM,							GND
# lower line			5V,  		4 ADC,digital,	5 CHIP pinP7,		6 digital, ws2812,		7 digital, ws2812,		GND
# small smd pad  top next to avr:				Pin 8, digital, ws2812
# big smd pad  top next to terminal: 			Pin 9, digital, ws2812
# big smd pad  bottom next to terminal: 	Pin10, digital, ws2812
#######################################################################################################
#######################################################################################################
class Color:
	def __init__(self,red,green, blue):
		self.red=red
		self.green=green
		self.blue=blue
#######################################################################################################
#######################################################################################################	
	
class connection:
	# some constants
	START_BYTE 		= 0xCE
	CMD_SET  			= 0xF0
	CMD_CONFIG	= 0xF1
	CMD_GET			= 0xF2
	CMD_RESET		= 0xF3

	MODE_PWM									= 0x01
	MODE_ANALOG_INPUT	 				= 0x02
	MODE_SINGLE_COLOR_WS2812	= 0x03
	MODE_MULTI_COLOR_WS2812	= 0x04
	MODE_DIGITAL_INPUT	 				= 0x05
	MODE_DIGITAL_OUTPUT      			= 0x06

	
######################################## constructor ##################################################
	def __init__(self, bus="", address="", warnings=1):
		if(bus==""):	
			# create connection
			self.bus = i2c.I2CMaster(1)
		else:
			self.bus = bus
		if(address==""):
			self.address = 0x04
		else:
			self.address = address
		self.warnings=warnings
		self.reset_config() # reset controller and vars
################################# prepare configuration and reset controller ##########################
	def reset_config(self):
		# reset avr and give some time for reboot
		self.bus.transaction(i2c.writing_bytes(self.address, self.START_BYTE, self.CMD_RESET))
		time.sleep(1)
		self.modes = [None] * 13
		self.ws2812count = [0] * 13
		return 0
################################# print warnings ######################################################
	def warn(self,text):
		if(self.warnings):
			print("Arduino bridge warning: "+text)
#######################################################################################################
######################################## SETUP #########################################################
################################### digital output ####################################################
	def setup_digital_output(self,pin):
		if(pin!=4 and pin<6 and pin>10):
			self.warn("Digital output only supported on pins 4, 6-10")
			return -1
			
		self.bus.transaction(i2c.writing_bytes(self.address, self.START_BYTE, self.CMD_CONFIG, pin, self.MODE_DIGITAL_OUTPUT))	
		self.modes[pin]=self.MODE_DIGITAL_OUTPUT
		return 0
################################### pwm output ########################################################
	def setup_pwm_output(self,pin):
		# only pins 0-3
		if(pin>4):#TODO new board will have 0-3, old has 1-4
			self.warn("PWM output only supported on pins 0,1,2,3")
			return -1
			
		self.bus.transaction(i2c.writing_bytes(self.address, self.START_BYTE, self.CMD_CONFIG, pin, self.MODE_PWM))	
		self.modes[pin]=self.MODE_PWM
		return 0
################################## ws2812 control #####################################################
	def setup_ws2812_common_color_output(self,pin,count):
		return self.setup_ws2812_output(pin,count,self.MODE_SINGLE_COLOR_WS2812)
	def setup_ws2812_unique_color_output(self,pin,count,mode=MODE_SINGLE_COLOR_WS2812):
		return self.setup_ws2812_output(pin,count,self.MODE_MULTI_COLOR_WS2812)
	def setup_ws2812_output(self,pin,count,mode=MODE_SINGLE_COLOR_WS2812):
		# only pins 6-10
		if(pin<6 and pin>10):
			self.warn("ws2812 only supported on pins 6-10")
			return -1
		if(mode!=self.MODE_SINGLE_COLOR_WS2812 and mode!=self.MODE_MULTI_COLOR_WS2812):
			self.warn("invalid mode for ws2812 output")
			return -1
		if(count<=0):
			return -1
			
		self.bus.transaction(i2c.writing_bytes(self.address, self.START_BYTE, self.CMD_CONFIG,  pin, mode, count))
		self.modes[pin]=mode
		self.ws2812count[pin]=count
		return 0
######################################## digital input ################################################
	def setup_digital_input(self,pin):
		# only pins 4,6,7
		if(pin!=4 and pin<6 and pin>10):
			self.warn("Digital output only supported on pins 4 and 6-10")
			return -1
			
		self.bus.transaction(i2c.writing_bytes(self.address, self.START_BYTE, self.CMD_CONFIG, pin, self.MODE_DIGITAL_INPUT))	
		self.modes[pin]=self.MODE_DIGITAL_INPUT
		return 0
######################################### analog input ################################################
	def setup_analog_input(self,pin):
		# only pins 4
		if(pin!=4 and pin!=8 and pin!=9):
			self.warn("Analog input only supported on pins 4,8 and 9")
			return -1
			
		self.bus.transaction(i2c.writing_bytes(self.address, self.START_BYTE, self.CMD_CONFIG, pin, self.MODE_ANALOG_INPUT))	
		self.modes[pin]=self.MODE_ANALOG_INPUT
		return 0
#######################################################################################################
######################################## SETUP #########################################################
#######################################################################################################


#######################################################################################################
######################################## USAGE ########################################################
####################################### set pin high or low ##############################################
	def digitalWrite(self,pin,value):
		if(self.modes[pin]!=self.MODE_PWM and self.modes[pin]!=self.MODE_ANALOG_INPUT and self.modes[pin]!=self.MODE_DIGITAL_OUTPUT):
			self.warn("pin "+str(pin)+" not configured for digital output, pwm output or analog input")
			self.warn("Please configure the pin accordingly")
			return -1
			
		if(value!=0 and value!=1):
			self.warn("Digital write only accepts input value 0 or 1")
			return -1
			
		if(self.modes[pin]==self.MODE_PWM):
			self.bus.transaction(i2c.writing_bytes(self.address, self.START_BYTE, self.CMD_SET, pin, 255*value))
		else:
			self.bus.transaction(i2c.writing_bytes(self.address, self.START_BYTE, self.CMD_SET, pin, value))
		return 0
##################################### write analog value ##############################################
	def analogWrite(self,pin,value):
		return self.setPWM(pin,value)
	def setPWM(self,pin,value):
		if(self.modes[pin]!=self.MODE_PWM):
			self.warn("pin "+str(pin)+" not configured for pwm output")
			self.warn("Please configure the pin accordingly")
			return -1
		if(value<0 or value>255):
			self.warn("only values between 0 and 255 are valid")
			return -1
		self.bus.transaction(i2c.writing_bytes(self.address, self.START_BYTE, self.CMD_SET, pin, value))
		return 0
##################################### read digital value ##############################################
	def digitalRead(self,pin):
		if(self.modes[pin]!=self.MODE_DIGITAL_INPUT and self.modes[pin]!=self.MODE_ANALOG_INPUT ):
			self.warn("pin "+str(pin)+" not configured for digital input")
			self.warn("Please configure the pin accordingly")
			return -1
		if(self.modes[pin]==self.MODE_DIGITAL_INPUT):
			return self.bus.transaction(i2c.writing_bytes(self.address, self.START_BYTE,self.CMD_GET,pin), i2c.reading(self.address, 1))[0][0]
		else:
			if(analogRead(pin,avoid_mode_warning=1)>100): # >0.5V
				return 1
			else:
				return 0
#################################### read analog value ################################################
	def analogRead(self, pin, avoid_mode_warning=0):
		if(self.modes[pin]!=self.MODE_ANALOG_INPUT and avoid_mode_warning==0):
			self.warn("pin "+str(pin)+" not configured for analog input")
			self.warn("Please configure the pin accordingly")
			return -1
			
		analog_high, analog_low = self.bus.transaction(i2c.writing_bytes(self.address, self.START_BYTE,self.CMD_GET,pin), i2c.reading(self.address, 2))[0]
		analog_value = analog_high<<8 | analog_low;
		return analog_value
################################### set ws2812 value ##################################################
	def ws2812set(self,pin,colors):
		if(self.modes[pin]!=self.MODE_MULTI_COLOR_WS2812 and self.modes[pin]!=self.MODE_SINGLE_COLOR_WS2812):
			self.warn("pin "+str(pin)+" not configured for ws2812 output")
			self.warn("Please configure the pin accordingly")
			return -1
			
		if(self.modes[pin]==self.MODE_SINGLE_COLOR_WS2812):
			if(not(isinstance(colors, Color))):
				self.warn("color argument is not of type 'Color'")
				return -1
			self.bus.transaction(i2c.writing_bytes(self.address, self.START_BYTE, self.CMD_SET, pin, colors.red, colors.green, colors.blue))
			
		else:
			type_check=0
			if(isinstance(colors, list)):
				if(isinstance(colors[0], Color)):
					type_check=1
			if(not(type_check)):
				self.warn("color argument is not a list of Color")
				return -1
			if(len(colors)!=self.ws2812count[pin]):
				self.warn("you submitted "+str(len(colors))+" Colors, but "+str(self.ws2812count[pin])+" are required")
				return -1
			# ok, lets transmit, we have to send blocks of 8 LEDs or less
			for ii in range(0,self.ws2812count[pin]//8+1):
				msg = []
				msg.append(self.START_BYTE)
				msg.append(self.CMD_SET)
				msg.append(pin)
				msg.append(ii*8)
				for i in range(0,min(self.ws2812count[pin]-ii*8,8)):
					msg.append(colors[i].red)
					msg.append(colors[i].green)
					msg.append(colors[i].blue)
				self.bus.transaction(i2c.writing(self.address,msg))
		return 0
#######################################################################################################
######################################## USAGE ########################################################
#######################################################################################################

