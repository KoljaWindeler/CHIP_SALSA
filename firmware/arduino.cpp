#include </home/kolja/projects/CHIP_pwm_shield/firmware/atmega328/WS2812.h>	// thanks Matthias Riegler!
#include <Wire.h>

#define START_BYTE		0xCE
#define CMD_SET 		0xF0
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

#define ST_WAIT 			0xFF
#define ST_CMD 				0x00
#define ST_CONFIG_CHANNEL		0x01
#define ST_CONFIG_MODE 			0x02
#define ST_SET_CHANNEL			0x03
#define ST_SET_OFFSET			0x04
#define ST_SET_SINGLE_VALUE 		0x05
#define ST_SET_VALUE_R 			0x06
#define ST_SET_VALUE_G 			0x07
#define ST_SET_VALUE_B			0x08
#define ST_WS2812_NUM			0x09

#define TRANSFER_TIMEOUT 50
#define I2C_ADDRESS	0x04

#define DEBUG
#define MAX_RESPONSE_LENGTH 4

uint32_t	m_lasttransfer=0;
uint8_t		m_state=ST_WAIT;
uint8_t		m_channel=0;
uint8_t		m_value_r=0;
uint8_t		m_value_g=0;
uint8_t		m_value_b=0;
uint8_t		m_value=0;
uint8_t		m_current_led=0;

uint8_t m_modes[5+4+4]; // 5+4+4 channels, saves the modes per channel, e.g. PWM or WS2812 // TODO new board
uint8_t m_ws_count[4]; //  only lower 4 channels	// TODO new board
uint8_t m_response_buffer[MAX_RESPONSE_LENGTH];
uint8_t m_response_length=0;
WS2812* m_ws2812[4]; //  only lower 4 channels	// TODO new board
cRGB value;


//========================= i2c config and init? =========================//
void setup(){
	Wire.begin(I2C_ADDRESS);                // join i2c bus with address #4
	Wire.onReceive(receiveEvent); // register event
	Wire.onRequest(requestEvent); // register event
	for(int i=0; i<13; i++){
		m_modes[i]=0xff; // invalid
	}
	for(int i=0; i<4; i++){
		m_ws_count[i]=0;
	}

#ifdef DEBUG
	Serial.begin(19200);
	Serial.println("woop woop");
#endif
}
//========================= i2c config and init? =========================//
//========================= check for incoming  timeouts =========================//
void loop(){
	delay(TRANSFER_TIMEOUT);
	if(millis()-m_lasttransfer>TRANSFER_TIMEOUT && m_state!=ST_WAIT){
		//Serial.print("State was ");
		//Serial.println(m_state);
		m_state=ST_WAIT;
	}
}
//========================= check for incoming  timeouts =========================//
//========================= declare reset function @ address 0 =========================//
void(* resetFunc) (void) = 0; 
//========================= declare reset function @ address 0 =========================//
//========================= translate linear shield pins to arduino pins =========================//
int shield_to_arduino_pin(uint8_t shield_pin){
	// channel row 1, left-to-right:	|| 	11(PB3), 	10(PB2),	9(PB1), 	6(PD6), 	5(PD5)	||
	// channel row 2, left-to-right:	||	 -(ADC7), 	A1/15(PC1), 	A2/16(PC2), 	A3/17(PC3),	XXX	||
	// channel row 3, left-to-right: 	||	0(PD0), 	1(PD1), 	2(PD2), 	3(PD3),		XXX	|| only these are used for up to 4 parallel ws2812 channels
	
	if(shield_pin==0)	{	return	11;	}	// upper left
	else if(shield_pin==1)	{	return	10;	}
	else if(shield_pin==2)	{	return	9;	}
	else if(shield_pin==3)	{	return	6;	}
	else if(shield_pin==4)	{	return	5;	} // upper right
	
	//else if(shield_pin==5){	return	0;	} // middle left
	else if(shield_pin==6)	{	return	15;	}
	else if(shield_pin==7)	{	return	16;	}
	else if(shield_pin==8)	{	return	17;	} // middle right
	
	else if(shield_pin==9)	{	return	0;	} // lower left
	else if(shield_pin==10)	{	return	1;	}
	else if(shield_pin==11)	{	return	2;	}
	else if(shield_pin==12)	{	return	3;	} // lower right
	
	return -1; // hmm
}
//========================= translate linear shield pins to arduino pins =========================//
//========================= setup - on-the-fly =========================//
void config_pin(uint8_t pin){
	if(m_modes[pin]==MODE_PWM){
#ifdef DEBUG
		Serial.print("PWM at pin ");
		Serial.println(shield_to_arduino_pin(pin));
#endif
		pinMode(shield_to_arduino_pin(pin),OUTPUT);
	} else if(m_modes[pin]==MODE_ANALOG_INPUT){
		if(pin>=6 && pin<=8){
			pinMode(pin,INPUT);
		}
	} else if(m_modes[pin]==MODE_DIGITAL_INPUT){
		pinMode(pin,INPUT);
	} else if(m_modes[pin]==MODE_SINGLE_COLOR_WS2812 || m_modes[pin]==MODE_MULTI_COLOR_WS2812){
		if(pin>=9 && pin<=11){
#ifdef DEBUG
			Serial.print("ws2812 at pin ");
			Serial.println(shield_to_arduino_pin(pin));
#endif

			WS2812 *ws2812 = new WS2812(m_ws_count[pin-9]);
			ws2812->setOutput(shield_to_arduino_pin(pin)); 
			m_ws2812[pin-9]=ws2812; // save pointer
		}
	}
}
//========================= setup - on-the-fly =========================//
 //========================= return read values =========================//
 // we can not pre-poplate the i2c send buffer, (yeah, too bad, I know)
 // but we can fill our own buffer
