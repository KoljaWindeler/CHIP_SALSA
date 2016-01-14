//#include </home/kolja/projects/CHIP_pwm_shield/atmega328/WS2812.h>	// thanks Matthias Riegler!
#include <WS2812.h>	// thanks Matthias Riegler!
#include <Wire.h>
// -------------------------------------------------------------------------------------------------------------------------------------------------------------//
// config pin 5 as  pwm 			START_BYTE, 	CMD_CONFIG, 5, MODE_PWM
// set all 50%					START_BYTE, 	CMD_SET, 5, 128
// ---------------------------------------------------------------------------------------------------------------
// config pin 9 as  5 x same ws2812		START_BYTE, 	CMD_CONFIG, 9, MODE_SINGLE_COLOR_WS2812, 5
// set all red 					START_BYTE, 	CMD_SET, 9, 0xff, 0x00,0x00
// set all green				START_BYTE, 	CMD_SET, 9, 0x00, 0xff,0x00
// ---------------------------------------------------------------------------------------------------------------
// config pin 10 as  3 x unique ws2812 		START_BYTE, 	CMD_CONFIG, 10, MODE_MULTI_COLOR_WS2812, 3
// set all 1xr,1xg,1xb 				START_BYTE, 	CMD_SET, 10, 	0xff, 0x00,0x00, 	0x00, 0xff,0x00, 	0x00, 0x00,0xff
// ---------------------------------------------------------------------------------------------------------------
// restart, erase config			START_BYTE, 	CMD_RESET
// -------------------------------------------------------------------------------------------------------------------------------------------------------------//

#define START_BYTE		0xCE
#define CMD_SET 		0xF0
#define CMD_CONFIG		0xF1
#define CMD_READ		0xF2
#define CMD_RESET		0xF3

#define MODE_PWM			0x01
#define MODE_INPUT			0x02
#define MODE_SINGLE_COLOR_WS2812	0x03
#define MODE_MULTI_COLOR_WS2812		0x04
#define MAX_MODE			MODE_MULTI_COLOR_WS2812

#define ST_WAIT 			0xFF
#define ST_CMD 				0x00
#define ST_CONFIG_CHANNEL		0x01
#define ST_CONFIG_MODE 			0x02
#define ST_SET_CHANNEL			0x03
#define ST_SET_SINGLE_VALUE 		0x04
#define ST_SET_VALUE_R 			0x05
#define ST_SET_VALUE_G 			0x06
#define ST_SET_VALUE_B			0x07
#define ST_WS2812_NUM			0x08

#define TRANSFER_TIMEOUT 200
#define I2C_ADDRESS	0x04

uint8_t  m_state=ST_WAIT;
uint8_t	 m_channel=0;
uint8_t	 m_value_r=0;
uint8_t	 m_value_g=0;
uint8_t	 m_value_b=0;
uint8_t	 m_value=0;
uint8_t	 m_current_led=0;
uint32_t m_lasttransfer=0;
uint8_t m_modes[5+4+4]; // 5+4+4 channels
uint8_t m_ws_count[4]; //  only lower 4 channels
WS2812* m_ws2812[4]; //  only lower 4 channels
cRGB value;


// i2c config and init
void setup(){
	Wire.begin(I2C_ADDRESS);                // join i2c bus with address #4
	Wire.onReceive(receiveEvent); // register event
	for(int i=0; i<13; i++){
		m_modes[i]=0xff; // invalid
	}
	for(int i=0; i<4; i++){
		m_ws_count[i]=0;
	}
	//Serial.begin(19200);
	//Serial.println("Hi");
}

// timeout
void loop(){
	delay(TRANSFER_TIMEOUT);
	if(millis()-m_lasttransfer>TRANSFER_TIMEOUT){
		m_state=ST_WAIT;
	}
}

//declare reset function @ address 0
void(* resetFunc) (void) = 0; 

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

