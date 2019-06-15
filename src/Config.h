#ifndef ESP_INFORMER_CONFIG_H
#define ESP_INFORMER_CONFIG_H

#include <string>
#include <Arduino.h>

#define FIRMWARE_VERSION "0.1.0"                    /* Firmware version */

#define INTERVAL_PUBLISH_STATE 600000               /* Interval to send statistics to the mqtt broker */

/* MQTT Settings */
#define MQTT_TOPIC_STATE "informer/state"           /* state report MQTT topic */
#define MQTT_TOPIC_SET "informer/set/#"             /* command MQTT topic */

#define MQTT_TOPIC_STATUS "informer/status"     /* status MQTT topic: online/offline */
#define MQTT_STATUS_PAYLOAD_ON "online"
#define MQTT_STATUS_PAYLOAD_OFF "offline"

#define MQTT_KEEP_ALIVE_SECONDS 30


/* WiFi Manager settings */
#define WIFI_AP_NAME "SmartInformer"
#define WIFI_AP_PASS "123456789"


/* Device settings */
#define  LEDMATRIX_CS_PIN D8

/* Number of 8x8 segments you are connecting */
#define  LEDMATRIX_SEGMENTS 8
#define  LEDMATRIX_WIDTH  LEDMATRIX_SEGMENTS * 8

/* RTC pins */
#define RTC_CLK_PIN D1
#define RTC_DAT_PIN D2
#define RTC_RST_PIN D3



/* Calculates uptime for the device */
inline char *uptime(unsigned long milli) {
  static char _return[32];
  unsigned long secs=milli/1000, mins=secs/60;
  unsigned int hours=mins/60, days=hours/24;
  milli-=secs*1000;
  secs-=mins*60;
  mins-=hours*60;
  hours-=days*24;
  sprintf(_return,"%dT%2.2d:%2.2d:%2.2d.%3.3d", (byte)days, (byte)hours, (byte)mins, (byte)secs, (int)milli);
  return _return;
}

#endif //ESP_INFORMER_CONFIG_H
