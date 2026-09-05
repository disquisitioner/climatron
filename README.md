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

* [JC2432W328](JC2432W328 ) microcontroller board, one of the many varieties of ESP32-based boards often known as "CYD" (for "Cheap Yellow Display"). The JC2432W328 includes a ST7789 display controller, 320x240 TFT LCD display, and CST820 capacitive touchscreen controller as well as a number of useful connections for external devices.
* [Sensirion SEN66](https://sensirion.com/products/catalog/SEN66) air quality sensor, providing a wide range of environmental readings via an I2C interface.
* [Adafruit SEN6x breakout board](https://www.adafruit.com/product/6331) for the SEN66, adapting its special data and power connector to STEMMA QT/Qwiic for easy integration with the rest of Climatron's components.
* Three [NeoPixel](https://www.adafruit.com/product/1612) "smart" LEDs for additional feedback in Climatron's head.

### Software

Climatron is built using a variety of Arduino libraries.  All configurations require the following (and their dependencies):

* [WiFi Manager](https://github.com/tzapu/WiFiManager) by tzapu, available via the Arduino Library Manager
* [TFT_eSPI graphics library](https://github.com/Bodmer/TFT_eSPI) by Bodmer, available via the Arduino Library Manager
* [Sensirion I2C SEN66](https://github.com/Sensirion/arduino-i2c-sen66) by Sensirion, available via the Arduino Library Manager
* [Adafruit NeoPixel](https://github.com/adafruit/adafruit_neopixel) by Adafruit, available via the Arduino Library Manager
* [ArduinoJson](https://github.com/bblanchon/ArduinoJson) by Benoit Blanchon, available via the Arduino Library Manager
* [Time](https://github.com/PaulStoffregen/Time) by Michael Margolis and Paul Stoffregen, available via the Arduino Library Manager
* [Measure](https://github.com/disquisitioner/Measure) by David Bryant, downloaded and installed from GitHub
* [CST820 Touchscreen library](https://github.com/ericklein/CST820_Arduino_Library) by Eric Klein, downloaded and installed from GitHub

Optional endpoint services can be enabled to support MQTT, Home Assistant, InfluxDB and/or ThingSpeak integration and therefore may require any or all of the following depending on build settings (along with any dependencies):

* [PubSubClient](https://github.com/knolleary/pubsubclient) by Nick O'Leary, available via the Arduino Library Manager (for MQTT and Home Assisatant)
* [ThingSpeak](https://github.com/mathworks/thingspeak-arduino) by MathWorks, available via the Arduino Library Manager
* [InfluxDB](https://github.com/tobiasschuerg/InfluxDB-Client-for-Arduino) by Tobias Schürg, available via the Arduino Library Manager

#### Software Installation
Step 1: Ensure all of the libraries listed above are installed in your Arduino Libraries directory.

Step 2: The root Climatron project directory contains a directory called `"put in TFT_eSPI library folder"`. Inside this directory is a directory called `"TFT_eSPI_Setups"`. Copy the `TFT_eSPI_Setups` directory to the root of your `TFT_eSPI` directory. (You installed the `TFT_eSPI` directory into your Arduino Libraries directory during Step 1. If you need help finding your Arduino Libraries directory consult the official [project documentation](https://docs.arduino.cc/software/ide-v1/tutorials/installing-libraries/))

Step 3: In the TFT_eSPI library directory, there is a file named `"User_Setup_Select.h"`. Line 27 of the file is:
```
        #include <User_Setup.h>     // Default setup is root library folder
```
You should comment this line out and add the following line in its place:
```
        #include <TFT_eSPI_Setups/climatron_setup.h>
```
This directs TFT_eSPI configuration to the custom climatron setup you installed in Step 2. If you ever need to use TFT_eSPI to build a project other than Climatron, you can simply comment out the added line and uncomment the original line 27. This modification of a library file is undesirable, but is the suggested solution of the TFT_eSPI author until a better fix is available.

If TFT_eSPI is updated in the future, please repeat Step 3 before compiling Climatron again. If you remove and reinstall the TFT_eSPI library you will need to repeat Step 2 and Step 3.

## Building the Project
While you can build the project direcly from the code in the repository you'll want to customize it for your installation. Site-specific settings for confidential information like  API keys for cloud services need to be provided in the `secrets.h` file.  Visible deployment qualities, such as the which network service endpoints Climatron will report to, are managed through `config.h`.  This two-part approach minimizes the likelihood that confidential site-specific information will be shared through the project repository.

Additionally, Climatron includes a first-time setup procedure that enables managing device settings through creation of a local WiFi access point during
that process. Configuration settings provided in `secrets.h` and `config.h` will be presented as defaults in that setup procedure and can be changed if desired. This allows Climatron to be customized on-site without having to modify, compile, and reload firmware.

### Secrets.h
To establish confidential settings in `secrets.h` you should copy the `secrets_template.h` file in the repository to `secrets.h` and edit it to reflect your envirionment.  Comments there will guide you through that process and provide examples.  Note that you'll only need to provide values for network services you plan to use.  Minimally you should provide the latitude, longitude and altitude for Climatron's location and an API key for [OpenWeather Map](https://openweathermap.org/).  

WiFi credentials are managed through the first-time setup process built into Climatron and aren't based on having default values.

### Config.h
From a deployment perspective, `config.h` provides a single place to selectively enable and disable network endpoint services. Climatron is built to optionally report data to [ThingSpeak](https://thingspeak.mathworks.com/), [MQTT](https://mqtt.org/), [InfluxDB](https://www.influxdata.com/) and [HomeAssistant](https://www.home-assistant.io/). If you plan to use those services you will need to set the associated parameters in `secrets.h` following the information provided in comments there.

A separate setting in `config.h` controls whether Climatron runs in normal or debug mode. In debug mode much more information about ongoing operation is provided through serial output, e.g. to the Arduino IDE SerialMonitor, and values are reported more often.

Other settings in `config.h` specify hardware attributes, user interface values (e.g., colors), and operational parameters and can be left at their defaults.
