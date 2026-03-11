#pragma once
#include <Arduino.h>

class Meter {
public:
  Meter(int pin, int channel);
  void begin();
  void setValue(int value);  // Immediate set
  void setTarget(float value); // Smooth ramp target
  void update();             // Call frequently to update ramp

private:
  int _pin;
  int _channel;
  const int _freq = 5000;
  const int _resolution = 10;

  float _currentVal = 0;
  float _targetVal = 0;
  unsigned long _lastUpdate = 0;
};
