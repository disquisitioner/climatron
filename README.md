<div align="center">
	<h1>climatron</h1>
	<h3>Your Personal Air Quality Robot</h3>
</div>

![Climatrons](docs/images/climatrons-at-teardown2026.jpg)

The [Climatron Project](https://climatron.io) aims to make monitoring environmental air quality around
you easy, informative, and fun by blending a modern array of sensors with a
convenient touchscreen display, open source hardware and software, and a special fondness for [toy robots](https://tintoyrobots.com/).

Climatron uses a Sensirion SEN-66 sensor to sample its surroundings, measuring temperature, humidity, carbon dioxide levels, airborne particulates, nitrous oxide, and volatile organic compounds. Current values as well as graphs of recent measurements are available through the built-in touchscreen color display, along with a five-day weather forecast for the current location. Overall indication of air quality for each environmental factor is provided through LEDs built into the unit's head, ranging from green for good through yellow, orange and red for fair, poor and bad respectively. 

Here's a sample of Climatron's user interface (which you can also see in the photo above).

![Climatron screens](docs/images/climatron_screens-1200x218.png)

## Project Components

### Hardware

Climatron is built around the following hardware components:

* [JC2432W328](JC2432W328 ) microcontroller board, one of the many varieties of ESP32-based boards often known as "CYD" (for "Cheap Yellow Display"). The JC2432W328 includes a ST7789 display controller, 320x240 TFT LCD display, and CST820 capacitive touchscreen controller.
* [Sensirion SEN66](https://sensirion.com/products/catalog/SEN66) air quality sensor, providing a wide range of environmental readings via an I2C interface.
* [Adafruit SEN6x breakout board](https://www.adafruit.com/product/6331) for the SEN66, adapting its special data and power connector to STEMMA QT/Qwiic for easy integration with the rest of Climatron's components.
* Three [NeoPixel](https://www.adafruit.com/product/1612) "smart" LEDs for additional feedback in Climatron's head unit.

### Software

Climatron is built using a variety of Arduino libraries.  All configurations require the following:

* [WiFi Manager](https://github.com/tzapu/WiFiManager), available via the Arduino Library Manager
[TFT_eSPI graphics library](https://github.com/Bodmer/TFT_eSPI) by Bodmer, available via the Arduino Library Manager
* [Sensirion I2C SEN66](https://github.com/Sensirion/arduino-i2c-sen66) by Sensirion, available via the Arduino Library Manager
* [Adafruit NeoPixel](https://github.com/adafruit/adafruit_neopixel) by Adafruit, available via the Arduino Library Manager
* [ArduinoJson](https://github.com/bblanchon/ArduinoJson) by Benoit Blanchon, available via the Arduino Library Manager
* [Time](https://github.com/PaulStoffregen/Time), available via the Arduino Library Manager
* [Measure](https://github.com/disquisitioner/Measure) by David Bryant, downloaded and installed from GitHub
* [CST820 Touchscreen library](https://github.com/ericklein/CST820_Arduino_Library) by Eric Klein, downloaded and installed from GitHub

Optional endpoint services can be enabled to support MQTT, Home Assistant, InfluxDB and/or ThingSpeak integration and therefore may require any or all of the following depending on build settings:

* [PubSubClient](https://github.com/knolleary/pubsubclient) by Nick O'Leary, available via the Arduino Library Manager (for MQTT and Home Assisatant)
* [ThingSpeak](https://github.com/mathworks/thingspeak-arduino) by MathWorks, available via the Arduino Library Manager
* [InfluxDB](https://github.com/tobiasschuerg/InfluxDB-Client-for-Arduino) by Tobias Schürg, available via the Arduino Library Manager

