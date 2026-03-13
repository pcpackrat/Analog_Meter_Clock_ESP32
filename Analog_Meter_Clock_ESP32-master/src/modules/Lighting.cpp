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

void Lighting::update(int hour, int minute, Config &config, bool isError) {
  if (isError) {
    // Flash LED 1 Red, others OFF
    fill_solid(_leds, NUM_LEDS, CRGB::Black);
    if ((millis() / 500) % 2 == 0) { // 1Hz Flash
      _leds[0] = CRGB::Red;          // LED 1
    }
    return;
  }

  // Get settings from Config
  uint8_t nightStart = config.getNightStart();
  uint8_t nightStartMin = config.getNightStartMinute();
  uint8_t nightEnd = config.getNightEnd();
  uint8_t nightEndMin = config.getNightEndMinute();

  // Convert everything to minutes from midnight
  int currentMin = (hour * 60) + minute;
  int startMin = (nightStart * 60) + nightStartMin;
  int endMin = (nightEnd * 60) + nightEndMin;

  bool isNight = false;
  if (startMin > endMin) {
    // Example: Start 21:30 (1290), End 07:15 (435)
    // Night is: current >= 1290 OR current < 435
    if (currentMin >= startMin || currentMin < endMin)
      isNight = true;
  } else {
    // Example: Start 01:00 (60), End 05:00 (300)
    // Night is: current >= 60 AND current < 300
    if (currentMin >= startMin && currentMin < endMin)
      isNight = true;
  }

  CRGB targetColor;
  uint8_t brightness;

  if (isNight) {

    targetColor = CRGB(config.getNightColor());
    brightness = config.getNightBrightness();
  } else {

    targetColor = CRGB(config.getDayColor());
    brightness = config.getDayBrightness();
  }

  FastLED.setBrightness(brightness);
  fill_solid(_leds, NUM_LEDS, targetColor);
}

void Lighting::show() { FastLED.show(); }
