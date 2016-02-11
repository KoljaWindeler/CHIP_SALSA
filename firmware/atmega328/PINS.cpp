#include "PINS.h"

PIN::PIN(){
	m_analog_in = false;
	m_digital = false;
	m_pwm = false;
	m_ws2812 = false;
	m_arduino_pin = 0xff;
	m_mode = 0xff;
	m_ws2812_count = 0xff;
	m_value=0x00;
};

void PIN::set(bool analog_in, bool digital, bool pwm, bool ws2812, uint8_t arduino_pin){
	m_analog_in = analog_in;
	m_digital = digital;
	m_pwm = pwm;
	m_ws2812 = ws2812;
	m_arduino_pin = arduino_pin;
	m_mode = 0xff;
	m_ws2812_count = 0;
	m_value = 0x00;
};

bool PIN::is_analog_in(){
	return m_analog_in;
}

bool PIN::is_analog_in_out(){
	return is_pwm_out();
};


bool PIN::is_pwm_out(){
	return m_pwm;
};

bool PIN::is_digital_inout(){
	return m_digital;
};

bool PIN::is_ws2812(){
	return m_ws2812;
};




