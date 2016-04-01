#include "WS2812.h"	// thanks Matthias Riegler!
#include "LowPower.h" // thanks LowPowerLab https://github.com/LowPowerLab/LowPower

#include "PINS.h"
#include <Wire.h>

#define START_BYTE		0xCE
#define CMD_SET_SIMPLE 		0xF0
#define CMD_CONFIG		0xF1
#define CMD_GET			0xF2
#define CMD_RESET		0xF3
#define CMD_DIMM		0xF4
#define CMD_PWM_FREQ		0xF5
#define CMD_TRIGGER_AFTER_SLEEP 0xF6

#define MODE_PWM			0x01
#define MODE_ANALOG_INPUT		0x02
#define MODE_SINGLE_COLOR_WS2812	0x03
#define MODE_MULTI_COLOR_WS2812		0x04
#define MODE_DIGITAL_INPUT		0x05
#define MODE_DIGITAL_OUTPUT		0x06
#define MAX_MODE			MODE_DIGITAL_OUTPUT
// non user mode
#define MODE_PWM_DIMMING 	0xF1


#define TRANSFER_TIMEOUT 	50
#define I2C_ADDRESS		0x04
#define MAX_CHANNEL_COUNT 	17

//#define DEBUG			1		// if you enabled this: remember to slow your request down! serial.print is sloooow
#define MAX_RESPONSE_LENGTH 	4

uint8_t intens[100] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,27,28,29,30,31,32,33,34,35,36,38,39,40,41,43,44,45,47,48,50,51,53,55,57,58,60,62,64,66,68,70,73,75,77,80,82,85,88,91,93,96,99,103,106,109,113,116,120,124,128,132,136,140,145,150,154,159,164,170,175,181,186,192,198,205,211,218,225,232,239,247,255};