// setup - on-the-fly
void config_pin(uint8_t pin){
	if(m_modes[pin]==MODE_PWM){
		pinMode(pin,OUTPUT);
	} else if(m_modes[pin]==MODE_INPUT){
		if(pin>=6 && pin<=8){
			pinMode(pin,INPUT);
		}
	} else if(m_modes[pin]==MODE_SINGLE_COLOR_WS2812 || m_modes[pin]==MODE_MULTI_COLOR_WS2812){
		if(pin>=9 && pin<=11){
			WS2812 *ws2812 = new WS2812(m_ws_count[pin-9]);
			ws2812->setOutput(shield_to_arduino_pin(pin)); 
			m_ws2812[pin-9]=ws2812; // save pointer
		}
	}
}

// control the led
void set_value(){
	// pwm
	if(m_modes[m_channel]==MODE_PWM){ // output digital
		digitalWrite(m_channel,m_value);
	}	// pwm end 
	
	// ws2812
	else if(m_modes[m_channel]==MODE_SINGLE_COLOR_WS2812 || m_modes[m_channel]==MODE_MULTI_COLOR_WS2812){ // output pixel
		// get values
		value.r = m_value_r; 
		value.g = m_value_g; 
		value.b = m_value_b; // RGB Value -> Blue
		// 
		if(m_modes[m_channel]==MODE_SINGLE_COLOR_WS2812){
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


// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany){
	while(Wire.available()){ // loop through all but the last
		m_lasttransfer=millis();
		unsigned char c = Wire.read(); // receive byte as a character
		//Serial.println(c,DEC);      // print the character
		
		//############# HEADER ###############
		if(m_state==ST_CMD){
			if(c == CMD_SET){
				m_state = ST_SET_CHANNEL;
			} else if(c == CMD_CONFIG) {
				m_state = ST_CONFIG_CHANNEL;
			} else if(c == CMD_RESET) {
				resetFunc();  //call reset
			}
		} else if(m_state==ST_SET_CHANNEL || m_state==ST_CONFIG_CHANNEL ){
			m_channel = c;
			if(m_state==ST_CONFIG_CHANNEL) {			
				m_state=ST_CONFIG_MODE;	
			} else {
				if(m_modes[m_channel]==MODE_PWM){	
					m_state=ST_SET_SINGLE_VALUE;	
				} else if(m_modes[m_channel]==MODE_SINGLE_COLOR_WS2812 || m_modes[m_channel]==MODE_MULTI_COLOR_WS2812){	
					m_state=ST_SET_VALUE_R;
				}
			}
		} 
		//############# HEADER ###############
		//############# setup ###############
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
				m_state=ST_WAIT;
			}
		} else if(m_state==ST_WS2812_NUM){
			m_ws_count[m_channel]=c;
			m_state=ST_WAIT;
			config_pin(m_channel);
		}
		//############# setup ###############	
		//############ set value #############
		else if(m_state==ST_SET_SINGLE_VALUE){		// pwm
			m_value=c;
			m_state=ST_WAIT;
			set_value();
		} else if(m_state==ST_SET_VALUE_R){	// MODE_SINGLE_COLOR_WS2812 or MODE_MULTI_COLOR_WS2812
			m_value_r=c;
			m_state=ST_SET_VALUE_G;
		} else if(m_state==ST_SET_VALUE_G){
			m_value_g=c;
			m_state=ST_SET_VALUE_B;
		} else if(m_state==ST_SET_VALUE_B){
			m_value_b=c;
			set_value();	
			if(m_modes[m_channel]==MODE_MULTI_COLOR_WS2812){ 	// maybe we have to go back to red
				if(m_current_led>=m_ws_count[m_channel-9]){ 					// set_value sets m_current_led++
					m_state=ST_WAIT; // done
				} else {
					m_state=ST_SET_VALUE_R; // resume
				}
			} else { // mode ws2812 .. single color, done!
				m_state=ST_WAIT;
			}
		} 
		//############ set value #############
		//############ start #############
		else if(c==START_BYTE){
			m_state=ST_CMD;
		}
		//############ start #############
	}
}
