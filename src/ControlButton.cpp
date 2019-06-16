#include "ControlButton.h"

ControlButton::ControlButton()
{
}


ControlButton::~ControlButton()
{
}


void ControlButton::init(uint8_t pin)
{
  m_pin = pin;
  pinMode(pin, INPUT);
  m_buttonStatus = m_defaultButtonStatus;
}

void ControlButton::run()
{
  if (digitalRead(m_pin) != m_buttonStatus) {
    // Debounce
    unsigned long start = millis();
    while ((millis() - start) < DEBOUNCE_DELAY) delay(1);

    if (digitalRead(m_pin) != m_buttonStatus) {
      m_buttonStatus = !m_buttonStatus;

      // released
      if (m_buttonStatus == m_defaultButtonStatus) {
        m_eventLength = millis() - m_eventStart;
        m_ready = true;
        m_eventPressAndHoldStart = 0;
      } else { // pressed
        m_eventStart = millis();
        m_eventPressAndHoldStart = m_eventStart;
        m_pressAndHoldEventDetected = false;
        m_eventLength = 0;
        if (m_resetCount) {
          m_eventCount = 1;
          m_resetCount = false;
        } else {
          m_eventCount++;
        }
        m_ready = false;
      }
    }
  }

  /* Detect a press-and-hold event */
  if ( ((millis() - m_eventPressAndHoldStart) > PRESS_AND_HOLD_DELAY) && (!m_ready) && (m_eventPressAndHoldStart > 0) ) {
    Serial.printf("ControlButton: Press And Hold event\n");
    m_eventPressAndHoldStart = millis();
    if (m_onPressAndHoldEvent) {
      this->m_onPressAndHoldEvent();
    }
    m_pressAndHoldEventDetected = true; // It prevents the release event after press-and-hold
  }

  /* A click is done */
  if (m_ready && ((millis() - m_eventStart) > REPEAT_DELAY) && (!m_pressAndHoldEventDetected)) {
    m_ready = false;
    m_resetCount = true;

    Serial.printf("ControlButton: Click duration: %d; Event count: %d\n", m_eventLength, m_eventCount);

    if (m_eventCount == 1) {
      if ((m_eventLength > 150) && m_onClickEvent) {
        this->m_onClickEvent();
      }
    } else if ((m_eventCount == 2) && m_onDoubleEvent) {
      this->m_onDoubleEvent();
    } else if ((m_eventCount == 3) && m_onTripleEvent) {
      this->m_onTripleEvent();
    }
  }
}
