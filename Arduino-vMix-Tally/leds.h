#pragma once
/**
 * Functionality related to running leds.
 */

#include <FastLED.h>
#include <bitset>
#include <cc.h>

#include "config.h"

namespace leds
{
  constexpr int NUM_LEDS = 10;

  static CRGB leds[NUM_LEDS];
  static std::bitset<NUM_LEDS> led_active;

  // Declare colors
  static const CRGB program_color = CRGB(0xFF0000);    // Red
  static const CRGB preview_color = CRGB(0xFF7F00);    // Yellow
  static const CRGB inactive_color = CRGB(0x0000FF);   // Blue
  static const CRGB connecting_color = CRGB(0xFF00FF); // Purple
  static const CRGB ap_color = CRGB(0xFFFFFF);         // White
  static const CRGB colors[3] = {inactive_color, program_color, preview_color};
  static const CRGB menu_color = CRGB(0x00FFFF); // Turquoise

  void initialize_leds()
  {
    FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  }

  /**
   * Set the brightness of the leds.
   */
  void set_brightness(u8_t brightness)
  {
    FastLED.setBrightness(brightness);
  }

  /**
   * Set the areas that are light up in normal operation mode.
   * Bit 0 is the small side, bit 1 is the large side.
   */
  void set_light_areas(u8_t areas)
  {
    for (int i = 0; i < NUM_LEDS; i++)
    {
      if ((i + 1) % (NUM_LEDS / 2) == 0)
      {
        // Small side
        led_active.set(i, areas & 1);
      }
      else
      {
        // Large side
        led_active.set(i, areas & 2);
      }
    }
  }

  /**
   * Show a rainbow effect while connecting to wifi.
   */
  void show_connecting_wifi()
  {
    static unsigned long last_update = millis();
    constexpr unsigned long UPDATE_INTERVAL = 10;

    unsigned long now = millis();
    if (now - last_update >= UPDATE_INTERVAL)
    {
      if (now - last_update >= 2 * UPDATE_INTERVAL)
      {
        last_update = now;
      }
      else
      {
        last_update += UPDATE_INTERVAL;
      }

      // Rainbow effect
      static int state = 0;
      int color = state++;

      for (int i = 0; i < NUM_LEDS; i++)
      {
        leds[i] = led_active[i] ? CRGB(CHSV((color + i * (256 / NUM_LEDS)) & 0xFF, 255, 255)) : CRGB::Black;
      }
    }

    // The leds need to be updated every iteration to keep dithering looking good.
    FastLED.show();
  }

  /**
   * Show the input color.
   */
  void show_main(bool is_connected, u8_t status)
  {
    CRGB color = is_connected ? colors[status] : connecting_color;
    for (int i = 0; i < NUM_LEDS; i++)
    {
      leds[i] = led_active[i] ? color : CRGB::Black;
    }

    // The leds need to be updated every iteration to keep dithering looking good.
    FastLED.show();
  }

  /**
   * Show the number of active input in binary while showing the input color.
   */
  void show_input(bool is_connected, u8_t status, u16_t input)
  {
    CRGB color = is_connected ? colors[status] : connecting_color;
    for (int i = 0; i < NUM_LEDS; i++)
    {
      if ((input + 1) & (1 << i))
      {
        leds[i] = color; // We don't care of the activation status for showing binary bit
      }
      else
      {
        leds[i] = led_active[i] ? color / (u8_t)5 : CRGB::Black;
      }
    }

    // The leds need to be updated every iteration to keep dithering looking good.
    FastLED.show();
  }

  /**
   * Show the battery status.
   * The top row has first led lit up, the bottom row shows the battery charge.
   */
  void show_battery_status(float voltage)
  {
    // First row shows the menu input
    for (int i = 0; i < NUM_LEDS; i++)
    {
      leds[i] = CRGB::Black;
    }
    leds[0] = menu_color;

    // Second row shows the battery charge based on voltage
    // Thresholds are based on https://blog.ampow.com/lipo-voltage-chart/
    static_assert(NUM_LEDS == 10, "Battery status indicator works only for 10 leds without modifications");
    leds[9] = voltage >= 4.15 ? menu_color : CRGB::Black; // 95%
    leds[8] = voltage >= 3.98 ? menu_color : CRGB::Black; // 75%
    leds[7] = voltage >= 3.85 ? menu_color : CRGB::Black; // 55%
    leds[6] = voltage >= 3.79 ? menu_color : CRGB::Black; // 35%
    leds[5] = voltage >= 3.71 ? menu_color : CRGB::Black; // 15%

    // The leds need to be updated every iteration to keep dithering looking good.
    FastLED.show();
  }

  /**
   * Show the config mode option in menu.
   * The top row has the second led lit up.
   */
  void show_enter_config_mode()
  {
    // First row shows the menu input
    for (int i = 0; i < NUM_LEDS; i++)
    {
      leds[i] = CRGB::Black;
    }
    leds[1] = menu_color;

    // TODO: Show something else?

    // The leds need to be updated every iteration to keep dithering looking good.
    FastLED.show();
  }

  /**
   * Show the charge mode in menu.
   * The top row has the third led lit up;
   */
  void show_enter_charge_mode()
  {
    // First row shows the menu input
    for (int i = 0; i < NUM_LEDS; i++)
    {
      leds[i] = CRGB::Black;
    }
    leds[2] = menu_color;

    // TODO: Show something else?

    // The leds need to be updated every iteration to keep dithering looking good.
    FastLED.show();
  }

  void show_ap_mode()
  {
    static unsigned long last_update = millis();
    constexpr unsigned long UPDATE_INTERVAL = 10;

    unsigned long now = millis();
    if (now - last_update >= UPDATE_INTERVAL)
    {
      if (now - last_update >= 2 * UPDATE_INTERVAL)
      {
        last_update = now;
      }
      else
      {
        last_update += UPDATE_INTERVAL;
      }

      // Rainbow effect
      static int state = 0;
      state++;
      int color = state & 0x100 ? state & 0xFF : 255 - (state & 0xFF);

      for (int i = 0; i < NUM_LEDS; i++)
      {
        // TODO: Should we care about active leds?
        leds[i] = CRGB(ap_color.r * color / 255, ap_color.g * color / 255, ap_color.b * color / 255);
      }
    }

    // The leds need to be updated every iteration to keep dithering looking good.
    FastLED.show();
  }
} // namespace leds