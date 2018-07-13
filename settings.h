
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

// * The port the webserver listens on
#define HTTP_PORT 80

// * The password used for uploading
#define OTA_PASSWORD "admin"

// * Webserver http port to listen on
#define HTTP_PORT 80

// * Wifi timeout in milliseconds
#define WIFI_TIMEOUT 30000

// * Watchdog settings
#define OSWATCH_RESET_TIME 300

// * The topics to get and set the doorbell ringer state
#define MQTT_DOORBELL_TOPIC "home/doorbell/ring"
#define MQTT_DOORBELL_TOPIC_SET "home/doorbell/ring/set"

// * The topic where button updates are send
#define MQTT_BUTTON_TOPIC "home/doorbell/button"

// * Will be updated each loop
static unsigned long last_loop;

// * If set to true; eeprom is written
bool SAVE_WIFI_CONFIG                     = false;

//
// * JSON Settings
//

const int JSON_BUFFER_SIZE                = JSON_OBJECT_SIZE(4) + 150;

// * The buffer to convert arduinojson object to char*
char JSON_OUTPUT_BUFFER[200];

//
// * MQTT Settings
//

// * MQTT Last reconnection counter
long LAST_RECONNECT_ATTEMPT               = 0;

// * To be filled with EEPROM data
char MQTT_HOST[64]                        = "";
char MQTT_PORT[6]                         = "";
char MQTT_USER[32]                        = "";
char MQTT_PASS[32]                        = "";


//
// * Button Settings
//

// Will be set to 1 when the button is actively pressed
int BUTTON_PRESS_ACTIVE                   = 0;

// * Will be increased with 1 everytime the button is pressed
int BUTTON_PRESS_COUNT                    = 0;

// * The last time the button was pressed
unsigned long BUTTON_PRESS_TIMESTAMP;

// * Default max times the doorbel is pressed before throttling
int BUTTON_THROTTLE_MAX                   = 3;                        // * Max 3 times

// * The default wait time when the button has reached `BUTTON_THROTTLE_MAX` times
unsigned long BUTTON_THROTTLE_TIME        = 5000;                     // * 5 seconds

//
// * Bell Sequence Settings
//

// * Will be set to 1 when an alarm is active
int BELL_SEQUENCE_ACTIVE                  = 0;

// * Will be set to 1 when the ring is actually on
int BELL_SEQUENCE_RING_ON                 = 0;

// * The timestamp of when the alarm sequence was initiated
unsigned long BELL_SEQUENCE_STARTED;

// * The duration of the alarm sequence
unsigned long BELL_SEQUENCE_DURATION;

// * Will hold on/off timing of the doorbell ring
unsigned long BELL_SEQUENCE_RING_ON_TIME  = 0;
unsigned long BELL_SEQUENCE_RING_OFF_TIME = 0;

//
// * Default Settings
//

// * The default time in milliseconds how long the bell should sound
unsigned long DEFAULT_RING_DURATION       = 300;                      // * 0.3 seconds

// Default pulse settings (used when not specified)
int DEFAULT_PULSE                         = 0;
unsigned long DEFAULT_PULSE_TIME          = 500;                      // 0.5 seconds
unsigned long DEFAULT_PULSE_WAIT_TIME     = 500;                      // 0.5 seconds

//
// * Pulse Mode
//

// * Will be set to 1 when pulse is active
int PULSE_ACTIVE                          = DEFAULT_PULSE;

// * Will be overwritten when custom pulse timing is requested
unsigned long PULSE_TIME                  = DEFAULT_PULSE_TIME;       // 0.5 seconds
unsigned long PULSE_WAIT_TIME             = DEFAULT_PULSE_WAIT_TIME;  // 0.5 seconds
