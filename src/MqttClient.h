#ifndef ESP_LIGHT_MQTT_CLIENT_H
#define ESP_LIGHT_MQTT_CLIENT_H

#include <AsyncMqttClient.h>      // https://github.com/marvinroger/async-mqtt-client + (https://github.com/me-no-dev/ESPAsyncTCP)
#include <ESP8266WiFi.h>          // https://github.com/esp8266/Arduino
#include <ArduinoJson.h>          // https://github.com/bblanchon/ArduinoJson (ver: 5.x)

#include <functional>
#include <vector>

#include "Config.h"

/*
 * The structure of the JSON document:
 * {
 *   "state": "ON",
 *   "ip": "192.168.1.110",
 *   "mac": "11:22:33:44:55:66",
 *   "rssi": "-67",
 *   "uptime": "12:23:33"
 * }
 */
class LEDMatrixDevice;

class DeviceMqttClient : public AsyncMqttClient
{
public:
  DeviceMqttClient();
  ~DeviceMqttClient();

  /*
   * Publish an MQTT message with a current state of the device.
   * It sends a json document as the payload of mqt message.
   */
  void publishDeviceState();

  /*
   * Set a reference to the device state structure.
   * This must be called in setup() before sending/receiving any data.
   */
  void setDevice(LEDMatrixDevice *device) {
    m_device = device;
  }

private:
  void onMqttConnect(bool sessionPresent);
  void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
  void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);

  LEDMatrixDevice *m_device = nullptr;
};


#endif //ESP_LIGHT_MQTT_CLIENT_H
