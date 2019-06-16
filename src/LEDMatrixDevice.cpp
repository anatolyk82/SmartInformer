#include "LEDMatrixDevice.h"
#include "Font.h"
#include <string>

#include <Arduino.h>
#include "DS1302RTC.h"

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
  Notification *ntf = new Notification();
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
    m_text_x = 0;
    if (timeout > 0) {
      m_internalTimerTimeoutMilliseconds = timeout * 1000;
      m_internalTimerActive = true;
    }
    m_displayState = DisplayState::Notification;
  }

  m_notificationQueue.push(ntf); //Notification in the queue

}


void LEDMatrixDevice::setTime(uint8_t hour, uint8_t minute, uint8_t second, uint8_t day, uint8_t month, uint16_t year)
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
}


void LEDMatrixDevice::setSecondsVisible( const bool secondsVisible )
{
  m_driver->clear();
  m_secondsVisible = secondsVisible;
}


void LEDMatrixDevice::buttonPressAndHold()
{
  setState( !m_state );
}


void LEDMatrixDevice::run()
{
  time_t myTime;
  myTime = m_rtc->get();

  if ((m_internalTimerStart == 0) && (m_internalTimerActive)) {
    m_internalTimerStart = millis();
  }

  if (m_displayState == DisplayState::Time)
  {
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
  else if (m_displayState == DisplayState::Notification)
  {
    Notification *ntf = m_notificationQueue.front();

    const char *text = ntf->text.c_str();
    int len = ntf->text.length();

    if (len > (LEDMATRIX_SEGMENTS - 1)) {
      if ( --m_text_x < len * -8 ) {
    		m_text_x = (LEDMATRIX_WIDTH - 8);
    	}
      delay(40);
    } else {
      m_text_x = (((LEDMATRIX_SEGMENTS - 1) - len) / 2 + 1) * 8;
      delay(300);
    }

    drawString(text, len, m_text_x, 0);

    if (ntf->icon) {
      drawSprite(ntf->icon, 0, 0, 8, 8);
    } else {
      drawSprite(m_emptyIcon, 0, 0, 8, 8);
    }

  }
  else
  {
    m_driver->setPixel(0, 0, true);
    delay(1000);
  }

  /* Internal notification timer */
  if ( ((millis() - m_internalTimerStart) >= m_internalTimerTimeoutMilliseconds) &&
      (m_internalTimerStart > 0) &&
      m_internalTimerActive) {
    // Reset timer's parameters
    m_internalTimerTimeoutMilliseconds = 0;
    m_internalTimerStart = 0;
    m_internalTimerActive = false;

    // Release notification memory
    Notification *ntf = m_notificationQueue.front();
    // Release icon's memory
    if (ntf->icon) {
      delete [] ntf->icon;
    }
    delete ntf;

    //Remove the element from the queue
    m_notificationQueue.pop();

    if ( m_notificationQueue.empty() ) {
      m_displayState = DisplayState::Time;
    } else {
      m_displayState = DisplayState::Notification;
      // Prepare the screen and the timer for the next notification in the queue
      m_text_x = 0;
      if (m_notificationQueue.front()->timeout > 0) {
        m_internalTimerTimeoutMilliseconds = m_notificationQueue.front()->timeout * 1000;
        m_internalTimerActive = true;
      }
    }

    m_driver->clear();
  }

  m_driver->display();
}
