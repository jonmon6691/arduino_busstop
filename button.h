#ifndef _BUTTON_H_
#define _BUTTON_H_

#include <stdlib.h>

#define BUTTON_NOT_PRESSED 0
#define BUTTON_FALLING_EDGE 1
#define BUTTON_BEING_HELD 2
#define BUTTON_HELD_50MS 3
#define BUTTON_HELD_500MS 4
#define BUTTON_HELD_2000MS 5
#define BUTTON_RISING_EDGE 6

struct button {
	uint8_t pin;
	unsigned long millis_last_falling_edge;
	int last_button_state;
	int timer1_fired;
	int timer2_fired;
	int timer3_fired;
};


void init_button(struct button *btn, uint8_t pin);
int handle_button(struct button *btn);


#endif