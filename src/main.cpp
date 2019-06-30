
#include "Config.h"
#include "UiManager.h"
#include "MqttClient.h"
#include "LEDMatrixDevice.h"
#include "ControlButton.h"

/* Create a UI manager */
UiManager uiManager;

/* MQTT Client instance */
DeviceMqttClient mqttClient;

/* Device control object */
LEDMatrixDevice *device;

/* Timer to publish the current state */
//SimpleTimer timer;
unsigned long timer_startTime = 0;

ControlButton button;


void setup() {

  /* Init serial port */
  Serial.begin(115200);
  Serial.println();

  //clean FS, for testing
  //SPIFFS.format();

  /* Initialize all devices */
  device = new LEDMatrixDevice();

  /* Create UI and connect to WiFi */
  uiManager.initUIManager(false);

  /* Configure MQTT */
  mqttClient.setDevice( device );
  int p = atoi( uiManager.mqttPort() );
  mqttClient.setServer( uiManager.mqttServer(), p );
  mqttClient.setCredentials( uiManager.mqttLogin(), uiManager.mqttPassword() );
  mqttClient.setKeepAlive( MQTT_KEEP_ALIVE_SECONDS );
  mqttClient.setWill( MQTT_TOPIC_STATUS, 1, true, MQTT_STATUS_PAYLOAD_OFF ); //topic, QoS, retain, payload
  String string_client_id( uiManager.mqttClientId() );
  string_client_id.trim();
  if (string_client_id != String("")) {
    mqttClient.setClientId( uiManager.mqttClientId() );
  }

  /* Set a callback to update the actual state of the device when an mqtt command is received */
  //mqttClient.onMessageReveived( std::bind(&LightDevice::updateDeviceState, &device) );

  /* Connect the MQTT client to the broker */
  int8_t attemptToConnectToMQTT = 0;
  Serial.println("MQTT: Connect to the broker");
  while ( (mqttClient.connected() == false) && (attemptToConnectToMQTT < 5) ) {
    if (WiFi.isConnected()) {
      Serial.printf("MQTT: Attempt %d. Connecting to broker [%s:%d]: login: [%s] password: [%s] ... \n", attemptToConnectToMQTT, uiManager.mqttServer(), p, uiManager.mqttLogin(), uiManager.mqttPassword() );
      mqttClient.connect();
    } else {
      //attemptToConnectToMQTT = 0;
      Serial.println("MQTT: WiFi is not connected. Try to reconnect WiFi.");
      WiFi.reconnect();
    }
    delay(3000);
    attemptToConnectToMQTT += 1;
  }

  /* If there is still no connection here, restart the device */
  if (!WiFi.isConnected()) {
    Serial.println("setup(): WiFi is not connected. Reset the device to initiate connection again.");
    ESP.restart();
  }

  if (!mqttClient.connected()) {
    Serial.println("setup(): The device is not connected to MQTT. Reset the device to initiate connection again.");
    ESP.restart();
  }

  /* Initialize the button */
  button.init(BUTTON_PIN);
  button.onClicked( std::bind(&LEDMatrixDevice::buttonClicked, device) );
  button.onPressAndHold( std::bind(&LEDMatrixDevice::buttonPressAndHold, device) );

  /* Publish device state periodicly */
  timer_startTime = millis();
}


void loop() {
  /* Periodic task to publish the current state via mqtt */
  unsigned long timer_currentTime = millis();
  if ( timer_currentTime > timer_startTime ) {
    if ( (timer_currentTime - timer_startTime) >= INTERVAL_PUBLISH_STATE ) {
      mqttClient.publishDeviceState();
      timer_startTime = timer_currentTime;
    }
  } else {
    timer_startTime = timer_currentTime;
  }

  int delayMillisecs = device->run();
  button.run();

  delay(delayMillisecs);

  /*if (WiFi.status() != WL_CONNECTED) {
    Serial.println("loop(): WiFi is not connected. Reset the device to initiate connection again.");
    ESP.restart();
  }*/
}
