# esp8266_mqtt_doorbell_notifier

This is a little sketch I use to send notifications to home-assistant when my doorbell is triggered.

Using a Wemos D1 esp8266 board and a relay shield in between the AC doorbell trafo and the ringer
while connecting the doorbell button to the D1 pins, this humble little instance can controll
my ringer over MQTT as an alarm bell and send notifications to a MQTT topic when the button is pressed.

To prevent the doorbell from locking and ringing all day while I'm at work or annoying little neighbour kids
playing with the doorbells, throttling is activated after the button has been pressed multiple times.


## Pinout

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


## MQTT

### MQTT in and output

The doorbell button and ringer state are send to separate MQTT topics so you can use both for different automations.
When the ringer started or stopped, a json string is send to `home/doorbell/ring`.
To control the ringer, send your json payload to `home/doorbell/ring` as described below.

When the button is pressed or released a single `ON` or `OFF` payload is send to `home/doorbell/button`.

### Ringer modes when using MQTT

By enabling or disabling pulse mode you can choose whether the doorbell should make a constant ringing sound or in intervalse causing a pulse.
Using the time functions you can adjust the time the ring sounds (`duration`) in total, how long a single pulse should sound (`pulse_time`) and how long
the silence between pulses should be (`pulse_wait`)

### MQTT ringer control

To use the ringer as an alarm bell, you can publish a json payload to the configured MQTT topic.
Change the `MQTT_DOORBELL_TOPIC` and `MQTT_DOORBELL_TOPIC_SET` settings in `setings.h` to change the MQTT topic.

To activate the ringer, send a json payload to `home/doorbell/ring/set`:

```
mosquitto_pub -h $MQTT_BROKER -t "home/doorbell/ring/set" -m '{"state":"ON"}'
```

The keys and values you can use in your json string are:

| Key          |  Value        | Functionality |
| -------      | -------       | -------       |
| `state`      | `ON` or `OFF` | Turn the ringer ON or OFF. |
| `duration`   | number        | The time in milliseconds the ringer should sound |
| `pulse`      | 1 or 0        | Enable (1) or disable (0) pulse mode |
| `pulse_time` | number        | The duration of a ring pulse |
| `pulse_wait` | number        | The wait time in between ring pulses |

Only the `state` setting is required to enable or disable the ring, the others are optional.
If you leave out the key, the default values from `settings.h` are used.

After each ring cycle the settings are reset to their default value.

The latest value always takes presence, so if you publish data to set a ringer for 10 seconds, you can stop it any time by sending another command.


Examples:

```
## Play the ringer for 2 seconds
mosquitto_pub -h $MQTT_BROKER -t "home/doorbell/ring/set" -m '{"state":"ON","duration":2000}'

## Stop the ringer when playing
mosquitto_pub -h $MQTT_BROKER -t "home/doorbell/ring/set" -m '{"state":"OFF"}'

## Play the ringer for 5 seconds with short fast pulses
mosquitto_pub -h $MQTT_BROKER -t "home/doorbell/ring/set" -m '{"pulse":1,"duration":5000,"pulse_time":100,"pulse_wait":200,"state":"ON"}'

## Play 5 short rings after each other
mqtt-pub "home/doorbell/ring/set" '{"pulse":1,"duration":1000,"pulse_time":100,"pulse_wait_time":100,"state":"ON"}'
```

It won't work if you make the wait or pulse time longer than the duration of the pulse.
