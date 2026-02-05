#pragma once
#include <FastLED.h>

#define NUM_LEDS 3
#define LED_DATA_PIN 13 // Default pin, can be changed
#define LED_TYPE WS2811
#define COLOR_ORDER RGB

class Lighting {
public:
  Lighting();
  void begin();
  void setColor(int index, CRGB color);
  void show();

private:
  CRGB _leds[NUM_LEDS];
};
