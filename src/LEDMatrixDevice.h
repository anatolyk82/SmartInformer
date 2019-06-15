#ifndef ESP_LIGHT_DEVICE_CONTROL_H
#define ESP_LIGHT_DEVICE_CONTROL_H


#include <map>
#include "Config.h"

#include <Ds1302.h>
#include <LEDMatrixDriver.hpp>

class DS1302RTC;

class LEDMatrixDevice
{
public:
  LEDMatrixDevice();
  LEDMatrixDevice( const LEDMatrixDevice& ) = delete;
  ~LEDMatrixDevice();

  enum class DisplayState {
    None,
    Time,
    Notification
  };

  void setTime(uint8_t hour, uint8_t minute, uint8_t second, int8_t day = -1, int8_t month = -1, int16_t year = -1);
  void setNotification( byte *icon, const std::string &text, int timeout = -1 );

  /*
   * Run the device.
   * It must be called in loop()
   */
  void run();


  /* Getters */
  bool state() const { return m_state; }
  uint8_t brightness() const { return m_brightness; }

  /* Setters */
  void setState( const bool state );
  void setBrightness( uint8_t brightness );

private:
  void drawSprite( byte* sprite, int x, int y, int width, int height );
  void drawString( const char* text, int len, int x, int y );
  uint8_t flipByte( uint8_t c ) const;

  /* Properties */
  bool m_state = true;
  uint8_t m_brightness = 5;

  /* Devices */
  LEDMatrixDriver *m_driver = nullptr;
  DS1302RTC *m_rtc = nullptr;

  /* Displaying information */
  DisplayState m_displayState = DisplayState::None;

  /* Notifications */
  std::string m_notificationText;
  byte *m_notificationIcon = nullptr;
  byte m_emptyIcon[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  int m_text_x = 0;

  /* Internal timer */
  bool m_internalTimerActive = false;
  unsigned long m_internalTimerStart = 0;
  unsigned long m_internalTimerTimeoutMilliseconds = 0;

};

#endif //ESP_LIGHT_DEVICE_CONTROL_H
