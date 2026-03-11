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

  _targetVal = value;
  _currentVal = value; // Instant set
  ledcWrite(_channel, value);
}

void Meter::setTarget(int value) {
  if (value > 1023)
    value = 1023;
  if (value < 0)
    value = 0;
  _targetVal = value;
}

void Meter::update() {
  unsigned long now = millis();
  unsigned long dt = now - _lastUpdate;

  if (dt > 2) { // Update every few ms
    _lastUpdate = now;

    // Ramping speed: units per millisecond
    // 1024 / 2000ms = 0.512 units/ms.
    float speed = 1.8f;
    float maxStep = speed * (float)dt;
    if (maxStep < 1.0f)
      maxStep = 1.0f; // Minimum step

    if (abs(_currentVal - _targetVal) < maxStep) {
      _currentVal = _targetVal;
    } else {
      if (_currentVal < _targetVal) {
        _currentVal += maxStep;
      } else {
        _currentVal -= maxStep;
      }
    }
    ledcWrite(_channel, (int)_currentVal);
  }
}
