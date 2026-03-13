#pragma once
#include <Arduino.h>

class Meter {
public:
  Meter(int pin, int channel);
  void begin();
  void setValue(int value); // 0-1023

private:
  int _pin;
  int _channel;
  const int _freq = 5000;
  const int _resolution = 10;
};
