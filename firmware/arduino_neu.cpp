#include "WS2812.h"	// thanks Matthias Riegler!
#include "PINS.h"
#include <Wire.h>

#define START_BYTE		0xCE
#define CMD_SET_SIMPLE 		0xF0
#define CMD_CONFIG		0xF1
#define CMD_GET			0xF2
#define CMD_RESET		0xF3

#define MODE_PWM			0x01
#define MODE_ANALOG_INPUT		0x02
#define MODE_SINGLE_COLOR_WS2812	0x03
#define MODE_MULTI_COLOR_WS2812		0x04
#define MODE_DIGITAL_INPUT		0x05
#define MODE_DIGITAL_OUTPUT		0x06
#define MAX_MODE			MODE_DIGITAL_OUTPUT


#define TRANSFER_TIMEOUT 	50
#define I2C_ADDRESS		0x04
#define MAX_CHANNEL_COUNT 	17

#define DEBUG			1
#define MAX_RESPONSE_LENGTH 	4

uint32_t	m_lasttransfer=0;
uint8_t		m_channel=0;
uint8_t		m_value=0;

uint8_t 	m_response_buffer[MAX_RESPONSE_LENGTH];
uint8_t 	m_response_length=0;
cRGB 		value;

PIN m_pins[MAX_CHANNEL_COUNT];

