#include "Meter.h"

Meter::Meter(int pin, int channel) : _pin(pin), _channel(channel) {}

void Meter::begin() {
  // Note: This uses ESP32 Arduino Core v2 API.
  // Core v3 API changes: ledcAttach(pin, freq, res)
  // We stick to v2 as it's commonly compatible with many libs.
  ledcSetup(_channel, _freq, _resolution);
  ledcAttachPin(_pin, _channel);
}

void Meter::setValue(int value) {
  if (value > 1023)
    value = 1023;
  if (value < 0)
    value = 0;
  ledcWrite(_channel, value);
}
