#pragma once

/**
 * Settings of the device. This includes the programmatic representation for the
 * settings and storing the settings in non-volatile memory.
 */

#include <Preferences.h>
#include <FreeRTOS.h>
#include "counter.h"

namespace settings
{
    constexpr int SSID_LENGTH = 33;
    constexpr int PASS_LENGTH = 64;
    constexpr int HOST_LENGTH = 64;

    /**
     * Settings needed to connect to vmix.
     */
    struct WlanSettings
    {
        char ssid[SSID_LENGTH];      // max 32 + nul byte
        char pass[PASS_LENGTH];      // max 63 + nul byte
        char host_name[HOST_LENGTH]; // max 63 + nul byte

        bool operator==(const WlanSettings &other) const
        {
            for (int i = 0; i < SSID_LENGTH; i++)
            {
                if (ssid[i] != other.ssid[i])
                    return false;
            }
            for (int i = 0; i < PASS_LENGTH; i++)
            {
                if (pass[i] != other.pass[i])
                    return false;
            }
            for (int i = 0; i < HOST_LENGTH; i++)
            {
                if (host_name[i] != other.host_name[i])
                    return false;
            }
        }

        bool operator!=(const WlanSettings &other) const
        {
            return !(*this == other);
        }
    };

    /**
     * All settings of the device.
     */
    struct Settings
    {
        WlanSettings wlan;
        CyclicCounter<u8_t, 1, 3> light_areas = 3;
        Counter<u8_t, 1, 8> brightness = 5;
        CyclicCounter<u16_t, 0, MAX_TALLY_SOURCES - 1> input = 0;
    };

    /**
     * Place for storing settings.
     * This takes care that the modifications are done atomically.
     */
    class SettingsStore
    {
    public:
        void init()
        {
            // Initialize EEPROM and read previous settings
            preferences.begin("oubs-tally");
            preferences.getBytes("wlan", &settings.wlan, sizeof(settings.wlan));
            preferences.getBytes("light_areas", &settings.light_areas, sizeof(settings.light_areas));
            preferences.getBytes("brightness", &settings.brightness, sizeof(settings.brightness));
            preferences.getBytes("input", &settings.input, sizeof(settings.input));
            settings_mux = portMUX_INITIALIZER_UNLOCKED;
        }

        /**
         * Gets the current settings.
         */
        Settings get_settings()
        {
            Settings s;
            portENTER_CRITICAL(&settings_mux);
            s = settings;
            portEXIT_CRITICAL(&settings_mux);
            return s;
        }

        /**
         * Set and store the wlan settings.
         */
        void set_wlan(WlanSettings wlan)
        {
            portENTER_CRITICAL(&settings_mux);
            bool changed = settings.wlan != wlan;
            if (changed)
            {
                settings.wlan = wlan;
            }
            portEXIT_CRITICAL(&settings_mux);
            if (changed)
            {
                // NVS operations can't be called inside critical sections
                preferences.putBytes("wlan", &wlan, sizeof(wlan));
            }
        }

        /**
         * Set and store the light area settings.
         */
        void set_light_areas(CyclicCounter<u8_t, 1, 3> light_areas)
        {
            portENTER_CRITICAL(&settings_mux);
            bool changed = settings.light_areas != light_areas;
            if (changed)
            {
                settings.light_areas = light_areas;
            }
            portEXIT_CRITICAL(&settings_mux);
            if (changed)
            {
                // NVS operations can't be called inside critical sections
                preferences.putBytes("light_areas", &light_areas, sizeof(light_areas));
            }
        }

        /**
         * Set and store the brightness setting.
         */
        void set_brightness(Counter<u8_t, 1, 8> brightness)
        {
            portENTER_CRITICAL(&settings_mux);
            bool changed = settings.brightness != brightness;
            if (changed)
            {
                settings.brightness = brightness;
            }
            portEXIT_CRITICAL(&settings_mux);
            if (changed)
            {
                // NVS operations can't be called inside critical sections
                preferences.putBytes("brightness", &brightness, sizeof(brightness));
            }
        }

        /**
         * Set and store the current input setting.
         */
        void set_input(CyclicCounter<u16_t, 0, MAX_TALLY_SOURCES - 1> input)
        {
            portENTER_CRITICAL(&settings_mux);
            bool changed = settings.input != input;
            if (changed)
            {
                settings.input = input;
            }
            portEXIT_CRITICAL(&settings_mux);
            if (changed)
            {
                // NVS operations can't be called inside critical sections
                preferences.putBytes("input", &input, sizeof(input));
            }
        }

    private:
        Settings settings;
        portMUX_TYPE settings_mux = portMUX_INITIALIZER_UNLOCKED;
        Preferences preferences;
    };

    /**
     * Singleton for accessing settings.
     */
    SettingsStore settings;

} // namespace settings