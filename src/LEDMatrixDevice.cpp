#include "LEDMatrixDevice.h"
#include "Font.h"
#include <string>

#include <Arduino.h>
#include "DS1302RTC.h" // https://github.com/iot-playground/Arduino/tree/master/external_libraries/DS1302RTC

LEDMatrixDevice::LEDMatrixDevice()
{
  m_driver = new LEDMatrixDriver(LEDMATRIX_SEGMENTS, LEDMATRIX_CS_PIN, LEDMatrixDriver::INVERT_DISPLAY_X | LEDMatrixDriver::INVERT_SEGMENT_X | LEDMatrixDriver::INVERT_Y );
  m_rtc = new DS1302RTC( RTC_RST_PIN, RTC_DAT_PIN,  RTC_CLK_PIN ); //CE, IO, CLK

  m_driver->setEnabled(true);
  m_driver->setIntensity(m_brightness); // 0 = low, 10 = high
  m_driver->clear();

  m_displayState = DisplayState::Time;
}


LEDMatrixDevice::~LEDMatrixDevice()
{
  if (m_rtc) {
    delete m_rtc;
  }

  if (m_driver) {
    delete m_driver;
  }
}

void LEDMatrixDevice::drawSprite( byte* sprite, int x, int y, int width, int height )
{
  // The mask is used to get the column bit from the sprite row
  byte mask = B10000000;
  for( int iy = 0; iy < height; iy++ )
  {
    for( int ix = 0; ix < width; ix++ )
    {
      //lmd.setPixel(x + ix, y + iy, (bool)(sprite[iy] & mask ));
      m_driver->setPixel(x + ix, y + iy, (bool)(flipByte(sprite[iy]) & mask ));

      // Shift the mask by one pixel to the right
      mask = mask >> 1;
    }
    // Reset column mask
    mask = B10000000;
  }
}


void LEDMatrixDevice::drawString( const char* text, int len, int x, int y )
{
  for( int idx = 0; idx < len; idx ++ )
  {
    int c = text[idx] - 32;

    // stop if char is outside visible area
    if( x + idx * 8  > LEDMATRIX_WIDTH )
      return;

    // only draw if char is visible
    if( 8 + x + idx * 8 > 0 ) {
      drawSprite( font[c], x + idx * 8, y, 8, 8 );
    }

  }
}


uint8_t LEDMatrixDevice::flipByte(uint8_t c) const
{
  uint8_t r = 0;
  for (uint8_t i = 0; i < 8; i++) {
    r <<= 1;
    r |= c & 1;
    c >>= 1;
  }
  return r;
}


void LEDMatrixDevice::setNotification(byte *icon, const std::string &text, int timeout)
{
  std::shared_ptr<Notification> ntf = std::make_shared<Notification>();
  if (icon) {
    ntf->icon = icon;
  } else {
    ntf->icon = nullptr;
  }

  if (text != "") {
    ntf->text = text;
  }

  if (timeout > 0) {
    ntf->timeout = timeout;
  }

  if ( m_notificationQueue.empty() ) {
    m_driver->clear();
    m_textX = ntf->icon != nullptr ? 8 : 0;
    if (timeout > 0) {
      m_notificationTimerTimeoutMilliseconds = timeout * 1000;
      m_notificationTimerActive = true;
    }
    m_displayState = DisplayState::Notification;
  }

  m_notificationQueue.push(ntf); // Notification in the queue

}


void LEDMatrixDevice::setScreen( uint8_t id, byte *icon, const std::string &text )
{
  for (Screen* screen : m_screenList) {
    if (screen->id == id) {
      screen->icon = icon;
      screen->text = text;
      return;
    }
  }

  Screen *scr = new Screen();
  scr->id = id;
  scr->icon = icon;
  scr->text = text;
  m_screenList.push_back(scr);
}


void LEDMatrixDevice::setTime( uint8_t hour, uint8_t minute, uint8_t second, uint8_t day, uint8_t month, uint16_t year )
{
  tmElements_t tm;
  tm.Hour = hour > 23 ? 23 : hour;
  tm.Minute = minute > 59 ? 59 : minute;
  tm.Second = second > 59 ? 59 : second;
  tm.Day = day;
  tm.Month = month;
  tm.Year = year;
  m_rtc->write(tm);
}


void LEDMatrixDevice::setBrightness( uint8_t brightness )
{
  if ( brightness >= 0 && brightness < 11 ) {
    m_brightness = brightness;
    m_driver->setIntensity(brightness);
  }
}


void LEDMatrixDevice::setState( const bool state )
{
  m_driver->clear();
  m_state = state;
  m_displayState = state ? DisplayState::Time : DisplayState::None;
  m_switchOffAfterNotification = (m_displayState == DisplayState::None);
}


void LEDMatrixDevice::setSecondsVisible( const bool secondsVisible )
{
  m_driver->clear();
  m_secondsVisible = secondsVisible;
}


