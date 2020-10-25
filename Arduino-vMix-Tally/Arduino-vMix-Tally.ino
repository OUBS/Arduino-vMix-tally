#include <Esp.h>
#include <FreeRTOS.h>
#include "leds.h"
#include "buttons.h"
#include "wifi.h"
#include "counter.h"
#include "settings.h"

void setup()
{
  Serial.begin(9600);
  settings::settings.init();

  leds::initialize_leds();
  buttons::initialize_buttons();

  wifi::start_wifi_task();

  // Battery level indicator
  pinMode(BATTERY_SENSE_PIN, INPUT);
  analogSetPinAttenuation(BATTERY_SENSE_PIN, ADC_11db);
  analogSetSamples(128);
  if (!adcAttachPin(BATTERY_SENSE_PIN))
  {
    Serial.println("Failed to attach ADC to battery pin");
  }
}

enum class MenuState : u8_t
{
  Main,
  Main_HilightInput,
  Brightness,
  Settings_BatteryStatus,
  Settings_EnterConfigMode,
  Settings_EnterChargeMode,
  AP_Mode,
};

void loop()
{
  // Keep track of the state of the device menu
  static MenuState menu = MenuState::Main;
  // We need to store the time of the last input so that we can automatically
  // reset the menu to Main.
  static unsigned long last_input = millis();
  const wifi::Status status = wifi::get_status();

  auto settings = settings::settings.get_settings();

  // Input handling
  switch (menu)
  {
  case MenuState::Main:
  case MenuState::Main_HilightInput:
    if (buttons::up.is_clicked())
    {
      settings.input.inc();
      menu = MenuState::Main_HilightInput;
      last_input = millis();
    }
    if (buttons::down.is_clicked())
    {
      settings.input.dec();
      menu = MenuState::Main_HilightInput;
      last_input = millis();
    }
    if (buttons::mode.is_clicked())
    {
      menu = MenuState::Settings_BatteryStatus;
      last_input = millis();
    }
    if (buttons::brightness.is_clicked())
    {
      menu = MenuState::Brightness;
      last_input = millis();
    }
    if (buttons::brightness.is_held())
    {
      settings.light_areas.inc();
      last_input = millis();
    }
    break;

  case MenuState::Brightness:
    if (buttons::up.is_clicked())
    {
      settings.brightness.inc();
      last_input = millis();
    }
    if (buttons::down.is_clicked())
    {
      settings.brightness.dec();
      last_input = millis();
    }
    if (buttons::mode.is_clicked())
    {
      menu = MenuState::Settings_BatteryStatus;
      last_input = millis();
    }
    if (buttons::brightness.is_clicked())
    {
      menu = MenuState::Main;
      last_input = millis();
    }
    if (buttons::brightness.is_held())
    {
      settings.light_areas.inc();
      last_input = millis();
    }
    break;

  case MenuState::Settings_BatteryStatus:
  case MenuState::Settings_EnterConfigMode:
  case MenuState::Settings_EnterChargeMode:
    // Somewhat ugly hack to get the menu be cyclic
    {
      CyclicCounter<u8_t, (u8_t)MenuState::Settings_BatteryStatus, (u8_t)MenuState::Settings_EnterChargeMode> menu_cycle = (u8_t)menu;
      if (buttons::up.is_clicked())
      {
        menu_cycle.inc();
        last_input = millis();
      }
      if (buttons::down.is_clicked())
      {
        menu_cycle.dec();
        last_input = millis();
      }
      menu = (MenuState)(u8_t)menu_cycle;
    }

    if (buttons::mode.is_clicked())
    {
      switch (menu)
      {
      case MenuState::Settings_BatteryStatus:
        menu = MenuState::Main;
        break;
      case MenuState::Settings_EnterConfigMode:
        menu = MenuState::AP_Mode;
        break;
      case MenuState::Settings_EnterChargeMode:
        break;
      }
      last_input = millis();
    }
    break;

  case MenuState::AP_Mode:
    if (buttons::mode.is_clicked())
    {
      menu = MenuState::Main;
    }
    break;
  }

  // Timeouts
  if (millis() - last_input > IDLE_TIMEOUT && menu != MenuState::AP_Mode)
  {
    menu = MenuState::Main;
  }

  // Update state
  leds::set_brightness((1 << settings.brightness) - 1);
  leds::set_light_areas(settings.light_areas);

  if (menu == MenuState::AP_Mode)
  {
    wifi::set_mode(wifi::Mode::AP);
  }
  else
  {
    wifi::set_mode(wifi::Mode::STA);
  }

  // Store changed settings
  settings::settings.set_brightness(settings.brightness);
  settings::settings.set_light_areas(settings.light_areas);
  settings::settings.set_input(settings.input);

  // Show state
  switch (menu)
  {
  case MenuState::Main:
  case MenuState::Main_HilightInput:
  case MenuState::Brightness:
    // Then show status
    if (!status.is_wifi_connected)
    {
      leds::show_connecting_wifi();
    }
    else
    {
      if (menu == MenuState::Main_HilightInput)
      {
        leds::show_input(status.is_vmix_connected, status.color(settings.input), settings.input);
      }
      else
      {
        leds::show_main(status.is_vmix_connected, status.color(settings.input));
      }
    }
    break;
  case MenuState::Settings_BatteryStatus:
    // int total = 0;
    // for (int i = 0; i < 128; i++)
    // {
    //   total += analogRead(BATTERY_SENSE_PIN);
    // }
    // leds::show_battery_status(total * (1.0 / 128.0) * (2.0 * (3.3 / 4096.0)));
    leds::show_battery_status(4.0); // TODO: Actually read the voltage

    break;
  case MenuState::Settings_EnterConfigMode:
    leds::show_enter_config_mode();
    break;
  case MenuState::Settings_EnterChargeMode:
    leds::show_enter_charge_mode();
    break;

  case MenuState::AP_Mode:
    leds::show_ap_mode();
    break;
  }
}
