#include <Wire.h>

#define SEPARATOR 	0x09
#define ST_START 	0x00
#define ST_CHANNEL 	0x01
#define ST_MODE 	0x02
#define ST_VALUE 	0x03
#define TRANSFER_TIMEOUT 200

uint8_t  m_state=0;
uint8_t	 m_channel=0;
uint8_t	 m_mode=0;
uint8_t	 m_value=0;
uint32_t m_lasttransfer=0;


void setup(){
	Wire.begin(4);                // join i2c bus with address #4
	Wire.onReceive(receiveEvent); // register event
	for(int i=0; i<18; i++){
		pinMode(i,OUTPUT);
	}
	Serial.begin(9600);
}

void loop(){
	delay(100);
	if(millis()-m_lasttransfer>TRANSFER_TIMEOUT){
		m_state=ST_START;
	}
}

void run(){
	if(m_mode==0x00){ // output digital
		digitalWrite(m_channel,m_value);
	} else if(m_mode==0x01){ // output pwm
		analogWrite(m_channel, m_value);
	}
}
// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany){
	while(Wire.available()){ // loop through all but the last
		m_lasttransfer=millis();
		char c = Wire.read(); // receive byte as a character
		//Serial.println(c,DEC);      // print the character
		
		if(m_state==ST_START){
			if(c==SEPARATOR){
				m_state=ST_CHANNEL;
			}
		} else if(m_state==ST_CHANNEL){
			if(c==SEPARATOR){
				m_state=ST_MODE;
			} else {
				m_channel = c;
                                //Serial.println("channel set");
			}
		} else if(m_state==ST_MODE){
			if(c==SEPARATOR){
				m_state=ST_VALUE;
			} else {
				m_mode = c;
                                //Serial.println("mode set");
			}
		} else if(m_state==ST_VALUE){
			m_value=c;
                        //Serial.println("run");
			run();
		}
	}
}

// 0x09, channel, 0x09, mode, 0x09, value
// 0x09, channel, 0x09, 0x01, 0x09, 0x8F
// channel row 1, left-to-right: 11(PB3), 	10(PB2),	9(PB1), 	6(PD6), 	5(PD5)
// channel row 2, left-to-right: -(ADC7), 	A1/15(PC1), 	A2/16(PC2), 	A3/17(PC3)
// channel row 3, left-to-right: 0(PD0), 	1(PD1), 	2(PD2), 	3(PD3)
