# esp8266 Doorbell Notifier over MQTT

This is a little sketch I use to send notifications to home-assistant when my doorbell is triggered.

Using a Wemos D1 esp8266 board and a relay shield in between the AC doorbell trafo and the ringer
while connecting the doorbell button to the esp8266 pins, this humble little instance can controll
my ringer over MQTT and HTTP as an alarm bell and send notifications to a MQTT topic when the button is pressed.

To prevent the doorbell from locking and ringing all day while I'm at work or annoying little neighbour kids
playing with the doorbells, throttling is activated after the button has been pressed multiple times.

Additionally you can disable the button over mqtt and http in case of a nasty hangover.


## Setup

Connect the relay shield to the Wemos D1 using the header pins and connect the doorbell button to GND and D5.
For more info see the pinout.

### Pinout

Relay:

| Esp8266  | Relay |
| ----     | ----  |
| VCC (5V) | 5V    |
| GND      | GND   |
| D1       | CTL   |

Button:

| Esp8266  | Button |
| ----     | ----   |
| GND      | WIRE 1 |
| D5       | WIRE 2 |


### Flashing

To flash the sketch to the nodemcu i used the arduino IDE, but with minor adjustments you can use platformio as well.
I'll create some configuration later to make this more accessible. (A PR is welcome)

## API

You can get and set the state of the ringer over http and mqtt.
As there is not much use to it, it's not possible to trigger the button itself over mqtt or http.

### Ringer modes

By enabling or disabling pulse mode you can choose whether the doorbell should make a constant ringing sound or in intervalse causing a pulse.
Using the time functions you can adjust the time the ring sounds (`duration`) in total, how long a single pulse should sound (`pulse_time`) and how long
the silence between pulses should be (`pulse_wait`)


### Json

To use the ringer as an alarm bell, you can publish or post a json payload to the configured MQTT topic or http endpoint.

The keys and values you can use in your json string are:

| Key               |  Value        | Functionality                                    |
| -------           | -------       | -------                                          |
| `state`           | `ON` or `OFF` | Turn the ringer ON or OFF.                       |
| `duration`        | number        | The time in milliseconds the ringer should sound |
| `pulse`           | `1` or `0`        | Enable (1) or disable (0) pulse mode         |
| `pulse_time`      | number        | The duration of a ring pulse                     |
| `pulse_wait`      | number        | The wait time in between ring pulses             |
| `button_disabled` | `1` or `0`    | Enable or Disable the button                     |

Only the `state` setting is required to enable or disable the ring, the others are optional.

If you don't implicitly set the pulse settings and or the duration, the default values from `settings.h` are used.

After each ring cycle the settings are reset to their default value.
The latest value always takes presence, so if you publish data to set a ringer for 10 seconds, you can stop it any time by sending another command over both mqtt or http.

If you change the timing settings while ringing, some unexpected behaviour can appear as this is changed on the fly.

Pulse won't work if you make the wait or pulse time longer than the duration of the pulse.


### HTTP

You can post json or retrieve the status of the doorbell ringer using http requests:

*Get:*
```
## Get the doorbell ringer state
curl http://door.bell
```

The ringer is controllable over http by posting json to the d1:

*Set:*

```
function post() {
    curl --header 'Content-Type: application/json' -X POST \
        --data "$@" http://door.bell/set
}

## Turn on with pulse
post '{"state":"ON","duration":1500,"pulse":1,"pulse_time":500,"pulse_wait":500}'

## Turn on long
post '{"state":"ON","duration":60000}'

## Turn off
post '{"state":"OFF"}'

## Disable the button
post '{"button_disabled":1}'

## Enable the button again
post '{"button_disabled":0}'

```

## Enable disable button

There are multiple ways to disable the button.
You can either use the json method, that is available for mqtt as well, as both methods share the same json parser, or use the dedicated http endpoints:

```
## Disable the button
curl http://door.bell/disable

## Enable the button again
curl http://door.bell/enable

```

### MQTT

The doorbell button and ringer state are send to separate MQTT topics so you can use both for different automations.
When the ringer started or stopped, a json string is send to `home/doorbell/ring`.
To control the ringer, send your json payload to `home/doorbell/ring/set` as described below.

When the button is pressed or released a single `ON` or `OFF` payload is send to `home/doorbell/button`.

You can change the topics and some other settings in `settings.h`

*Set:*

```
function publish() {
    mosquitto_pub -h $MQTT_BROKER -t "home/doorbell/ring/set" -m "$@"
}

## Turn on with pulse
publish '{"state":"ON","duration":1500,"pulse":1,"pulse_time":500,"pulse_wait":500}'

## Turn on long
publish '{"state":"ON","duration":60000}'

## Turn off
publish '{"state":"OFF"}'

## Disable the button
publish '{"button_disabled":1}'

## Enable the button again
publish '{"button_disabled":0}'

```

The latest state of the doorbell ringer will be send to the MQTT state topic on every change.


## Home Assistant Config

Have a look at the yaml files in hass/ to get an idea how to integrate in home assistant


