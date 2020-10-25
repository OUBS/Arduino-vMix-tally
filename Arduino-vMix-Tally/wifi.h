#pragma once

/**
 * Code to be run on the core 0 of the ESP32. This includes all wifi related
 * stuff.
 */

#include <atomic>
#include <bitset>
#include <FreeRTOS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include "config.h"
#include "settings.h"

namespace wifi
{
  static TaskHandle_t wifi_task_handle;

  /**
   * Status of the wifi thread.
   */
  struct Status
  {
    unsigned int is_wifi_connected : 1;
    unsigned int is_vmix_connected : 1;
    unsigned int is_ap_active : 1;
    // unsigned int color : 2;
    std::bitset<MAX_TALLY_SOURCES> preview;
    std::bitset<MAX_TALLY_SOURCES> program;

    static Status connecting()
    {
      return {0, 0, 0, {}, {}};
    }

    static Status ap_active()
    {
      return {0, 0, 1, {}, {}};
    }

    static Status wifi_connected()
    {
      return {1, 0, {}, {}};
    }

    static Status vmix_connected()
    {
      return {1, 1, 0, {}, {}};
    }

    static Status tally(std::bitset<MAX_TALLY_SOURCES> preview,
                        std::bitset<MAX_TALLY_SOURCES> program)
    {
      return {1, 1, 0, preview, program};
    }

    bool is_connected() const
    {
      return is_wifi_connected || is_vmix_connected;
    }

    operator bool() const
    {
      return is_connected();
    }

    /**
     * Gets the color of the input.
     */
    int color(int input) const
    {
      if (input >= MAX_TALLY_SOURCES)
      {
        return 0;
      }
      if (program[input])
      {
        return 1;
      }
      if (preview[input])
      {
        return 2;
      }
      return 0;
    }
  };

  void wifi_task(void *);

  void start_wifi_task()
  {
    SPIFFS.begin();
    xTaskCreatePinnedToCore(
        wifi_task,
        "wifi task",
        16384,
        nullptr,
        0,
        &wifi_task_handle,
        0 // Run on core 0 while the default is 1
    );
  }

  // Implement atomic operations for status using lock
  static Status status{};
  static portMUX_TYPE status_mux = portMUX_INITIALIZER_UNLOCKED;

  Status get_status()
  {
    Status result;
    portENTER_CRITICAL(&status_mux);
    result = status;
    portEXIT_CRITICAL(&status_mux);
    return result;
  }

  void set_status(Status s)
  {
    portENTER_CRITICAL(&status_mux);
    status = s;
    portEXIT_CRITICAL(&status_mux);
  }

  enum class Mode
  {
    STA,
    AP
  };
  std::atomic<Mode> mode(Mode::STA);

  void set_mode(Mode m)
  {
    mode.store(m);
  }

  /**
   * Connect to vMix and receive tally status.
   */
  void vmix_mode()
  {
    // Get the wlan settings from the settings store.
    settings::WlanSettings wlan_settings = settings::settings.get_settings().wlan;

    // Start connecting to the given access point
    WiFi.mode(WIFI_STA);
    WiFi.begin(wlan_settings.ssid, wlan_settings.pass);
    Serial.print("Connecting to ");
    Serial.println(wlan_settings.ssid);

    set_status(Status::connecting());

    // Try connecting until timeout or success
    const auto start = millis();
    while (WiFi.status() != WL_CONNECTED && mode == Mode::STA && millis() - start < CONNECTION_TIMEOUT)
    {
      taskYIELD();
    }

    if (WiFi.status() == WL_CONNECTED)
    {
      set_status(Status::wifi_connected());
      Serial.println("Connected");

      // Try to connect to vmix
      WiFiClient client;
      bool connected;
      do
      {
        connected = client.connect(wlan_settings.host_name, VMIX_PORT);
      } while (WiFi.status() == WL_CONNECTED && mode == Mode::STA && !connected);

      if (connected)
      {
        set_status(Status::vmix_connected());

        // Subscribe for tally messages.
        client.print("SUBSCRIBE TALLY\r\n");

        while (client && mode == Mode::STA)
        {
          taskYIELD();

          String message = client.readStringUntil('\n');
          message.trim();

          // Parse the TALLY message and ignore the rest
          if (message.startsWith("TALLY OK "))
          {
            std::bitset<MAX_TALLY_SOURCES> preview{};
            std::bitset<MAX_TALLY_SOURCES> program{};
            for (int i = 0; i < MAX_TALLY_SOURCES && 9 + i < message.length(); i++)
            {
              switch (message[9 + i])
              {
              case '1':
                program.set(i);
                break;
              case '2':
                preview.set(i);
                break;
              default:
                break;
              }
            }
            set_status(Status::tally(preview, program));
          }
        }
      }
    }
    else
    {
      // Some debug information in case the connection failed.
      if (WiFi.status() == WL_IDLE_STATUS)
        Serial.println("Idle");
      else if (WiFi.status() == WL_NO_SSID_AVAIL)
        Serial.println("No SSID Available");
      else if (WiFi.status() == WL_SCAN_COMPLETED)
        Serial.println("Scan Completed");
      else if (WiFi.status() == WL_CONNECT_FAILED)
        Serial.println("Connection Failed");
      else if (WiFi.status() == WL_CONNECTION_LOST)
        Serial.println("Connection Lost");
      else if (WiFi.status() == WL_DISCONNECTED)
        Serial.println("Disconnected");
      else
        Serial.println("Unknown Failure");
    }
  }

