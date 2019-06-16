#ifndef ESP_INFORMER_BUTTON_H
#define ESP_INFORMER_BUTTON_H

#define DEBOUNCE_DELAY          50
#define REPEAT_DELAY            500
#define PRESS_AND_HOLD_DELAY    3000

#include <functional>
#include <Arduino.h>
#include "Config.h"

class ControlButton
{
public:
  ControlButton();
  ~ControlButton();

  /* Callback setters */
  void onClicked(std::function<void(void)> onClickEvent) {
    m_onClickEvent = onClickEvent;
  }
  void onDoubleClicked(std::function<void(void)> onDoubleEvent) {
    m_onDoubleEvent = onDoubleEvent;
  }
  void onTripleClicked(std::function<void(void)> onTripleEvent) {
    m_onTripleEvent = onTripleEvent;
  }
  void onPressAndHold(std::function<void(void)> onPressAndHoldEvent) {
    m_onPressAndHoldEvent = onPressAndHoldEvent;
  }

  /*
   * Init the button.
   * It must be called in setup()
   */
  void init(uint8_t);

  /*
   * Run the button.
   * It must be called in loop()
   */
  void run();

private:
  std::function<void(void)> m_onClickEvent = nullptr;
  std::function<void(void)> m_onDoubleEvent = nullptr;
  std::function<void(void)> m_onTripleEvent = nullptr;
  std::function<void(void)> m_onPressAndHoldEvent = nullptr;

  const uint8_t m_defaultButtonStatus = 0;
  uint8_t m_buttonStatus;
  uint8_t m_pin;

  unsigned long m_eventStart;
  unsigned long m_eventLength;
  bool m_resetCount = true;
  unsigned char m_eventCount = 0;
  bool m_ready = false;

  unsigned long m_eventPressAndHoldStart = 0;
  bool m_pressAndHoldEventDetected = false;
};

#endif //ESP_INFORMER_BUTTON_H
