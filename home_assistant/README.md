# Climatron and Home Assistant

An active area of Climatron project development is exploring how Climatron can interoperate with Smart Home hubs and other similar devices.  Climatron's array of sensors, logic in processing sensor readings over time, and ability to generate notifications all can be useful in having it be an active and beneficial partner to those devices and also to get more value out of Climatron itself.

Current work is focused on Climatron integration with [Home Assistant](https://www.home-assistant.io/), a popular open source home automation platform with a vibrant community of developers, enthusiasts, and partners. Tools and information related to integration with Home Assistant are gathered here and will be updated as development proceeds.

## Introduction

Home Assistant provides intefaces that other devices and services can use to interoperate with it as an automation platform and also in providing a centralized user interface for smart home management. To do so, devices need to be describe to Home Assistant what they are capable of doing and must be able to communicate with it in an ongoing way to exchange data and commands.

Climatron already supports MQTT for general purpose publish/subscribe communications, so can extend how it uses it to also publish data to Home Assistant. Helpful information on how this is done is documented [here](https://www.home-assistant.io/integrations/mqtt). Home Assistant expects devices to use MQTT in ways that reflect the nature of the device, given a fixed set of device types that Home Assistant understands.  Climatron, to Home Assistant, is a single device that includes a number of sensors so it needs to communicate in ways consistent with Home Assistant's expectations of sensors as described [here](https://www.home-assistant.io/integrations/sensor.mqtt/). Features accomplishing this in Climatron can be enabled through a setting in Climatron's `config.h` configuration file.

That leaves the matter of providing Home Assistant with a proper set of configuration details including what specific sensors Climatron contains and how it will publish their values. The simplest way to do this is by manually adding those details to Home Assistant's master configuration file. Home Assistant's documentation describes that process [here](https://www.home-assistant.io/integrations/mqtt#manual-configured-mqtt-items). (A more sophisticated and less manual approach is to use Home Assistant's automatic discovery capabilities, which we're investigating for Climatron now.)

Home Assistant expects device configuration information to be provided using a special syntax called ["YAML"]https://yaml.org/), which is human readable and easy to maintain but requires care in editing in order to properly adhere to YAML syntax. To make that easier, especially given the many sensors Climatron provides, we've developed a Python script that can take basic Climatron configuration parameters and generate the proper YAML definition to be added to Home Assistant's master configuration file.

## Generating Climatron's Configuration

The `climatron_ha_config.py` script found here takes key information specific to any particular Climatron and its deployment setting and generates as output the YAML configuration details that need to be added to Home Assistant's `configuration.yaml` file so it will recognize and interoperate with it.  You use the script as follows:

```
% python3 climatron_ha_config.py -d Climatron -s SITE -l LOCATION -r ROOM -i CLIMATRON_ID
```

Values for `SITE`, `LOCATION`, and `ROOM` **must** match those same values as configured in the associated Climatron unit. This is because those values are used by the software onboard Climatron to generate the MQTT topic to which it publishes sensor values, and Home Assistant must subscribe to that same topic to receive and process those values. Default values for any build of the Climatron software are configured in `secrets.h`. If you modified those values as part of Climatron's WiFi Manager setup you'll need to use the updated settings.  (In case you aren't sure of those settings you can watch for Climatron's published reports using Home Assistant's MQTT broker settings page at Settings -> Devices & Services -> MQTT, then click on the gear icon to reach the settings page, and use "Listen to a topic" to subscribe to the topic "#" which will show you every message the MQTT broker receives.)

The value for `CLIMATRON_ID` should be the last four characters in the Climatron device's official identifier, e.g. `0a7e` for the device `Climatron-0a7e`.

For example:
```
% python3 climatron_ha_config.py -d Climatron -s "myhouse" -l "indoor" -r "kitchen" -i "0a7e"
```

The (lengthy) output from the script should then be copied and inserted into Home Assistant's `configuration.yaml` configuration file as described [here](https://www.home-assistant.io/docs/configuration/). Those instructions also detail how to validate the configuration file and restart Home Assistant to use the newly provided information. 
