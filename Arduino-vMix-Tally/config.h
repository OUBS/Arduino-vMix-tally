#pragma once
/**
 * Common configurations for the tally.
 */

// Pins
constexpr int LED_PIN = 4;
constexpr int BTN_UP_PIN = 27;
constexpr int BTN_MODE_PIN = 14;
constexpr int BTN_DOWN_PIN = 12;
constexpr int BTN_BRGHT_PIN = 13;
constexpr int BATTERY_SENSE_PIN = 35;

// User input timeouts
constexpr int BTN_HOLD_TIMEOUT = 1000;       // ms
constexpr unsigned long IDLE_TIMEOUT = 5000; // ms

// Connection settings
constexpr int CONNECTION_TIMEOUT = 15000; // ms
constexpr int VMIX_PORT = 8099;
constexpr int MAX_TALLY_SOURCES = 1000;