//========================= i2c config and init? =========================//
void setup(){
	// Activate Pin PB6 and PB7 input pull up and read i2c address adder value
	DDRB &= ~((1 << 6) & (1 << 7));
	PORTB |= (1 << 6) | (1 << 7); 
	delay(10); // 10ms to pull the pin up
	uint8_t i2c_adder = ((PINB ^ 0xff)>>6);

	// prepare
	//set(bool analog_in, bool digital, bool pwm, bool ws2812, uint8_t arduino_pin);
	m_pins[0].set(false, true, true, true, 10);
	m_pins[1].set(false, true, true, true, 9);
	m_pins[2].set(false, true, true, true, 6);
	m_pins[3].set(false, true, true, true, 5);
	m_pins[4].set(true, true, true, false, 15);
	m_pins[5].set(false, false, false, false, 0xff);
	m_pins[6].set(false, true, false, true, 2);
	m_pins[7].set(false, true, false, true, 11);
	m_pins[8].set(true, true, true, false, 16);
	m_pins[9].set(false, true, false, true, 3);
	m_pins[10].set(true, true, true, false, 17);
	m_pins[11].set(false, true, false, true, 8);
	m_pins[12].set(false, true, false, true, 7);

	m_pins[13].set(false, true, true, false, 12); // internal pin to motor driver
	m_pins[14].set(false, true, true, false, 13); // internal pin to motor driver
	m_pins[15].set(false, true, true, false, 4);  // internal pin to CHIP
	m_pins[16].set(false, true, true, false, 14); // internal pin to LED

	
	Wire.begin(I2C_ADDRESS+i2c_adder);                // join i2c bus with address #4,5,6 or 7
	Wire.onReceive(receiveEvent); // register event
	Wire.onRequest(requestEvent); // register event
	
	#ifdef DEBUG
	Serial.begin(9600);
	Serial.print("woop woop online at ID ");
	Serial.println(I2C_ADDRESS+i2c_adder);
	#endif
}
//========================= i2c config and init? =========================//
//========================= check for incoming  timeouts =========================//
void loop(){
	delay(TRANSFER_TIMEOUT);
}
//========================= check for incoming  timeouts =========================//
//========================= declare reset function @ address 0 =========================//
void(* resetFunc) (void) = 0; 
//========================= declare reset function @ address 0 =========================//
//========================= setup - on-the-fly =========================//
void config_pin(){
	m_response_buffer[0] = 0xff; // assume NACK
	m_response_length = 1;

	// PWM pins for dimming
	if(m_pins[m_channel].m_mode==MODE_PWM){
		if(m_pins[m_channel].is_pwm_out()){	
			m_response_buffer[0]=0; 
			
			#ifdef DEBUG
			Serial.print("PWM at pin ");
			Serial.print(m_channel);
			Serial.print(" that is ");
			Serial.println(m_pins[m_channel].m_arduino_pin);
			#endif

			pinMode(m_pins[m_channel].m_arduino_pin,OUTPUT);
		}
	} 

	// analog input 
	else if(m_pins[m_channel].m_mode==MODE_ANALOG_INPUT){
		if(m_pins[m_channel].is_analog_in()){
			
			#ifdef DEBUG
			Serial.print("analog in at pin ");
			Serial.print(m_channel);
			Serial.print(" that is ");
			Serial.println(m_pins[m_channel].m_arduino_pin);
			#endif

			m_response_buffer[0]=0; 
			pinMode(m_pins[m_channel].m_arduino_pin,INPUT);
		};
	} 

	// digital input 
	else if(m_pins[m_channel].m_mode==MODE_DIGITAL_INPUT){
		if(m_pins[m_channel].is_digital_inout()){	
			m_response_buffer[0]=0;
			pinMode(m_pins[m_channel].m_arduino_pin,INPUT);
		};
	} 

	// digital output
	else if(m_pins[m_channel].m_mode==MODE_DIGITAL_OUTPUT){
		if(m_pins[m_channel].is_digital_inout()){
			m_response_buffer[0]=0;
			pinMode(m_pins[m_channel].m_arduino_pin,OUTPUT);

			#ifdef DEBUG
			Serial.print("digital output at pin ");
			Serial.println(m_pins[m_channel].m_arduino_pin);
			#endif		

		};
	} 

	// WS2812 LEDs
	else if(m_pins[m_channel].m_mode==MODE_SINGLE_COLOR_WS2812 || m_pins[m_channel].m_mode==MODE_MULTI_COLOR_WS2812){
		if(m_pins[m_channel].is_ws2812()){
			m_response_buffer[0]=0;
			
			#ifdef DEBUG
			Serial.print("ws2812 at pin ");
			Serial.println(m_pins[m_channel].m_arduino_pin);
			#endif
			
			WS2812 *ws2812 = new WS2812(m_pins[m_channel].m_ws2812_count);
			ws2812->setOutput(m_pins[m_channel].m_arduino_pin); 
			m_pins[m_channel].m_ws2812_pointer=ws2812; // save pointer
		};
	}
}
//========================= setup - on-the-fly =========================//
//========================= i2c input statemachine =========================//
// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany){
	uint8_t msg[howMany];
	m_lasttransfer=millis();
	
	#ifdef DEBUG
	Serial.print("Data in: ");
	Serial.println(howMany);
	#endif

	// copy all bytes to have them available at once
	for(uint8_t i=0; Wire.available(); i++){ // loop through all
		msg[i]=Wire.read();
		
		#ifdef DEBUG
		Serial.print(msg[i],HEX);
		Serial.print(",");
		#endif
	}

	#ifdef DEBUG
	Serial.println("");
	#endif

	// only read it if there is a magic startbyte
	if(msg[0] == START_BYTE){
		/////////////////////////////  RESET CONFIGURATION /////////////////////////////
		if(msg[1] == CMD_RESET){
			resetFunc();  
		} 
		/////////////////////////////  RESET CONFIGURATION /////////////////////////////
		////////////////////////////  SIMPLE SETTING A VALUE ///////////////////////////
		else if(msg[1] == CMD_SET_SIMPLE){
			m_channel = msg[2];

			//## digital write for output or pull ups ##
			if(m_pins[m_channel].m_mode==MODE_DIGITAL_OUTPUT || m_pins[m_channel].m_mode==MODE_DIGITAL_INPUT || m_pins[m_channel].m_mode==MODE_ANALOG_INPUT){
				m_value = msg[3];

				#ifdef DEBUG
				Serial.print("digitalWrite ");
				Serial.print(m_value);
				Serial.print(" at pin ");
				Serial.println(m_pins[m_channel].m_arduino_pin);
				#endif		

				digitalWrite(m_pins[m_channel].m_arduino_pin,m_value);
			} 

			//## write a pwm signal ##
			else if(m_pins[m_channel].m_mode==MODE_PWM){ 
				m_value = msg[3];

				#ifdef DEBUG
				Serial.print("dutycycle of ");
				Serial.print(m_value);
				Serial.print(" at pin ");
				Serial.println(m_pins[m_channel].m_arduino_pin);
				#endif

				analogWrite(m_pins[m_channel].m_arduino_pin,m_value);
			}

			//## single color for all LEDs ##
			else if(m_pins[m_channel].m_mode==MODE_SINGLE_COLOR_WS2812){
				value.r = msg[3]; 
				value.g = msg[4]; 
				value.b = msg[5]; // RGB Value -> Blue

				#ifdef DEBUG
				Serial.print("single color for ");			
				Serial.print(m_pins[m_channel].m_ws2812_count);
				Serial.println(" leds");
				#endif

				for(uint8_t i=0; i<m_pins[m_channel].m_ws2812_count; i++){
					m_pins[m_channel].m_ws2812_pointer->set_crgb_at(i, value); // Set value at all LEDs
				}
				m_pins[m_channel].m_ws2812_pointer->sync(); // Sends the value to the LED
			} 

			//## multiple values, for each LED value ##
			else if(m_pins[m_channel].m_mode==MODE_MULTI_COLOR_WS2812){
				uint8_t m_current_led=msg[3];
				for(uint8_t i=4; i<howMany; i+=3){
					value.r = msg[i+0]; 
					value.g = msg[i+1]; 
					value.b = msg[i+2]; // RGB Value -> Blue
					m_pins[m_channel].m_ws2812_pointer->set_crgb_at(m_current_led, value); // Set value at LED found at RUNNING INDEX
					m_current_led++;
				}

				if(m_current_led>=m_pins[m_channel].m_ws2812_count){
					m_pins[m_channel].m_ws2812_pointer->sync(); // Sends the value to the LED only when all value are known
				}	
			}

		} 
		////////////////////////////  SIMPLE SETTING A VALUE ///////////////////////////
		///////////////////////////////  READING A VALUE ///////////////////////////////
		else if(msg[1] == CMD_GET){
			m_channel = msg[2];

			#ifdef DEBUG
			Serial.print("get value for arduino pin ");
			Serial.println(m_pins[m_channel].m_arduino_pin);
			#endif

			if(m_pins[m_channel].m_mode == MODE_DIGITAL_INPUT){
				m_response_buffer[0] = digitalRead(m_pins[m_channel].m_arduino_pin);
				m_response_length = 1;

				#ifdef DEBUG
				Serial.print("digital read value: ");
				Serial.println(m_response_buffer[0]);
				#endif

			} else 	if(m_pins[m_channel].m_mode == MODE_ANALOG_INPUT){
				uint16_t value=analogRead(m_pins[m_channel].m_arduino_pin);
				m_response_buffer[1] =  value >> 8;
				m_response_buffer[0] =  value & 0xff;
				m_response_length = 2;

				#ifdef DEBUG
				Serial.print("Value: ");
				Serial.println(value);
				Serial.print("Sending as ");
				Serial.print(m_response_buffer[1]);
				Serial.print("/");
				Serial.println(m_response_buffer[0]);
				#endif

			}

			#ifdef DEBUG
			Serial.print("Buffer: ");
			Serial.println(m_response_length);
			#endif

		} 
		///////////////////////////////  READING A VALUE ///////////////////////////////
		///////////////////////////////  CONFIGURE A PIN ///////////////////////////////
		else if(msg[1] == CMD_CONFIG){
			m_channel = msg[2];
			m_pins[m_channel].m_mode = msg[3];

			if( m_pins[m_channel].m_mode == MODE_SINGLE_COLOR_WS2812 ||  m_pins[m_channel].m_mode == MODE_MULTI_COLOR_WS2812 ){
				m_pins[m_channel].m_ws2812_count = msg[4]; 
			}
			config_pin();
		}
		///////////////////////////////  CONFIGURE A PIN ///////////////////////////////
	}

	#ifdef DEBUG
	Serial.println("end");
	#endif	
};

//========================= i2c input statemachine =========================//
//========================= i2c response =========================//
// the arduino wire lib is clearing the i2c buffer whenever it receives the SLAVE READ from the I2C Master
// therefore we can NOT preset the tx buffer, but have to do it after the readcall.
// this readcall triggers the onRequestService, to whom we've subscribed. So filling the buffer now should work
// remember to set the MAX_RESPONSE_LENGTH according to your answer. 
void requestEvent(){

	#ifdef DEBUG
	Serial.println("Request event");
	#endif

	if(m_response_length && m_response_length<MAX_RESPONSE_LENGTH){

		#ifdef DEBUG
		Serial.println("writing");
		for(int i=0; i<m_response_length;i++){
			Serial.print(m_response_buffer[i]);
		}
		#endif

		Wire.write(m_response_buffer,m_response_length);
		m_response_length = 0;
	}
}
//========================= i2c response =========================//
