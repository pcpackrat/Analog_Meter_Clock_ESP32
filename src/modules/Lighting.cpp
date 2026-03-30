#include "Lighting.h"
#include "TimeManager.h"

extern TimeManager timeManager;

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
  bool useTz2 = timeManager.isSecondaryTzActive();
  uint8_t nightStart = useTz2 ? config.getNightStart2() : config.getNightStart();
  uint8_t nightStartMin = useTz2 ? config.getNightStartMinute2() : config.getNightStartMinute();
  uint8_t nightEnd = useTz2 ? config.getNightEnd2() : config.getNightEnd();
  uint8_t nightEndMin = useTz2 ? config.getNightEndMinute2() : config.getNightEndMinute();

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

  uint8_t brightness = isNight ? config.getNightBrightness() : config.getDayBrightness();
  FastLED.setBrightness(brightness);

  if (config.getUseSharedColors()) {
    CRGB targetColor = CRGB(isNight ? config.getNightColor() : config.getDayColor());
    fill_solid(_leds, NUM_LEDS, targetColor);
  } else {
    if (isNight) {
      setColor(0, CRGB(config.getNightColor()));
      setColor(1, CRGB(config.getNightColorMinute()));
      setColor(2, CRGB(config.getNightColorSecond()));
    } else {
      setColor(0, CRGB(config.getDayColor()));
      setColor(1, CRGB(config.getDayColorMinute()));
      setColor(2, CRGB(config.getDayColorSecond()));
    }
  }
}

void Lighting::show() { FastLED.show(); }
