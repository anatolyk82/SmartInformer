#include "LEDMatrixDevice.h"
#include <string>

#include "DS1302RTC.h" // https://github.com/iot-playground/Arduino/tree/master/external_libraries/DS1302RTC

LEDMatrixDevice::LEDMatrixDevice()
{
  m_driver = new LEDMatrixDriver(LEDMATRIX_SEGMENTS, LEDMATRIX_CS_PIN, LEDMatrixDriver::INVERT_DISPLAY_X | LEDMatrixDriver::INVERT_SEGMENT_X | LEDMatrixDriver::INVERT_Y );
  m_rtc = new DS1302RTC( RTC_RST_PIN, RTC_DAT_PIN,  RTC_CLK_PIN ); //CE, IO, CLK

  m_driver->setEnabled(true);
  m_driver->setBrightness(m_brightness); // 0 = low, 15 = high
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


void LEDMatrixDevice::setNotification( const std::vector<byte> &icon, const std::string &text, int timeout )
{
  std::shared_ptr<Notification> ntf = std::make_shared<Notification>();
  ntf->icon = std::move(icon);
  ntf->text = std::move(text);
  if (timeout > 0) {
    ntf->timeout = timeout;
  }

  if ( m_notificationQueue.empty() ) {
    m_driver->clear();
    m_textX = ntf->icon.size();
    if (timeout > 0) {
      m_notificationTimerTimeoutMilliseconds = timeout * 1000;
      m_notificationTimerActive = true;
    }
    m_displayState = DisplayState::Notification;
  }

  m_notificationQueue.push(ntf); // Notification in the queue

}


void LEDMatrixDevice::setScreen( uint8_t id, const std::vector<byte> &icon, const std::string &text )
{
  for (auto screen : m_screenList) {
    if (screen->id == id) {
      screen->icon = std::move(icon);
      screen->text = std::string(text);
      return;
    }
  }

  std::shared_ptr<Screen> scr = std::make_shared<Screen>();
  scr->id = id;
  scr->icon = std::move(icon);
  scr->text = std::move(text);
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
  if ( brightness >= 0 && brightness <= 15 ) {
    m_brightness = brightness;
    m_driver->setBrightness(brightness);
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
  } else if ( (m_screenIndex == (m_screenList.size() - 1)) && (m_displayState == DisplayState::Screen) ) {
    dismissScreen();
  } else if ((m_displayState == DisplayState::Screen) && (m_screenList.size() > 1)) {
    dismissScreen();
    m_displayState = DisplayState::Screen;
    m_screenTimerActive = true;
    m_screenTimerStart = millis();
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

  m_screenIndex = m_screenIndex < (m_screenList.size() - 1) ? m_screenIndex + 1 : 0;

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

  // Release notification memory: remove the element from the queue
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


int LEDMatrixDevice::run()
{
  int returnDelay = 0;
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
      m_driver->drawString(buf, 8, 0, 0);
      returnDelay = 500;
    } else {
      if (m_secondDelimiterVisible) {
        std::sprintf( buf, "%02d:%02d", hour(myTime), minute(myTime) );
      } else {
        std::sprintf( buf, "%02d %02d", hour(myTime), minute(myTime) );
      }
      m_secondDelimiterVisible = !m_secondDelimiterVisible;
      m_driver->drawString(buf, 5, 13, 0);
      returnDelay = 1000;
    }
  }
  else if (m_displayState == DisplayState::Screen)
  {
    bool iconExists = m_screenList.at(m_screenIndex)->icon.size() > 0;
    uint8_t screenLength = iconExists ? LEDMATRIX_SEGMENTS - 1 : LEDMATRIX_SEGMENTS;
    const char *text = m_screenList.at(m_screenIndex)->text.c_str();
    int textLength = m_screenList.at(m_screenIndex)->text.length();

    if (textLength > screenLength) {
      m_textX = (m_textX < -8 * textLength + 8 * iconExists) ? LEDMATRIX_WIDTH : (m_textX - 1);
    } else {
      m_textX = (screenLength - textLength) * 8 / 2 + 8 * iconExists;
    }

    m_driver->drawString( text, textLength, m_textX, 0 );
    if (iconExists) {
      m_driver->drawSprite(m_screenList.at(m_screenIndex)->icon.data(), 0, 0, 8, 8);
    }

    returnDelay = (textLength > screenLength ? 50 : 300);
  }
  else if (m_displayState == DisplayState::Notification)
  {
    bool iconExists = m_notificationQueue.front()->icon.size() > 0;
    uint8_t screenLength = iconExists ? LEDMATRIX_SEGMENTS - 1 : LEDMATRIX_SEGMENTS;
    const char *text = m_notificationQueue.front()->text.c_str();
    int textLength = m_notificationQueue.front()->text.length();

    if (textLength > screenLength) {
      m_textX = (m_textX < -8 * textLength + 8 * iconExists) ? LEDMATRIX_WIDTH : (m_textX - 1);
    } else {
      m_textX = (screenLength - textLength) * 8 / 2 + 8 * iconExists;
    }

    m_driver->drawString(text, textLength, m_textX, 0);
    if (iconExists) {
      m_driver->drawSprite(m_notificationQueue.front()->icon.data(), 0, 0, 8, 8);
    }

    returnDelay = (textLength > screenLength ? 50 : 300);
  }
  else
  {
    m_driver->setPixel(0, 0, true);
    returnDelay = 1000;
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
    m_screenIndex = 0;
  }

  m_driver->display();

  return returnDelay;
}