void get_value(){
	if(m_modes[m_channel] == MODE_DIGITAL_INPUT){
		m_response_buffer[0] = digitalRead(shield_to_arduino_pin(m_channel));
		m_response_length = 1;
	} else 	if(m_modes[m_channel] == MODE_ANALOG_INPUT){
		uint16_t value=analogRead(shield_to_arduino_pin(m_channel));
		uint8_t data[2];
		m_response_buffer[1] =  value >> 8;
		m_response_buffer[0] =  value & 0xff;
		m_response_length = 2;
	}
}
 //========================= return read values =========================//
//========================= control the leds, outputs =========================//
void set_value(){
	// pwm
	if(m_modes[m_channel]==MODE_PWM){ // output digital
#ifdef DEBUG
		Serial.print("dutycycle of ");
		Serial.print(m_value);
		Serial.print(" at pin ");
		Serial.print(shield_to_arduino_pin(m_channel));

#endif
		analogWrite(shield_to_arduino_pin(m_channel),m_value);
	}	// pwm end 
	
	// ws2812
	else if(m_modes[m_channel]==MODE_SINGLE_COLOR_WS2812 || m_modes[m_channel]==MODE_MULTI_COLOR_WS2812){ // output pixel
		// get values
		value.r = m_value_r; 
		value.g = m_value_g; 
		value.b = m_value_b; // RGB Value -> Blue

		if(m_modes[m_channel]==MODE_SINGLE_COLOR_WS2812){
#ifdef DEBUG
			Serial.print("single color for ");			
			Serial.print(m_ws_count[m_channel-9]);
			Serial.println(" leds");
#endif

			for(int i=0; i<m_ws_count[m_channel-9]; i++){
				m_ws2812[m_channel-9]->set_crgb_at(i, value); // Set value at all LEDs
			}
			m_ws2812[m_channel-9]->sync(); // Sends the value to the LED
		} else if(m_modes[m_channel]==MODE_MULTI_COLOR_WS2812){ 
			m_ws2812[m_channel-9]->set_crgb_at(m_current_led, value); // Set value at LED found at RUNNING INDEX
			m_current_led++;
			if(m_current_led>=m_ws_count[m_channel-9]){
				m_ws2812[m_channel-9]->sync(); // Sends the value to the LED only when all value are known
			}
		}
	}	// ws 2812 end
}
//========================= control the leds, outputs =========================//
//========================= i2c input statemachine =========================//
// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany){
	while(Wire.available()){ // loop through all
		m_lasttransfer=millis();
		unsigned char c = Wire.read(); // receive byte as a character
		//Serial.println(c,DEC);      // print the character
		
		//############# HEADER ###############
		if(m_state==ST_CMD){		// First byte, what shall we do?
			if(c == CMD_SET){			// setup a channel
				m_state = ST_SET_CHANNEL;
			} else if(c == CMD_GET){		// get a value
				m_state = ST_SET_CHANNEL;
			} else if(c == CMD_CONFIG) { // configure a channel
				m_state = ST_CONFIG_CHANNEL;
			} else if(c == CMD_RESET) { //call reset
				resetFunc();  
			}
		} else if(m_state==ST_SET_CHANNEL || m_state==ST_CONFIG_CHANNEL ){ // Second byte: we need to know what channel we are talking about
			m_channel = c;
			// distinguish where we have to go, depending on our current state
			if(m_state==ST_CONFIG_CHANNEL) {	// configuration		
				m_state=ST_CONFIG_MODE;	
			} else {														// set or get a value, we can resurrect that from the mode, the channel is in :) 
				// SET a channel  (outputs)
				if(m_modes[m_channel]==MODE_PWM){	
					m_state=ST_SET_SINGLE_VALUE;	
				} else if(m_modes[m_channel]==MODE_SINGLE_COLOR_WS2812){
					m_state=ST_SET_VALUE_R;
				} else if(m_modes[m_channel]==MODE_MULTI_COLOR_WS2812){
					m_state=ST_SET_OFFSET;
				} 
				// GET a channel (inputs)
				else if(m_modes[m_channel]==MODE_DIGITAL_INPUT){
					get_value();
					m_state=ST_CMD;
				} else if(m_modes[m_channel]==MODE_ANALOG_INPUT){
					get_value();
					m_state=ST_CMD;
				} 
			}
		} 
		//############# HEADER ###############
		//############# jump here for configuration ###############
		else if(m_state==ST_CONFIG_MODE){
			if(c<=MAX_MODE){
				m_modes[m_channel]=c;
				if(c==MODE_SINGLE_COLOR_WS2812 || c==MODE_MULTI_COLOR_WS2812){
					m_state=ST_WS2812_NUM;
				} else {
					m_state=ST_WAIT;
					config_pin(m_channel);
				}
			} else {
				m_state=ST_WAIT;  // illegal mode, go back to wait state
			}
		} else if(m_state==ST_WS2812_NUM){
			m_ws_count[m_channel-9]=c; // TODO new board
			m_state=ST_WAIT;
			config_pin(m_channel);
		}
		//############# jump here for configuration ###############	
		//############ jump here for setting a value #############
		else if(m_state==ST_SET_OFFSET){	// shift the start LED to the offset char
			m_current_led=c;		
			m_state=ST_SET_VALUE_R;
		} else if(m_state==ST_SET_SINGLE_VALUE){		// pwm
			m_value=c;
			m_state=ST_WAIT;
			set_value();
		} else if(m_state==ST_SET_VALUE_R){	// STEP1 for MODE_SINGLE_COLOR_WS2812 or MODE_MULTI_COLOR_WS2812
			m_value_r=c;
			m_state=ST_SET_VALUE_G;
		} else if(m_state==ST_SET_VALUE_G){	// STEP 2
			m_value_g=c;
			m_state=ST_SET_VALUE_B;
		} else if(m_state==ST_SET_VALUE_B){	// STEP 3
			m_value_b=c;
			set_value();	// save color and sync
			if(m_modes[m_channel]==MODE_MULTI_COLOR_WS2812){ 	// maybe we have to go back to red
				if(m_current_led>=m_ws_count[m_channel-9] || m_current_led%8==0){ // set_value() has already increase: m_current_led++, are we done, (at least with this block?)
					m_state=ST_WAIT; // done
				} else {
					m_state=ST_SET_VALUE_R; // resume to STEP 1
				}
			} else { // mode ws2812 .. single color, done!
				m_state=ST_WAIT;
			}
		} 
		//############ jump here for setting a value #############
		//############ start #############
		// if we are in a "invalid" state (or ST_WAIT),
		// we will wait for the START_BYTE
		else if(c==START_BYTE){
			m_state=ST_CMD;
		}
		//############ start #############
	}
}
//========================= i2c input statemachine =========================//
//========================= i2c response =========================//
// the arduino wire lib is clearing the i2c buffer whenever it receives the SLAVE READ from the I2C Master
// therefore we can NOT preset the tx buffer, but have to do it after the readcall.
// this readcall triggers the onRequestService, to whom we've subscribed. So filling the buffer now should work
// remember to set the MAX_RESPONSE_LENGTH according to your answer. 
void requestEvent(){
	if(m_response_length && m_response_length<MAX_RESPONSE_LENGTH){
		Wire.write(m_response_buffer,m_response_length);
		m_response_length = 0;
	}
}
//========================= i2c response =========================//