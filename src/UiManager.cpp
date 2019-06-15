#include "UiManager.h"

UiManager::UiManager() :
  m_mqtt_server(nullptr),
  m_mqtt_port(nullptr),
  m_mqtt_login(nullptr),
  m_mqtt_password(nullptr),
  m_mqtt_client_id(nullptr)
{
  m_mqtt_server = new char[40];
  m_mqtt_port = new char[6];
  m_mqtt_login = new char[64];
  m_mqtt_password = new char[64];
  m_mqtt_client_id = new char[64];
}


UiManager::~UiManager() {
  if (m_mqtt_server)
    delete [] m_mqtt_server;

  if (m_mqtt_port)
    delete [] m_mqtt_port;

  if (m_mqtt_login)
    delete [] m_mqtt_login;

  if (m_mqtt_password)
    delete [] m_mqtt_password;

  if (m_mqtt_client_id)
    delete [] m_mqtt_client_id;
}


bool UiManager::shouldSaveConfig = false;


void UiManager::saveConfigCallback () {
  UiManager::shouldSaveConfig = true;
}


void UiManager::initUIManager(bool _resetSettings) {

  /*if (!_resetSettings) {

  }*/
  this->readConfigurationFile();

  // The extra parameters to be configured
  WiFiManagerParameter custom_text("<br/><b>MQTT Server</b>");
  WiFiManagerParameter custom_mqtt_server("mqtt_server", "MQTT server", m_mqtt_server, 40); //id, placeholder, default value, length
  WiFiManagerParameter custom_mqtt_port("mqtt_port", "MQTT port", m_mqtt_port, 5);
  WiFiManagerParameter custom_mqtt_login("mqtt_login", "MQTT login", m_mqtt_login, 64);
  WiFiManagerParameter custom_mqtt_password("mqtt_password", "MQTT password", m_mqtt_password, 64);
  WiFiManagerParameter custom_mqtt_client_id("mqtt_client_id", "MQTT client id", m_mqtt_client_id, 64);

  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  // Set config save notify callback
  wifiManager.setSaveConfigCallback(&UiManager::saveConfigCallback);

  // Add all custom parameters
  wifiManager.addParameter(&custom_text);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_login);
  wifiManager.addParameter(&custom_mqtt_password);
  wifiManager.addParameter(&custom_mqtt_client_id);

  // Reset settings if needed
  if (_resetSettings) {
    wifiManager.resetSettings();
  }

  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  wifiManager.setMinimumSignalQuality();

  // Sets timeout until configuration portal gets turned off
  // useful to make it all retry or go to sleep in seconds
  wifiManager.setConfigPortalTimeout(60);

  // Fetches ssid and pass and tries to connect
  // if it does not connect it starts an access point with the specified name
  // and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect(WIFI_AP_NAME, WIFI_AP_PASS)) {
    Serial.println("Failed to connect to AP and hit timeout");
  }

  // If you get here you have connected to the WiFi
  Serial.println("The device is connected to WiFi");

  // Read updated parameters
  strcpy(m_mqtt_server, custom_mqtt_server.getValue());
  strcpy(m_mqtt_port, custom_mqtt_port.getValue());
  strcpy(m_mqtt_login, custom_mqtt_login.getValue());
  strcpy(m_mqtt_password, custom_mqtt_password.getValue());
  strcpy(m_mqtt_client_id, custom_mqtt_client_id.getValue());


  // Save the custom parameters to FS
  if (UiManager::shouldSaveConfig) {
    writeConfigurationFile();
  }
}



// Read config from ESP8266 FS
void UiManager::readConfigurationFile() {
  // Read configuration from FS json
  Serial.print("Mounting FS ... ");

  if (SPIFFS.begin()) {
    Serial.println("Ok");
    Serial.print("Check config file ... ");
    if (SPIFFS.exists("/config.json")) {
      // File exists, reading and loading
      Serial.print("Open config file ... ");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("Ok");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        Serial.print("Parsing JSON config ... ");
        if (json.success()) {
          Serial.println("Ok");

          strcpy(m_mqtt_server, json["mqtt_server"]);
          strcpy(m_mqtt_port, json["mqtt_port"]);
          strcpy(m_mqtt_login, json["mqtt_login"]);
          strcpy(m_mqtt_password, json["mqtt_password"]);
          strcpy(m_mqtt_client_id, json["mqtt_client_id"]);

        } else {
          Serial.println("Failed");
        }
      }
    } else {
      Serial.println("Failed. File /config.json not found");
    }
  } else {
    Serial.println("Failed");
  }

  Serial.println("Read data from the config file:");
  Serial.printf("  mqtt server: %s\n", m_mqtt_server);
  Serial.printf("  mqtt port: %s\n", m_mqtt_port);
  Serial.printf("  mqtt login: %s\n", m_mqtt_login);
  Serial.printf("  mqtt password: %s\n", m_mqtt_password);
  Serial.printf("  mqtt client id: %s\n", m_mqtt_client_id);
}


// Write configuration file
void UiManager::writeConfigurationFile() {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["mqtt_server"] = m_mqtt_server;
  json["mqtt_port"] = m_mqtt_port;
  json["mqtt_login"] = m_mqtt_login;
  json["mqtt_password"] = m_mqtt_password;
  json["mqtt_client_id"] = m_mqtt_client_id;

  Serial.print("Write configuration in /config.json ... ");
  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed");
  } else {
    Serial.println("Ok");
  }

  json.prettyPrintTo(Serial);
  json.printTo(configFile);
  Serial.println();
  configFile.close();
  UiManager::shouldSaveConfig = false;
}