  /**
   * Host an access point for setting settings.
   */
  void ap_mode()
  {
    WiFi.mode(WIFI_AP);
    WiFi.softAP("oubs-vmix-tally");
    IPAddress ip = WiFi.softAPIP();
    Serial.print("Entering AP mode. IP: ");
    Serial.println(ip);

    // Start the web server on port 80
    WebServer server(80);
    // Simple getters for getting the current settings. This is done this way
    // so that we don't need to inject these into the main web page and that we
    // don't need to worry about HTML escaping.
    server.on(
        "/get_ssid", HTTP_GET, [&]() {
          settings::WlanSettings wlan_settings = settings::settings.get_settings().wlan;
          server.send(200, "text/plain", wlan_settings.ssid);
        });
    server.on(
        "/get_pass", HTTP_GET, [&]() {
          settings::WlanSettings wlan_settings = settings::settings.get_settings().wlan;
          server.send(200, "text/plain", wlan_settings.pass);
        });
    server.on(
        "/get_host", HTTP_GET, [&]() {
          settings::WlanSettings wlan_settings = settings::settings.get_settings().wlan;
          server.send(200, "text/plain", wlan_settings.host_name);
        });

    // Handler for save endpoint
    server.on("/save", HTTP_POST, [&]() {
      // Construct settings from the given arguments
      settings::WlanSettings wlan_settings = settings::settings.get_settings().wlan;

      if (server.hasArg("ssid"))
      {
        String ssid = server.arg("ssid");
        if (ssid.length() + 1 <= settings::SSID_LENGTH)
        {
          ssid.toCharArray(wlan_settings.ssid, settings::SSID_LENGTH);
        }
      }

      if (server.hasArg("pass"))
      {
        String pass = server.arg("pass");
        if (pass.length() + 1 <= settings::PASS_LENGTH)
        {
          pass.toCharArray(wlan_settings.pass, settings::PASS_LENGTH);
        }
      }

      if (server.hasArg("host"))
      {
        String host = server.arg("host");
        if (host.length() + 1 <= settings::HOST_LENGTH)
        {
          host.toCharArray(wlan_settings.host_name, settings::HOST_LENGTH);
        }
      }

      settings::settings.set_wlan(wlan_settings);

      server.sendHeader("Location", String("/"), true);
      server.send(302, "text/plain", "Redirected to: /");
    });
    // Serve static files. All need to be individually listed here.
    server.serveStatic("/", SPIFFS, "/index.html");
    server.serveStatic("/oubs.png", SPIFFS, "/oubs.png");
    server.onNotFound([&]() {
      Serial.println("404");
      server.send(404, "text/plain", "Not found");
    });
    server.begin();

    set_status(Status::ap_active());

    // Handle clients until the user wants to switch to another mode.
    while (mode == Mode::AP)
    {
      server.handleClient();
    }

    server.close();
  }

  /**
   * Task run on core 0.
   */
  void wifi_task(void *)
  {
    while (true)
    {
      switch (mode)
      {
      case Mode::STA:
        vmix_mode();
        break;
      case Mode::AP:
        ap_mode();
        break;
      }
    }
  }
} // namespace wifi