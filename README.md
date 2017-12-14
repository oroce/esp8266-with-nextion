# esp8266-with-nextion

TODO: write proper readme.

This is the project: https://twitter.com/oroce/status/939961381331329029

# Starting

## ESP code changes

You need create a `config.json` in the `data` directory with the following content:

```
{
  "mqtt_server": "<mqtt host>",
  "mqtt_port": <mqtt port>,
  "mqtt_user": "<mqtt username>",
  "mqtt_password": "<mqtt password>"
}
```

Sidenote: I'm using cloudmqtt, it's a great tool


## Homeassistant changes

add a similar configuration to your the `configuration.yaml`:

```
automation:
  - alias: state sync for nextion
    hide_entity: True
    trigger:
      - platform: mqtt
        topic: nextion/request-state
      - platform: state
        entity_id: sensor.humidity_158d000153196d, sensor.temperature_158d000153196d, sensor.temperature_158d0001824749,sensor.humidity_158d0001824749,  sensor.temperature_158d0001823f4a,sensor.humidity_158d0001823f4a, sensor.frontdoor_status
    action:
      service: mqtt.publish
      data_template:
        topic: "nextion/reply-state"
        payload: "{\"bedroom-humidity\": {{ states.sensor.humidity_158d000153196d.state | string }}, \"bedroom-temperature\": {{ states.sensor.temperature_158d000153196d.state | string }}, \"living-room-temperature\": {{ states.sensor.temperature_158d0001824749.state | string }}, \"living-room-humidity\": {{ states.sensor.humidity_158d0001824749.state | string }}, \"loggia-temperature\": {{ states.sensor.temperature_158d0001823f4a.state | string }}, \"loggia-humidity\": {{ states.sensor.humidity_158d0001823f4a.state | string }}, \"frontdoor\": \"{{ states.sensor.frontdoor_status.state | string }}\" }"
```


The above configuration will publish state to `nextion/reply-state` topic when
- any of the sensors' state changed
- the esp8266 boots up (which triggers a new message in `nextion/request-state` topic)

The content of the message which gets published to `nextion/reply-state` is for me:

```
{
  "bedroom-humidity": 38.22,
  "bedroom-temperature": 23.13,
  "living-room-temperature": 23.02,
  "living-room-humidity": 39.35,
  "loggia-temperature": 5.81,
  "loggia-humidity": 81.86,
  "frontdoor": "closed"
}
```

# TODOS

- add license
- proper docs
- better json structure

