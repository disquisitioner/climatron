# Climatron and Home Assistant

An active area of Climatron project development is exploring how Climatron can interoperate with Smart Home hubs and other similar devices.  Climatron's array of sensors, logic in processing sensor readings over time, and ability to generate notifications all can be useful in having it be an active and beneficial partner to those devices and also to get more value out of Climatron itself.

Current work is focused on Climatron integration with [Home Assistant](https://www.home-assistant.io/), a popular open source home automation platform with a vibrant community of developers, enthusiasts, and partners. Tools and information related to integration with Home Assistant are gathered here and will be updated as development proceeds.

## Introduction

Home Assistant provides intefaces other devices and services can use to integrate with it as an automation platform and also provide a centralized user interface for smart home management. The most basic approach to device integration with Home Assistant is to use MQTT for communications, and manually add details about the messaging involved to Home Assistant's primary configuration file.  This approach and related helpful information is documented [here](https://www.home-assistant.io/integrations/mqtt).  (A more sophisticated and hands-on approach is to use Home Assistant's automatic discovery capabilities, which we're investigating now.)

Climatron's primary interaction with Home Assistant is in publishing sensor values, using MQTT. For a deeper understanding of how that works [this document](https://www.home-assistant.io/integrations/sensor.mqtt/) is helpful.  More on manual device integration with Home Assitant is provided [here](https://www.home-assistant.io/integrations/mqtt#manual-configured-mqtt-items), including modifying the primary Home Assistant configuration file to include Climatron. Configuration information for Home Assistant is defined in [YAML syntax](https://yaml.org/), which is human readable and easy to maintain but requires care in editing in order to properly adhere to YAML syntax.

To make that easier, especially given the many sensors Climatron provides, we've provided a Python script, `climatron_ha_config.py` that can take basic Climatron configuration parameters and generate the proper YAML definition to be incorporated in Home Assistant:

```
% python3 climatron_ha_config.py -d CLIMATRON_NAME -s SITE -l LOCATION -r ROOM -i CLIMATRON_ID
```

You can specify any string you like for `CLIMATRON_NAME`.  Values for `SITE`, `LOCATION`, and `ROOM` should be the same as defined in `secrets.h` for any particular build of the Climatron software, though can also be overdidden through the  WiFi Manager portal as part of initial device setup.  The value for `CLIMATRON_ID` should be the last four characters in the Climatron device's official identifier, e.g. `0a7e` for the device `Climatron-0a7e`.

For example:
```
% python3 climatron_ha_config.py -d "Kitchen Climatron" -s "myhouse" -l "indoor" -r "kitchen" -i "0a7e"
```

The `climatron_ha_config` script will generate the (somewhat length) configuration details for the specified Climatron device.  That output is then copied and inserted into Home Assistant's `configuration.yaml` configuration file as described [here](https://www.home-assistant.io/docs/configuration/). Those instructions also detail how to validate the configuration file and restart Home Assistant to use the newly provided information.