void LEDMatrixDevice::buttonClicked()
{
  if (m_displayState == DisplayState::Notification) {
    this->dismissNotification();
  } else if ((m_displayState == DisplayState::Time) && (m_screenList.size() > 0)) {
    m_displayState = DisplayState::Screen;
    m_screenTimerActive = true;
    m_screenTimerStart = millis();
    m_driver->clear();
  } else if ((m_displayState == DisplayState::Screen) && (m_screenList.size() > 1)) {
    dismissScreen();
    m_displayState = DisplayState::Screen;
    m_screenTimerActive = true;
    m_screenTimerStart = millis();
  } else if ((m_displayState == DisplayState::Screen) && (m_screenList.size() == 1)) {
    dismissScreen();
  }
}


void LEDMatrixDevice::buttonPressAndHold()
{
  setState( !m_state );
}


void LEDMatrixDevice::dismissScreen()
{
  // Reset timer's parameters
  m_screenTimerStart = 0;
  m_screenTimerActive = false;

  m_screenIndex = m_screenIndex < (m_screenList.size()-1) ? m_screenIndex + 1 : 0;

  if (m_displayState != DisplayState::Notification) {
    m_displayState = DisplayState::Time;
    m_driver->clear();
  }
}


void LEDMatrixDevice::dismissNotification()
{
  // Reset timer's parameters
  m_notificationTimerTimeoutMilliseconds = 0;
  m_notificationTimerStart = 0;
  m_notificationTimerActive = false;

  // Release notification memory
  // Release icon's memory
  if (m_notificationQueue.front()->icon) {
    delete [] m_notificationQueue.front()->icon;
  }
  //delete ntf;

  //Remove the element from the queue
  m_notificationQueue.pop();

  if ( m_notificationQueue.empty() ) {
    m_displayState = m_switchOffAfterNotification ? DisplayState::None : DisplayState::Time;
  } else {
    m_displayState = DisplayState::Notification;
    // Prepare the screen and the timer for the next notification in the queue
    m_textX = 0;
    if (m_notificationQueue.front()->timeout > 0) {
      m_notificationTimerTimeoutMilliseconds = m_notificationQueue.front()->timeout * 1000;
      m_notificationTimerActive = true;
    }
  }

  m_driver->clear();
}


void LEDMatrixDevice::run()
{

  if ((m_notificationTimerStart == 0) && (m_notificationTimerActive)) {
    m_notificationTimerStart = millis();
  }

  if ((m_screenTimerStart == 0) && (m_screenTimerActive)) {
    m_screenTimerStart = millis();
  }

  if (m_displayState == DisplayState::Time)
  {
    time_t myTime = m_rtc->get();
    char buf[8];
    if (m_secondsVisible) {
      std::sprintf( buf, "%02d:%02d:%02d", hour(myTime), minute(myTime), second(myTime) );
      drawString(buf, 8, 0, 0);
      delay(500);
    } else {
      if (m_secondDelimiterVisible) {
        std::sprintf( buf, "%02d:%02d", hour(myTime), minute(myTime) );
      } else {
        std::sprintf( buf, "%02d %02d", hour(myTime), minute(myTime) );
      }
      m_secondDelimiterVisible = !m_secondDelimiterVisible;
      drawString(buf, 5, 13, 0);
      delay(1000);
    }
  }
  else if (m_displayState == DisplayState::Screen)
  {
    Screen *scr = m_screenList.at(m_screenIndex);
    uint8_t screenLength = scr->icon ? LEDMATRIX_SEGMENTS - 1 : LEDMATRIX_SEGMENTS;
    const char *text = scr->text.c_str();
    int textLength = scr->text.length();

    if (textLength > screenLength) {
      m_textX = (m_textX < -8 * textLength + 8 * (scr->icon != nullptr)) ? LEDMATRIX_WIDTH : (m_textX - 1);
    } else {
      m_textX = ((screenLength - textLength) / 2 + 1) * 8;
    }

    if (scr->icon) {
      drawSprite(scr->icon, 0, 0, 8, 8);
    }
    drawString( text, textLength, m_textX, 0 );

    delay((textLength > screenLength ? 50 : 300));
  }
  else if (m_displayState == DisplayState::Notification)
  {
    bool iconExists = m_notificationQueue.front()->icon != nullptr;
    uint8_t screenLength = iconExists ? LEDMATRIX_SEGMENTS - 1 : LEDMATRIX_SEGMENTS;
    const char *text = m_notificationQueue.front()->text.c_str();
    int textLength = m_notificationQueue.front()->text.length();
    if (textLength > screenLength) {
      m_textX = (m_textX < -8 * textLength + 8 * iconExists) ? LEDMATRIX_WIDTH : (m_textX - 1);
    } else {
      m_textX = ((screenLength - textLength) / 2 + 1) * 8;
    }

    drawString(text, textLength, m_textX, 0);

    if (iconExists) {
      drawSprite(m_notificationQueue.front()->icon, 0, 0, 8, 8);
    }

    delay((textLength > screenLength ? 50 : 300));
  }
  else
  {
    m_driver->setPixel(0, 0, true);
    delay(1000);
  }

  /* The notification timer */
  if ( ((millis() - m_notificationTimerStart) >= m_notificationTimerTimeoutMilliseconds) &&
      (m_notificationTimerStart > 0) && m_notificationTimerActive) {
    dismissNotification();
  }

  /* The screen timer */
  if ( ((millis() - m_screenTimerStart) >= m_screenTimerTimeoutMilliseconds) &&
      (m_screenTimerStart > 0) && m_screenTimerActive) {
    dismissScreen();
  }

  m_driver->display();
}
