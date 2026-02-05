#include "Lighting.h"

Lighting::Lighting() {}

void Lighting::begin() {
  FastLED.addLeds<LED_TYPE, LED_DATA_PIN, COLOR_ORDER>(_leds, NUM_LEDS)
      .setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(128);
  fill_solid(_leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}

void Lighting::setColor(int index, CRGB color) {
  if (index >= 0 && index < NUM_LEDS) {
    _leds[index] = color;
  }
}

void Lighting::update(int hour) {
  // Simple Day/Night schedule
  // Day: 7am - 9pm (21)
  // Night: 9pm - 7am

  CRGB targetColor = _dayColor;
  if (hour < 7 || hour >= 21) {
    targetColor = _nightColor;
  }

  // Smooth transition could be here, for now just set
  fill_solid(_leds, NUM_LEDS, targetColor);
}

void Lighting::show() { FastLED.show(); }
