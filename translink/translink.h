#ifndef _TRANSLINK_H_
#define _TRANSLINK_H_

#include <Arduino.h>

void fetch_schedule(int stop_number, int route_number, struct bus *out);

#endif
