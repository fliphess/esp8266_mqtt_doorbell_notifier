
// * The GPIO pin for the doorbell button
#define BUTTON_PIN D5

// * The GPIO pin for the relay
#define RELAY_PIN D1

// * The Serial baud rate
#define BAUD_RATE 115200

// * MQTT network settings
#define MQTT_MAX_PACKET_SIZE 512

// * After 10 retry connects, cancel routine and return to loop
#define MQTT_MAX_RECONNECT_TRIES 10

// * The message to send when ON
#define ON_STATE "ON"

// * The message to send when OFF
#define OFF_STATE "OFF"

// * The hostname of the clock
#define HOSTNAME "doorbell.home"

// * The password used for uploading
#define OTA_PASSWORD "admin"

// * Webserver http port to listen on
#define HTTP_PORT 80

// * Wifi timeout in milliseconds
#define WIFI_TIMEOUT 30000

// * Watchdog settings
#define OSWATCH_RESET_TIME 300

// * Will be updated each loop
static unsigned long last_loop;

//
// * JSON Settings
//

const int JSON_BUFFER_SIZE = JSON_OBJECT_SIZE(4) + 150;
char JSON_OUTPUT_BUFFER[200];


//
// * MQTT Settings
//

// * To be filled with EEPROM data
char MQTT_HOST[64]   = "";
char MQTT_PORT[6]    = "";
char MQTT_USER[32]   = "";
char MQTT_PASS[32]   = "";

// * MQTT Last reconnection counter
long LAST_RECONNECT_ATTEMPT = 0;

// * The topics to get and set the doorbell ringer state
char *MQTT_DOORBELL_TOPIC     = (char *) "home/doorbell/ring";
char *MQTT_DOORBELL_TOPIC_SET = (char *) "home/doorbell/ring/set";

// * The topic where button updates are send
char *MQTT_BUTTON_TOPIC       = (char *) "home/doorbell/button";


//
// * Button Settings
//

// Will be set to 1 when the button is actively pressed
int BUTTON_PRESS_ACTIVE = 0;

// * Will be increased with 1 everytime the button is pressed
int BUTTON_PRESS_COUNT = 0;

// * The last time the button was pressed
unsigned long BUTTON_PRESS_TIMESTAMP;

// * The default wait time before the doorbell ring is retriggered
int BUTTON_RETRIGGER_TIME = 3000;                   // * 3 seconds

// * Default max times the doorbel is pressed before a delay is issued
int BUTTON_THROTTLE_MAX = 3;                        // * Max 3 times

// * The default wait time when the button has reached `DOORBELL_THROTTLE_MAX` times
unsigned long BUTTON_THROTTLE_TIME = 8000;          // * 8 seconds

//
// * Button Force Mode
//

// * Will be set with timestamp of Last time FORCE mode was used
unsigned long LAST_FORCED_PRESS;

// * The time before another force mode can be activated
unsigned long FORCE_MODE_WAIT_TIME = 30000;         // Wait 30 seconds before force can be used again

// * The time of a single long press to enable force mode
unsigned long FORCE_MODE_LONG_PRESS_TIME = 8000;   // * 8 seconds

// * The amount of presses to get a longer emergency ring
int FORCE_PRESSES = 11;                             // push the button for 11 times

// * The time an emergency rings takes
unsigned long FORCE_DURATION = 2000;                // 2 seconds

//
// * Bell Sequence Settings
//

// * Will be set to 1 when an alarm is active
int BELL_SEQUENCE_ACTIVE = 0;

// * The timestamp of when the alarm sequence was initiated
unsigned long BELL_SEQUENCE_STARTED;

// * The duration of the alarm sequence
unsigned long BELL_SEQUENCE_DURATION;

// * Will be set to 1 when the ring is actually on
int BELL_SEQUENCE_RING_ON = 0;

// * Will hold on/off timing of the doorbell ring
unsigned long BELL_SEQUENCE_RING_ON_TIME = 0;
unsigned long BELL_SEQUENCE_RING_OFF_TIME = 0;

//
// * Doorbel Ring Settings
//

// * The default time in milliseconds how long the bell should sound
unsigned long DOORBEL_RING_DURATION = 500;   // * 0.5 seconds

//
// * Pulse Mode
//

// * Will be set to one when pulse is active
int PULSE_ACTIVE = 0;

// Default pulse settings (used when not specified)
unsigned long DEFAULT_PULSE_TIME = 500;      // 0.5 seconds
unsigned long DEFAULT_PULSE_WAIT_TIME = 500; // 0.5 seconds

// * Will be overwritten when custom pulse timing is requested
unsigned long PULSE_TIME = 500;      // 0.5 seconds
unsigned long PULSE_WAIT_TIME = 500; // 0.5 seconds
