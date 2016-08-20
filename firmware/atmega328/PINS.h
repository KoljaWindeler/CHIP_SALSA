#ifndef DIP_PINS_H_
#define DIP_PINS_H_
#include <stdlib.h>
#include <stdint.h>
#include "WS2812.h"

class PIN {
	public:
		PIN();
		void set(bool analog_in, bool digital, bool pwm, bool ws2812, uint8_t arduino_pin);
		bool is_analog_in();
		bool is_analog_in_out();
		bool is_pwm_out();
		bool is_digital_inout();
		bool is_ws2812();
		void assign_ws2812(WS2812* p);

		uint8_t m_arduino_pin=0xff;
		uint8_t m_mode=0xff;
		uint8_t m_target_value = 0x00;
		uint16_t m_value=0x00; // 16 bit for analog read (10 bit adc)
		uint16_t m_dimm_interval  = 0x00;
		uint32_t m_next_action  = 0x00;
		

		WS2812* m_ws2812_pointer;
		uint16_t m_ws2812_count;

	private:
		bool m_analog_in=false;
		bool m_digital=false;
		bool m_pwm=false;
		bool m_ws2812=false;
		
		
};	
#endif
