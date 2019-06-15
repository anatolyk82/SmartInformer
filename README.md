# Smart Informer

 The project provides a firmware for a smart informer built with ESP8288 and 8x8 LED matrixes.
The idea is to get various information provided by Home Assistant. All information is sent by mqtt messages. Configuration of the device can be done over mqtt messages as well.

The informer has a esp8266 (Wemos R1 mini board), 8 led matrixes, RTC built on DS1302 and a button to control some functions manually.
The led matrixes communicate with ESP8266 via SPI interface with MAX7219. Connection pins can be configured in the file `Config.h`

DIN <--> D7
CS  <--> D8
CLK <--> D5


The real time clock module:
CLK <--> D1
CLK <--> D2
EN  <--> D3
