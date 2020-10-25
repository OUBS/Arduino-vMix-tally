#pragma once
/**
 * Methods for accessing buttons
 */

#include <ESP32TimerInterrupt.h>
#include <atomic>
#include "config.h"

namespace buttons
{
  // Every bit of the pattern represents 1ms
  constexpr u32_t PRESS_PATTERN = /*   */ 0b00000000000000000000000011111111;
  constexpr u32_t PRESS_MASK = /*      */ 0b11111111000000000000000011111111;
  constexpr u32_t RELEASE_PATTERN = /* */ 0b11111111000000000000000000000000;
  constexpr u32_t RELEASE_MASK = /*    */ 0b11111111000000000000000011111111;

  /**
   * Debouncer for buttons. Based on https://hackaday.com/2015/12/10/embed-with-elliot-debounce-your-noisy-buttons-part-ii/
   */
  struct Debouncer
  {
    volatile u32_t state;
    volatile std::atomic<bool> click_triggered;
    volatile std::atomic<bool> hold_triggered;
    volatile bool has_been_held;
    volatile unsigned long press_time;

    // This needs to be inlined because it is called from an interrupt, but it
    // can't be marked with IRAM_ATTR
    void __attribute__((always_inline)) update(bool s)
    {
      u32_t stat = (state << 1) | s;
      if ((stat & PRESS_MASK) == PRESS_PATTERN)
      {
        press_time = millis();
        stat = ~0;
      }
      else if ((stat & RELEASE_MASK) == RELEASE_PATTERN)
      {
        stat = 0;
        if (!has_been_held)
        {
          click_triggered = true;
        }
        has_been_held = false;
      }
      else if (stat == ~0)
      {
        if (millis() - press_time >= BTN_HOLD_TIMEOUT)
        {
          has_been_held = true;
          hold_triggered = true;
          press_time = millis();
        }
      }

      state = stat;
    }

    bool is_clicked()
    {
      return click_triggered.exchange(false);
    }

    bool is_held()
    {
      return hold_triggered.exchange(false);
    }
  };

  Debouncer brightness{};
  Debouncer up{};
  Debouncer mode{};
  Debouncer down{};

  void IRAM_ATTR update_buttons()
  {
    brightness.update(!digitalRead(BTN_BRGHT_PIN));
    up.update(!digitalRead(BTN_UP_PIN));
    mode.update(!digitalRead(BTN_MODE_PIN));
    down.update(!digitalRead(BTN_DOWN_PIN));
  }

  ESP32Timer ITimer0(0);

  void initialize_buttons()
  {
    // Set all buttons as inputs
    pinMode(BTN_BRGHT_PIN, INPUT);
    pinMode(BTN_UP_PIN, INPUT);
    pinMode(BTN_MODE_PIN, INPUT);
    pinMode(BTN_DOWN_PIN, INPUT);
    // Call interrupt every 1ms to read the button status and update the debouncers.
    ITimer0.attachInterruptInterval(1000, update_buttons);
  }

} // namespace buttons