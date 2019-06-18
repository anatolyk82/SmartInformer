#include "MqttClient.h"
#include "LEDMatrixDevice.h"

#include <string>

DeviceMqttClient::DeviceMqttClient() : AsyncMqttClient()
{
  onConnect( std::bind(&DeviceMqttClient::onMqttConnect, this, std::placeholders::_1) );
  onDisconnect( std::bind(&DeviceMqttClient::onMqttDisconnect, this, std::placeholders::_1) );
  onMessage( std::bind(&DeviceMqttClient::onMqttMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6) );
}

DeviceMqttClient::~DeviceMqttClient()
{
}

void DeviceMqttClient::onMqttConnect(bool sessionPresent)
{
  Serial.println("MQTT: Connected");
  Serial.printf("MQTT: Session present: %d\n", sessionPresent);

  /* Subscribe the command topic */
  Serial.printf("MQTT: Subscribing at QoS 0, topic: %s\n", MQTT_TOPIC_SET);
  subscribe(MQTT_TOPIC_SET, 0);

  Serial.printf("MQTT: Publish online status: %s\n", MQTT_TOPIC_STATUS);
  publish(MQTT_TOPIC_STATUS, 1, true, MQTT_STATUS_PAYLOAD_ON);

  publishDeviceState();
}

void DeviceMqttClient::onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
  Serial.println();
  Serial.print("MQTT: Disconnected: ");
  if (reason == AsyncMqttClientDisconnectReason::TCP_DISCONNECTED) {
    Serial.println("TCP disconnected");
  } else if (reason == AsyncMqttClientDisconnectReason::MQTT_UNACCEPTABLE_PROTOCOL_VERSION) {
    Serial.println("Unacceptable protocol version");
  } else if (reason == AsyncMqttClientDisconnectReason::MQTT_IDENTIFIER_REJECTED) {
    Serial.println("Indentifier rejected");
  } else if (reason == AsyncMqttClientDisconnectReason::MQTT_SERVER_UNAVAILABLE) {
    Serial.println("Server unavailable");
  } else if (reason == AsyncMqttClientDisconnectReason::MQTT_MALFORMED_CREDENTIALS) {
    Serial.println("Malformed credentials");
  } else if (reason == AsyncMqttClientDisconnectReason::MQTT_NOT_AUTHORIZED) {
    Serial.println("Not authorized");
  } else {
    Serial.println("Unknown reason");
  }

  int8_t attemptToConnectCounter = 0;
  delay(3000);
  Serial.println("MQTT: Attemp to reconnect to the broker");
  while ( (connected() == false) && (attemptToConnectCounter < 100) ) {
    if (WiFi.isConnected()) {
      Serial.printf("MQTT: Attempt %d. Reconnecting to broker ... \n", attemptToConnectCounter);
      connect();
    } else {
      Serial.println("MQTT: WiFi is not connected. Wait.");
    }
    delay(3000);
    attemptToConnectCounter += 1;
  }
}