uint8_t		m_channel=0;
uint8_t 	m_receive_buffer[32];
uint8_t 	m_receive_length=0;
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
	//set( 		ana,  digi,  pwm,   ws,   Arduino_pin);
	// screw outputs
	m_pins[0].set( false, true,  true,  true,  10); 	// PB2, P0 PWM or left motor driver (P0/1) En signal
	m_pins[1].set( false, true,  true,  true,  9); 		// PB1, P1 PWM
	m_pins[2].set( false, true,  true,  true,  6); 		// PD6, P2 PWM
	m_pins[3].set( false, true,  true,  true,  5); 		// PD5, P3 PWM
	m_pins[4].set( true,  true,  false, false, 15); 	// PC1, P4 analog input
	m_pins[5].set( false, false, false, false, 0xff); 	//      P5 direct to CHIP P7 pin
	m_pins[6].set( false, true,  false, true,  2); 		// PD2, P6 digital and ws2812 pin
	m_pins[7].set( false, true,  true,  true,  11); 	// PB3, P7 and to onboard ws2812 and to general LED

	// some pins spread over the board
	m_pins[8].set( true,  true,  false, false, 16); 	// PC2, PAD8 and jumper to powerdown
	m_pins[9].set( false, true,  true,  true,  3); 		// PD3, PAD9 and right motor driver (P2/3) En signal
	m_pins[10].set(true,  true,  false, false, 17); 	// PC3, PAD10
	m_pins[11].set(false, true,  false, true,  8); 		// PB0, PAD11
	m_pins[12].set(false, true,  false, true,  7); 		// PD7, PAD12

	// internal pins
	m_pins[13].set(false, true,  false, false, 12); 	// PB4, internal pin to motor driver, direction pins, right side (P2/3), function limited
	m_pins[14].set(false, true,  false, false, 13); 	// PB5, internal pin to motor driver,  direction pins, left side (P0/1), function limited
	m_pins[15].set(false, true,  false, false, 4);  	// PD4, internal pin to CHIP, e.g. interrupts
	m_pins[16].set(false, true,  false, false, 14); 	// PC0, internal pin, to optional switch

	
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
	for(uint16_t i=0; i<MAX_CHANNEL_COUNT; i++){
		if(m_pins[i].m_mode == MODE_PWM_DIMMING){
			if(m_pins[i].m_next_action <= millis()){
				if(m_pins[i].m_value < m_pins[i].m_target_value){
					m_pins[i].m_value++;
				} else if(m_pins[i].m_value > m_pins[i].m_target_value){
					m_pins[i].m_value--;
				}

				#ifdef DEBUG
				Serial.print("Dimming pin: ");
				Serial.print(m_pins[m_channel].m_arduino_pin);
				Serial.print(" set value ");
				Serial.println(intens[m_pins[m_channel].m_value]);
				#endif
				
				// write value to pin
				analogWrite(m_pins[i].m_arduino_pin, intens[m_pins[i].m_value]);
				
				if(m_pins[i].m_value == m_pins[i].m_target_value){
					m_pins[i].m_mode = MODE_PWM;
				} else {
					m_pins[i].m_next_action += m_pins[i].m_dimm_interval;
				};
			};	
		};
	};

	if(m_receive_length){
		parse();
		m_receive_length=0;
	};
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
			Serial.println(m_pins[m_channel].m_arduino_pin,DEC);
			#endif

			pinMode(m_pins[m_channel].m_arduino_pin,OUTPUT);
		}
	} 

	// analog input 
	else if(m_pins[m_channel].m_mode==MODE_ANALOG_INPUT){
		if(m_pins[m_channel].is_analog_in()){
			m_response_buffer[0]=0; 
			
			#ifdef DEBUG
			Serial.print("analog in at pin ");
			Serial.print(m_channel);
			Serial.print(" that is ");
			Serial.println(m_pins[m_channel].m_arduino_pin,DEC);
			#endif
			
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
			Serial.println(m_pins[m_channel].m_arduino_pin,DEC);
			#endif		

		};
	} 

	// WS2812 LEDs
	else if(m_pins[m_channel].m_mode==MODE_SINGLE_COLOR_WS2812 || m_pins[m_channel].m_mode==MODE_MULTI_COLOR_WS2812){
		if(m_pins[m_channel].is_ws2812()){
			m_response_buffer[0]=0;
			
			#ifdef DEBUG
			Serial.print("ws2812 at pin ");
			Serial.println(m_pins[m_channel].m_arduino_pin,DEC);
			#endif
			
			WS2812 *ws2812 = new WS2812(m_pins[m_channel].m_ws2812_count);
			ws2812->setOutput(m_pins[m_channel].m_arduino_pin); 
			m_pins[m_channel].m_ws2812_pointer=ws2812; // save pointer
		};
	}
	
	// un-configure pin-mode if the mode was rejected
	if(m_response_buffer[0]!=0){
		m_pins[m_channel].m_mode = 0xff;
	}
}
//========================= setup - on-the-fly =========================//
//========================= setup pwm - on-the-fly =========================//
/*
# timer 0
#[2]: 31250, 3906, 488, 122, 30,  // 1,8,64,256,1024 // untested pwm channel p2
#[3]: 31250, 3906, 488, 122, 30,  // 1,8,64,256,1024 // untested pwm channel p3

# timer 1
#[1]: 15625, 1953, 244, 61, 15 // 1,2,3,4,5 // untested pwm channel p1
#[0]: 15625, 1953, 244, 61, 15 // 1,2,3,4,5 // untested pwm channel p0

# timer 2
#[7]: 15625, 1953, 488, 244, 122, 61, 15 // 1,8,32,64,128,256,1024 // 1,2,3,4,5,6,7 // motor left tested
#[9]: 15625, 1953, 488, 244, 122, 61, 15 // 1,8,32,64,128,256,1024 // 1,2,3,4,5,6,7 // motor right tested
*/
void setPwmFrequency(int pin, int divisor) {
	byte mode;
	if(pin == 2 || pin == 3) {
		TCCR0B = TCCR0B & 0b11111000 | divisor;
	} else if (pin == 0 || pin == 1){
		TCCR1B = TCCR1B & 0b11111000 | divisor;
	} else if(pin == 7 || pin == 9) {
		TCCR2B = TCCR2B & 0b11111000 | divisor;
	}
}
//========================= setup pwm - on-the-fly =========================//
//========================= i2c input statemachine =========================//
// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int msgLength){
	
	
	#ifdef DEBUG
	Serial.print(msgLength);
	Serial.println(" byte in");
	#endif

	// copy all bytes to have them available at once
	for(uint8_t i=0; Wire.available(); i++){ // loop through all
		m_receive_buffer[i]=Wire.read();
		
		#ifdef DEBUG
		Serial.print(m_receive_buffer[i],HEX);
		Serial.print(",");
		#endif
	}

	#ifdef DEBUG
	Serial.println("");
	#endif
	m_receive_length = msgLength;
}
//========================= i2c input statemachine =========================//

