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

void init_button(struct button *btn, uint8_t pin)
{
    pinMode(pin, INPUT_PULLUP);
    btn->pin = pin;
    btn->millis_last_falling_edge = 0;
    btn->last_button_state = 1;
    btn->timer1_fired = 0;
    btn->timer2_fired = 0;
    btn->timer3_fired = 0;
}

int handle_button(struct button *btn)
{
    int button_state = digitalRead(btn->pin);
    int retval = BUTTON_NOT_PRESSED;

    // Falling edge (being pressed)
    if (btn->last_button_state == 1 && button_state == 0) {
        btn->millis_last_falling_edge = millis();
        retval = BUTTON_FALLING_EDGE;
    }

    // Low (being held)
    if (btn->last_button_state == 0 && button_state == 0) {
        retval = BUTTON_BEING_HELD;
        if (millis() - btn->millis_last_falling_edge > 50 && !btn->timer1_fired) {
            retval = BUTTON_HELD_50MS;
            btn->timer1_fired = 1;
        }
        if (millis() - btn->millis_last_falling_edge > 500 && !btn->timer2_fired) {
            retval = BUTTON_HELD_500MS;
            btn->timer2_fired = 1;
        }
        if (millis() - btn->millis_last_falling_edge > 2000 && !btn->timer3_fired) {
            retval = BUTTON_HELD_2000MS;
            btn->timer3_fired = 1;
        }
    }

    // Rising edge (being released)
    if (btn->last_button_state == 0 && button_state == 1) {
        btn->timer1_fired = 0;
        btn->timer2_fired = 0;
        btn->timer3_fired = 0;
        retval = BUTTON_RISING_EDGE;
    }

    btn->last_button_state = button_state;
    return retval;
}

#endif