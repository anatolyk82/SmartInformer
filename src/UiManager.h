#ifndef ESP_LIGHT_UI_MANAGER_H
#define ESP_LIGHT_UI_MANAGER_H

#include <FS.h>                   // This needs to be first, or it all crashes and burns... (from Adafruit)
#include <ESP8266WiFi.h>          // https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>          // https://github.com/bblanchon/ArduinoJson (ver: 5.x)
#include "Config.h"
#include "WiFiManager.h"

class UiManager {

public:
  UiManager();
  ~UiManager();

  /*
   *  It's a blocking function to create a UI
   *  and let the user make all necessary settings.
   */
  void initUIManager(bool resetSettngs);

  /*
   *  Getters of input parameters:
   *  They return input parameters from the UI.
   */
  char* mqttServer() { return m_mqtt_server; };
  char* mqttPort() { return m_mqtt_port; };
  char* mqttLogin() { return m_mqtt_login; };
  char* mqttPassword() { return m_mqtt_password; };
  char* mqttClientId() { return m_mqtt_client_id; };

private:
  void readConfigurationFile();
  void writeConfigurationFile();

  static void saveConfigCallback();
  static bool shouldSaveConfig;

  char *m_mqtt_server;
  char *m_mqtt_port;
  char *m_mqtt_login;
  char *m_mqtt_password;
  char *m_mqtt_client_id;
};

#endif //ESP_LIGHT_UI_MANAGER_H
