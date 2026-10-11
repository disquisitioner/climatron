import argparse

# Parse command line arguments
parser = argparse.ArgumentParser()
parser.add_argument("-d","--device",default="Climatron", choices=("Climatron","PAQ"),help="Climatron or PAQ?")
parser.add_argument("-s","--site",help="Device site name",required=True)
parser.add_argument("-l","--location",help="Device location (e.g., indoor or outdoor)",required=True)
parser.add_argument("-r","--room",help="Device room (e.g., kitchen)",required=True)
parser.add_argument("-i","--id",help="Device identifier (e.g., 0a7e)",required=True)
parser.add_argument
args = parser.parse_args()

# Configuration templates used to generate configuration file entries.
#
# IMPORTANT NOTE: State topic layout and state payload configuration
# (value_template) referenced here should match how those values are 
# generated in the Climatron firmware.
comment = """
# Configuration for {deviceID} integration with Home Assistant"""

start = """mqtt:
  sensor:"""

temperature = """    - name: "Temperature"
      device_class: "temperature"
      unit_of_measurement: "°F"
      state_topic: "homeassistant/{site}/{location}/{room}/{deviceID}/state"
      unique_id: "{deviceID}-temperature"
      value_template: "{{{{ value_json.temperatureF }}}}"
      state_class: "measurement"
      {devicemap}"""

humidity = """    - name: "Humidity"
      device_class: "humidity"
      unit_of_measurement: "%"
      state_topic: "homeassistant/{site}/{location}/{room}/{deviceID}/state"
      unique_id: "{deviceID}-humidity"
      value_template: "{{{{ value_json.humidity }}}}"
      state_class: "measurement"
      {devicemap}"""

co2 = """    - name: "CO2"
      device_class: "carbon_dioxide"
      unit_of_measurement: "ppm"
      state_topic: "homeassistant/{site}/{location}/{room}/{deviceID}/state"
      unique_id: "{deviceID}-co2"
      value_template: "{{{{ value_json.co2 }}}}"
      state_class: "measurement"
      {devicemap}"""

pm25 = """    - name: "PM2.5"
      device_class: "pm25"
      state_topic: "homeassistant/{site}/{location}/{room}/{deviceID}/state"
      unit_of_measurement: "µg/m³"
      unique_id: "{deviceID}-pm25"
      value_template: "{{{{ value_json.pm25 }}}}"
      state_class: "measurement"
      {devicemap}"""

aqi = """    - name: "AQI"
      device_class: "aqi"
      state_topic: "homeassistant/{site}/{location}/{room}/{deviceID}/state"
      unique_id: "{deviceID}-aqi"
      value_template: "{{{{ value_json.aqi }}}}"
      state_class: "measurement"
      {devicemap}"""

voc = """    - name: "VOCIndex"
      device_class: "aqi"
      state_topic: "homeassistant/{site}/{location}/{room}/{deviceID}/state"
      unique_id: "{deviceID}-vocIndex"
      value_template: "{{{{ value_json.vocIndex }}}}"
      state_class: "measurement"
      {devicemap}"""

nox = """    - name: "NOxIndex"
      device_class: "aqi"
      state_topic: "homeassistant/{site}/{location}/{room}/{deviceID}/state"
      unique_id: "{deviceID}-noxIndex"
      value_template: "{{{{ value_json.noxIndex }}}}"
      state_class: "measurement"
      {devicemap}"""

# Generalized device map information to be incorporated in each sensor configuration
device = """device:
        name: "{devicename}"
        identifiers:
          - "{deviceID}" """

site = args.site
location = args.location
room = args.room
deviceID = args.device + "-" + args.id
devicename = args.device + " (" + args.id + ")"
devicemap = device.format(deviceID=deviceID,devicename=devicename)

# Generate configuration YAML for this device
print(comment.format(deviceID=deviceID))
print(start)
print(temperature.format(deviceID=deviceID, site=site, location=location, room=room, devicemap=devicemap))
print(humidity.format(deviceID=deviceID, site=site, location=location, room=room, devicemap=devicemap))
print(co2.format(deviceID=deviceID, site=site, location=location, room=room, devicemap=devicemap))
print(pm25.format(deviceID=deviceID, site=site, location=location, room=room, devicemap=devicemap))
print(aqi.format(deviceID=deviceID, site=site, location=location, room=room, devicemap=devicemap))
print(voc.format(deviceID=deviceID, site=site, location=location, room=room, devicemap=devicemap))
# Only have NOxIndex values if it's a Climatron
if args.device == "Climatron":
	print(nox.format(deviceID=deviceID, site=site, location=location, room=room, devicemap=devicemap))