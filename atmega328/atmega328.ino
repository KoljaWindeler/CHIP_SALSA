/*
 * light_ws2812 example
 *
 * Created: 07.03.2014 12:49:30
 *  Author: Matthias Riegler
 */ 

#include </home/kolja/projects/CHIP_pwm_shield/atmega328/WS2812.h>
#include <Wire.h>

#define SEPARATOR 	0x09
#define START_BYTE	0xCE
#define ST_WAIT 	0xFF
#define ST_CHANNEL 	0x01
#define ST_MODE 	0x02
#define ST_VALUE 	0x03
#define TRANSFER_TIMEOUT 200

#define NEO_LED_COUNT 30

uint8_t  m_state=ST_WAIT;
uint8_t	 m_channel=0;
uint8_t	 m_mode=0;
uint8_t	 m_value=0;
uint32_t m_lasttransfer=0;


WS2812 LED(NEO_LED_COUNT); // 1 LED
cRGB value;


void setup(){
	Wire.begin(4);                // join i2c bus with address #4
	Wire.onReceive(receiveEvent); // register event
	for(int i=0; i<18; i++){
		pinMode(i,OUTPUT);
	}
	LED.setOutput(1); // Digital Pin 1
	//Serial.begin(19200);
	//Serial.println("Hi");
}

void loop(){
	delay(TRANSFER_TIMEOUT);
	if(millis()-m_lasttransfer>TRANSFER_TIMEOUT){
		m_state=ST_WAIT;
	}
}

void run(){
	if(m_mode==0x00){ // output digital
		digitalWrite(m_channel,m_value);
	} else if(m_mode==0x01){ // output pwm
		analogWrite(m_channel, m_value);
	} else if(m_mode==0x02){ // neo pixel
		value.b = m_value; value.g = 0; value.r = 0; // RGB Value -> Blue
		for(int i=0; i<NEO_LED_COUNT; i++){
			LED.set_crgb_at(i, value); // Set value at LED found at index 0
			uint8_t temp = value.r;
			value.r = value.g;
			value.g = value.b;
			value.b = temp;
		}
		LED.sync(); // Sends the value to the LED
	}
	
}
// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany){
	while(Wire.available()){ // loop through all but the last
		
		m_lasttransfer=millis();
		unsigned char c = Wire.read(); // receive byte as a character
		//Serial.println(c,DEC);      // print the character
		
		if(m_state==ST_CHANNEL){
			Serial.println("2");
			m_state=ST_MODE;
			m_channel = c;
		} else if(m_state==ST_MODE){
			Serial.println("3");
			m_state=ST_VALUE;
			m_mode = c;
		} else if(m_state==ST_VALUE){
			Serial.println("4");
			m_value=c;
			run();
			//m_state=ST_WAIT;
		} else if(c==START_BYTE){
			Serial.println("1");
			m_state=ST_CHANNEL;

		}
	}
}

// 0xCE, channel, mode,  value
// channel row 1, left-to-right: 11(PB3), 	10(PB2),	9(PB1), 	6(PD6), 	5(PD5)
// channel row 2, left-to-right: -(ADC7), 	A1/15(PC1), 	A2/16(PC2), 	A3/17(PC3)
// channel row 3, left-to-right: 0(PD0), 	1(PD1), 	2(PD2), 	3(PD3)
