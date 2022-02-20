#ifndef PINIO_H
#define PINIO_H

#include <Arduino.h>

///////////////////////////////////////////////////////////////////////////////
//                                 headers
///////////////////////////////////////////////////////////////////////////////

void pinModeFast(uint8_t pin, uint8_t mode);
void digitalWriteFast(uint8_t pin, bool x);
bool digitalReadFast(uint8_t pin);
void digitalToggleFast(uint8_t pin);

#endif