void DeviceMqttClient::onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
  Serial.println();
  Serial.println("MQTT: Message received.");
  Serial.printf("  topic: %s\n", topic);
  Serial.printf("  qos: %d\n", properties.qos);
  Serial.printf("  dup: %d\n", properties.dup);
  Serial.printf("  retain: %d\n", properties.retain);

  Serial.print("  payload: ");
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(payload);
  json.printTo(Serial);
  Serial.println();

  /* Set all received parameters to the device */
  Serial.print("MQTT: Parsing received JSON ... ");
  if (json.success()) {
    Serial.println("ok");

    /* Notification */
    if (std::string(topic) == "informer/set/notification") {
      std::vector<byte> icon;
      if (json.containsKey("icon")) {
        JsonArray &iconArray = json["icon"];
        if (iconArray.size() % 8 == 0) {
          for (auto &byteValue : iconArray) {
            icon.push_back(byteValue.as<byte>());
          }
        }
      }

      std::string textString = "";
      if (json.containsKey("text")) {
        textString = json["text"].as<const char*>();
      }

      int timeout = -1;
      if (json.containsKey("timeout")) {
        timeout = json["timeout"].as<int>();
      }

      m_device->setNotification(icon, textString, timeout);
    }

    /* Settings */
    if (std::string(topic) == "informer/set/settings") {
      if (json.containsKey("brightness")) {
        uint8_t brightness = json["brightness"].as<uint8_t>();
        m_device->setBrightness( brightness );
      }

      if (json.containsKey("state")) {
        m_device->setState( json["state"].as<bool>() );
      }

      if (json.containsKey("secondsVisible")) {
        m_device->setSecondsVisible( json["secondsVisible"].as<bool>() );
      }
    }

    /* Time */
    if (std::string(topic) == "informer/set/time") {
      uint8_t hour = 0;
      if (json.containsKey("hour")) {
        hour = json["hour"].as<uint8_t>();
      }

      uint8_t minute = 0;
      if (json.containsKey("minute")) {
        minute = json["minute"].as<uint8_t>();
      }

      uint8_t second = 0;
      if (json.containsKey("second")) {
        second = json["second"].as<uint8_t>();
      }

      uint8_t day = 0;
      if (json.containsKey("day")) {
        day = json["day"].as<uint8_t>();
      }

      uint8_t month = 0;
      if (json.containsKey("month")) {
        month = json["month"].as<uint8_t>();
      }

      uint16_t year = 0;
      if (json.containsKey("year")) {
        year = json["year"].as<uint16_t>();
      }

      bool dateIsValid = (day != 0) && (month != 0) && (year != 0);
      bool timeIsValid = ( (hour >= 0) && (hour < 24) ) &&
                         ( (minute >= 0) && (minute < 60) ) &&
                         ( (second >= 0) && (second < 60) );
      if (dateIsValid && timeIsValid) {
        m_device->setTime(hour, minute, second, day, month, year);
      }
    }

    /* Screens */
    if (std::string(topic) == "informer/set/screen") {
      bool idIsDefined = false;
      int id = 0;
      if (json.containsKey("id")) {
        id = json["id"].as<uint8_t>();
        idIsDefined = true;
      }

      std::vector<byte> icon;
      if (json.containsKey("icon")) {
        JsonArray &iconArray = json["icon"];
        if (iconArray.size() == 8) {
          for (auto &byteValue : iconArray) {
            icon.push_back(byteValue.as<byte>());
          }
        }
      }

      std::string textString = "";
      if (json.containsKey("text")) {
        textString = json["text"].as<const char*>();
      }

      if (idIsDefined) {
        m_device->setScreen(id, icon, textString);
      }
    }

  } else {
    Serial.println("failed");
  }
}


void DeviceMqttClient::publishDeviceState() {
  const int BUFFER_SIZE = JSON_OBJECT_SIZE(20);
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();

  // Light state: state, color, brightness
  root["state"] = m_device->state() ? "ON" : "OFF";
  root["brightness"] = m_device->brightness();

  // Additional parameters: IP-address, mac-address, RSSI, uptime, Firmware version
  // IP-address
  char ip[16];
  memset(ip, 0, 18);
  sprintf(ip, "%s", WiFi.localIP().toString().c_str());
  root["ip"] = ip;

  // MAC address
  uint8_t macAddr[6];
  WiFi.macAddress(macAddr);
  char mac[18];
  memset(mac, 0, 18);
  sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
  root["mac"] = mac;

  // RSSI
  char rssi[8];
  sprintf(rssi, "%d", WiFi.RSSI());
  root["rssi"] = rssi;

  // Uptime
  root["uptime"] = uptime( millis() );

  // Firmware version
  root["version"] = FIRMWARE_VERSION;

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

  Serial.printf("\nMQTT: Publish state: %s %s\n", MQTT_TOPIC_STATE, buffer);

  publish(MQTT_TOPIC_STATE, 0, true, buffer);
}
