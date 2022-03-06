# smartDotMatrix
Multi-purpose dot matrix device
[Video](https://youtu.be/Hd10FrcIJx4)
# Abstract
8x8x4 ESP8266 powered MAX72xx dot-matrix smart clock. Controllable with web interface via LAN and infrared remote controller.


# Modes
- Clock
- Chronometer
- Calendar
- Thermometer [indoor]
- Humidometer [indoor]
- Weather forecast with sunrise-sunset times (needs an Internet connection and a dedicated server to run python codes in the "automationCode" folder)
- Thermometer [outdoor]  (needs an Internet connection and a dedicated server to run python codes in the "automationCode" folder)
- Display rate of common currencies  (needs an Internet connection and a dedicated server to run python codes in the "automationCode" folder)
- Display user-defined text message

# Features
- Night mode with adjustable schedule (turn off display)
- External EEPROM to save preferences
- Auto-restart every 10 day
- OTA compatibility for software updates
- Infrared remote controller
- Web interface


# Disclaimer & Apology Message
The base code is from before I realized writing readable code is essential. Therefore, it is a bit unreadable. Sorry for that.

# Some pictures

<img src="https://github.com/Mehmet-Emre-Dogan/smartDotMatrix/blob/main/clock.png"> </img>

<img src="https://github.com/Mehmet-Emre-Dogan/smartDotMatrix/blob/main/temperature.png"> </img>

# References
- https://maker.robotistan.com/dot-matrix-arduino-saat/
- https://randomnerdtutorials.com/esp8266-ota-updates-with-arduino-ide-over-the-air/
 --> External EEPROM reading-writing functions:
- https://dronebotworkshop.com/eeprom-arduino/
- https://arduino-projects4u.com/arduino-24c16/
 --> Webpage UI designs:
- https://mytectutor.com/esp8266-nodemcu-relay-control-from-web-page-over-wifi/
- https://www.w3schools.com/howto/howto_css_custom_checkbox.asp