//================================ parse,  =================================//
// receiveEvent is callen in interrupt
// if we block to long, the i2c interface will be busy
// therefore copy it to a new buffer and process it in the userspace
// this will free the i2c buffer to receive a new message "in parallel"

void parse(){
	// only read it if there is a magic startbyte
	if(m_receive_buffer[0] == START_BYTE){
		/////////////////////////////  RESET CONFIGURATION /////////////////////////////
		if(m_receive_buffer[1] == CMD_RESET){
			resetFunc();  
		} 
		/////////////////////////////  RESET CONFIGURATION /////////////////////////////
		////////////////////////////  SIMPLE SETTING A VALUE ///////////////////////////
		// [0] START_BYTE
		// [1] CMD_SET_SIMPLE
		// [2] DIP pin
		// [3] value or value_r
		// opt: [4] value_g
		// opt: [5] value_b
		// opt: [3+0+3*i] value_r
		// opt: [3+1+3*i] value_g
		// opt: [3+2+3*i] value_b

		else if(m_receive_buffer[1] == CMD_SET_SIMPLE && m_receive_length>=4){
			m_channel = m_receive_buffer[2];

			//## digital write for output or pull ups or PWM ##
			if(m_pins[m_channel].m_mode==MODE_DIGITAL_OUTPUT || m_pins[m_channel].m_mode==MODE_DIGITAL_INPUT || m_pins[m_channel].m_mode==MODE_ANALOG_INPUT){
				m_pins[m_channel].m_value = m_receive_buffer[3];

				#ifdef DEBUG
				Serial.print("digitalWrite ");
				Serial.print(m_pins[m_channel].m_value);
				Serial.print(" at pin ");
				Serial.println(m_pins[m_channel].m_arduino_pin,DEC);
				#endif		

				
				digitalWrite(m_pins[m_channel].m_arduino_pin,m_pins[m_channel].m_value);
				
				// end dimming if we override it with hard value
				if( m_pins[m_channel].m_mode==MODE_PWM_DIMMING){
					m_pins[m_channel].m_mode = MODE_PWM;
				};
			} 

			//## write a pwm signal ##
			else if(m_pins[m_channel].m_mode==MODE_PWM || m_pins[m_channel].m_mode==MODE_PWM_DIMMING){ 
				m_pins[m_channel].m_value = m_receive_buffer[3];

				#ifdef DEBUG
				Serial.print("dutycycle of ");
				Serial.print(m_pins[m_channel].m_value);
				Serial.print(" at pin ");
				Serial.println(m_pins[m_channel].m_arduino_pin,DEC);
				#endif

				analogWrite(m_pins[m_channel].m_arduino_pin,m_pins[m_channel].m_value);
				
				// end dimming if we override it with hard value
				if( m_pins[m_channel].m_mode==MODE_PWM_DIMMING){
					m_pins[m_channel].m_mode = MODE_PWM;
				};
			}

			//## single color for all LEDs ##
			else if(m_pins[m_channel].m_mode==MODE_SINGLE_COLOR_WS2812 && m_receive_length>=6){
				value.r = m_receive_buffer[3]; 
				value.g = m_receive_buffer[4]; 
				value.b = m_receive_buffer[5]; // RGB Value -> Blue

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
				uint8_t m_current_led=m_receive_buffer[3];
				for(uint8_t i=4; i<m_receive_length; i+=3){
					value.r = m_receive_buffer[i+0]; 
					value.g = m_receive_buffer[i+1]; 
					value.b = m_receive_buffer[i+2]; // RGB Value -> Blue
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
		// [0] START_BYTE
		// [1] CMD_GET
		// [2] DIP pin
		else if(m_receive_buffer[1] == CMD_GET && m_receive_length>=3){
			m_channel = m_receive_buffer[2];
			
			#ifdef DEBUG
			Serial.print("Reading pin");
			Serial.println(m_pins[m_channel].m_arduino_pin,DEC);
			#endif
				
			if(m_pins[m_channel].m_mode == MODE_DIGITAL_INPUT){
				m_response_buffer[0] = digitalRead(m_pins[m_channel].m_arduino_pin);
				m_response_length = 1;

				#ifdef DEBUG
				Serial.print("digital read value: ");
				Serial.println(m_response_buffer[0]);
				#endif

			} else 	if(m_pins[m_channel].m_mode == MODE_ANALOG_INPUT){
				m_pins[m_channel].m_value=analogRead(m_pins[m_channel].m_arduino_pin);
				m_response_buffer[1] =  m_pins[m_channel].m_value >> 8;
				m_response_buffer[0] =  m_pins[m_channel].m_value & 0xff;
				m_response_length = 2;

				#ifdef DEBUG
				Serial.print("Value: ");
				Serial.println(m_pins[m_channel].m_value);
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
		// [0] START_BYTE
		// [1] CMD_SET_SIMPLE
		// [2] DIP pin
		// [3] mode
		else if(m_receive_buffer[1] == CMD_CONFIG && m_receive_length>=4){
			m_channel = m_receive_buffer[2];
			m_pins[m_channel].m_mode = m_receive_buffer[3];

			if( m_pins[m_channel].m_mode == MODE_SINGLE_COLOR_WS2812 ||  m_pins[m_channel].m_mode == MODE_MULTI_COLOR_WS2812 ){
				m_pins[m_channel].m_ws2812_count = m_receive_buffer[4]; 
			}
			config_pin();
		}
		///////////////////////////////  CONFIGURE A PIN ///////////////////////////////
		///////////////////////////////  DIMM THE PIN ///////////////////////////////
		// [0] START_BYTE
		// [1] CMD_DIMM
		// [2] DIP pin
		// [3] target value 0 to 99
		// [4] dimm intervall in ms
		else if(m_receive_buffer[1] == CMD_DIMM && m_receive_length>=5){
			m_channel = m_receive_buffer[2];
			if(m_pins[m_channel].m_mode==MODE_PWM){
				m_pins[m_channel].m_target_value = min(m_receive_buffer[3], 99); 	// 0-99 = 0-100%
				m_pins[m_channel].m_dimm_interval = m_receive_buffer[4];					// ms
				m_pins[m_channel].m_mode = MODE_PWM_DIMMING;
				m_pins[m_channel].m_next_action  = millis();

				#ifdef DEBUG
				Serial.print("Dimming pin: ");
				Serial.print(m_pins[m_channel].m_arduino_pin);
				Serial.print(" from value ");
				Serial.print(m_pins[m_channel].m_value);
				Serial.print(" to value ");
				Serial.println(m_pins[m_channel].m_target_value);
				#endif

			 }
		}
		///////////////////////////////  DIMM THE PIN ///////////////////////////////
		/////////////////////////////// SETUP PWM FREQ //////////////////////////////
		// [0] START_BYTE
		// [1] CMD_PWM_FREQ
		// [2] DIP pin
		// [3] divisior 
		else if(m_receive_buffer[1] == CMD_PWM_FREQ && m_receive_length>=4){
			uint8_t divisor = m_receive_buffer[3];
			m_channel = m_receive_buffer[2];
			setPwmFrequency(m_channel, divisor);
		}
		/////////////////////////////// SETUP PWM FREQ //////////////////////////////
		/////////////////////////////// TRIGGER AFTER SLEEP //////////////////////////////
		// [0] START_BYTE
		// [1] CMD_TRIGGER_AFTER_SLEEP
		// [2] Pin to trigger 
		// [3] hold down length in seconds for the first push
		// [4] hold down length in seconds for the second push
		// [5] low active
		// [6] wait time in sec until first push
		// [7] sleep SECONDS high byte for 2nd push
		// [8] sleep SECONDS low byte for 2nd push
		
		else if(m_receive_buffer[1] == CMD_TRIGGER_AFTER_SLEEP && m_receive_length>=9){
			uint16_t wait_2nd = m_receive_buffer[7]<<8 | m_receive_buffer[8];
			uint8_t wait_1st = m_receive_buffer[6];
			uint8_t hold_1st = m_receive_buffer[3];
			uint8_t hold_2nd = m_receive_buffer[4];
			bool inverse = m_receive_buffer[5];
			m_channel = m_receive_buffer[2];
			
			// configure output if not already done
			if(m_pins[m_channel].m_mode!=MODE_DIGITAL_OUTPUT){
				if(m_pins[m_channel].is_digital_inout()){
					// change the level first to avoid peak
					if(!inverse){
						digitalWrite(m_pins[m_channel].m_arduino_pin,LOW);
					} else {
						digitalWrite(m_pins[m_channel].m_arduino_pin,HIGH);
					}
					// set input for now
					pinMode(m_pins[m_channel].m_arduino_pin,INPUT);
				}
			}
			
			while(wait_1st>0){
				delay(1000);
				wait_1st--;
			}

			// set to output for first push
			pinMode(m_pins[m_channel].m_arduino_pin,OUTPUT);
			// push
			if(!inverse){
				digitalWrite(m_pins[m_channel].m_arduino_pin,HIGH);
			} else {
				digitalWrite(m_pins[m_channel].m_arduino_pin,LOW);
			}
			
			// hold down the pin
			delay(1000*((uint32_t)hold_1st));
			
			// release
			if(!inverse){
				digitalWrite(m_pins[m_channel].m_arduino_pin,LOW);
			} else {
				digitalWrite(m_pins[m_channel].m_arduino_pin,HIGH);
			}
			// set back to input for now
			pinMode(m_pins[m_channel].m_arduino_pin,INPUT);

			// here we go, try to stay in that 8sec mode as long as possible
			while(wait_2nd>8){
				LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);  
				wait_2nd-=8;
			}
			// switch to 1 sec
			while(wait_2nd>0){
				LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);  
				wait_2nd--;
			}
			

			// set to output for push
			pinMode(m_pins[m_channel].m_arduino_pin,OUTPUT);
			// push
			if(!inverse){
				digitalWrite(m_pins[m_channel].m_arduino_pin,HIGH);
			} else {
				digitalWrite(m_pins[m_channel].m_arduino_pin,LOW);
			}
			
			// hold down the pin
			delay(1000*((uint32_t)hold_2nd));
			
			// release
			if(!inverse){
				digitalWrite(m_pins[m_channel].m_arduino_pin,LOW);
			} else {
				digitalWrite(m_pins[m_channel].m_arduino_pin,HIGH);
			}			
			// set back to input for now
			pinMode(m_pins[m_channel].m_arduino_pin,INPUT);
		}
		/////////////////////////////// TRIGGER AFTER SLEEP //////////////////////////////
	}

	#ifdef DEBUG
	Serial.println("end");
	#endif
};
//================================ parse,  =================================//


//========================= i2c response =========================//
// the arduino wire lib is clearing the i2c buffer whenever it receives the SLAVE READ from the I2C Master
// therefore we can NOT pre-set the tx buffer, but have to do it after this read-call.
// this read-call triggers the onRequestService, to whom we've subscribed. So filling the buffer now should work
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
