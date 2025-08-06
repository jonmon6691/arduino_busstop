#ifndef _TRIMET_H_
#define _TRIMET_H_

#include <Arduino.h>

void fetch_schedule(int stop_number, int route_number, struct bus *out);

#endif
