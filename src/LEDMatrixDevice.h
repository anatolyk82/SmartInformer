#ifndef ESP_LIGHT_DEVICE_CONTROL_H
#define ESP_LIGHT_DEVICE_CONTROL_H


#include <queue>
#include <vector>
#include <memory>
#include "Config.h"

#include <LEDMatrixDriver.hpp>

class DS1302RTC;

struct Notification
{
  std::vector<byte> icon;
  std::string text;
  int timeout;
};

struct Screen
{
  uint8_t id;
  std::vector<byte> icon;
  std::string text;
};

class LEDMatrixDevice
{
public:
  LEDMatrixDevice();
  LEDMatrixDevice( const LEDMatrixDevice& ) = delete;
  ~LEDMatrixDevice();

  enum class DisplayState {
    None,
    Time,
    Screen,
    Notification
  };

  void setTime( uint8_t hour, uint8_t minute, uint8_t second, uint8_t day, uint8_t month, uint16_t year );
  void setNotification( const std::vector<byte> &icon, const std::string &text, int timeout = -1 );
  void setScreen( uint8_t id, const std::vector<byte> &icon, const std::string &text );

  /*
   * Run the device.
   * It must be called in loop()
   */
  void run();


  /* Getters */
  bool state() const { return m_state; }
  uint8_t brightness() const { return m_brightness; }
  bool secondsVisible() const { return m_secondsVisible; }

  /* Setters */
  void setState( const bool state );
  void setBrightness( uint8_t brightness );
  void setSecondsVisible( const bool secondsVisible );

  /* Button's callbacks */
  void buttonClicked();
  void buttonPressAndHold();

private:
  void drawSprite( byte* sprite, int x, int y, int width, int height );
  void drawString( const char* text, int len, int x, int y );
  uint8_t flipByte( uint8_t c ) const;

  void dismissScreen();
  void dismissNotification();

  /* Properties */
  bool m_state = true;
  uint8_t m_brightness = 5;
  bool m_secondsVisible = true;
  bool m_secondDelimiterVisible = true;

  /* Devices */
  LEDMatrixDriver *m_driver = nullptr;
  DS1302RTC *m_rtc = nullptr;

  /* Displaying information */
  DisplayState m_displayState = DisplayState::None;
  bool m_switchOffAfterNotification = false;

  /* Notifications */
  std::string m_notificationText;
  byte *m_notificationIcon = nullptr;
  byte m_emptyIcon[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  int m_textX = 0;
  std::queue<std::shared_ptr<Notification>> m_notificationQueue;

  /* Notification timer */
  bool m_notificationTimerActive = false;
  unsigned long m_notificationTimerStart = 0;
  unsigned long m_notificationTimerTimeoutMilliseconds = 0;

  /* Screen */
  std::vector<std::shared_ptr<Screen>> m_screenList;
  uint8_t m_screenIndex = 0;

  bool m_screenTimerActive = false;
  unsigned long m_screenTimerStart = 0;
  unsigned long m_screenTimerTimeoutMilliseconds = 6000;
};

#endif //ESP_LIGHT_DEVICE_CONTROL_H
