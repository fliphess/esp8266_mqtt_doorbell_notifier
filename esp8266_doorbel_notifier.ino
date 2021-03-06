#include <FS.h>
#include <EEPROM.h>
#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Ticker.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <Bounce2.h>

#include "settings.h"

// * Include the PubSubClient after setting mqtt packet size in settings.h
#include <PubSubClient.h>

// Initiate the watchdog ticker
Ticker tickerOSWatch;

// * Initiate led blinker library
Ticker ticker;

// * Initiate WIFI client
WiFiClient espClient;

// * Initiate MQTT client
PubSubClient mqtt_client(espClient);

// * Initiate HTTP server
ESP8266WebServer webserver (HTTP_PORT);

// * Initiate button debouncer
Bounce debouncer = Bounce();

// **********************************
// * Watchdog                       *
// **********************************

void ICACHE_RAM_ATTR osWatch(void)
{
    unsigned long t = millis();
    unsigned long last_run = abs(t - last_loop);

    if (last_run >= (OSWATCH_RESET_TIME * 1000))
        ESP.restart();
}

// **********************************
// * WIFI                           *
// **********************************

// * Gets called when WiFiManager enters configuration mode
void wifi_ap_mode_callback(WiFiManager *myWiFiManager)
{
    Serial.println(F("Entered config mode"));
    Serial.println(WiFi.softAPIP());

    // * If you used auto generated SSID, print it
    Serial.println(myWiFiManager->getConfigPortalSSID());

    // * Entered config mode, make led toggle faster
    ticker.attach(0.2, led_tick);
}

// **********************************
// * Ticker (System LED Blinker)    *
// **********************************

// * Blink on-board Led
void led_tick()
{
    // * Toggle state
    int state = digitalRead(LED_BUILTIN);    // * Get the current state of GPIO1 pin
    digitalWrite(LED_BUILTIN, !state);       // * Set pin to the opposite state
}

// **********************************
// * Webserver                      *
// **********************************

// * Handle json input string
void webserver_handle_json()
{
    // * Handle json input from http webserver
    String json_input = webserver.arg("plain");

    Serial.print(F("HTTP Incoming: "));
    Serial.println(json_input);

    bool result = process_json_input((char*) json_input.c_str());

    if (result)
        webserver.send(200, "application/json", F("{\"success\":\"true\",\"message\":\"OK\"}"));
    else
    {
        Serial.println(F("Error input from HTTP input: parseObject() failed."));
        webserver.send(500, "application/json", F("{\"success\":\"false\",\"message\":\"Update Failed\"}"));
    }
}

// * Return the status of the ringer as a json string
void webserver_handle_status()
{
    // * Update the current state
    update_json_output_buffer();

    // * Send the current json status to the client
    webserver.send(200, "application/json", JSON_OUTPUT_BUFFER);
}

// * Set the button to disabled
void webserver_handle_disable()
{
    BUTTON_DISABLED = 1;
    button_turned_off();
    webserver.send(200, "text/plain", F("Button Disabled"));
}

// * Set the button to enabled
void webserver_handle_enable()
{
    BUTTON_DISABLED = 0;
    webserver.send(200, "text/plain", F("Button Enabled"));
}


// * Return a 404 when the page does not exist
void webserver_handle_not_found()
{
    // * Handle 404 not found error
    webserver.send(404, "text/plain", F("File Not Found"));
}

// * Setup the webserver
void setup_webserver()
{
    Serial.println(F("HTTP Webserver setup"));

    webserver.on("/",        webserver_handle_status);
    webserver.on("/set",     webserver_handle_json);
    webserver.on("/enable",  webserver_handle_enable);
    webserver.on("/disable", webserver_handle_disable);

    webserver.onNotFound(webserver_handle_not_found);
    webserver.begin();

    Serial.println(F("HTTP Webserver Started"));
}

// **********************************
// * MQTT                           *
// **********************************

void update_json_output_buffer()
{
    StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();

    root["state"] = (BELL_SEQUENCE_ACTIVE == 1) ? ON_STATE : OFF_STATE;
    root["button_disabled"] = BUTTON_DISABLED;
    root["duration"] = BELL_SEQUENCE_DURATION;
    root["pulse"] = PULSE_ACTIVE;
    root["pulse_time"] = PULSE_TIME;
    root["pulse_wait"] = PULSE_WAIT_TIME;

    root.printTo(JSON_OUTPUT_BUFFER, sizeof(JSON_OUTPUT_BUFFER));
}


// * Update button to mqtt topic
void send_button_state_to_broker()
{
    const char *button_state = (BUTTON_PRESS_ACTIVE == 1) ? ON_STATE : OFF_STATE;

    if (!mqtt_client.publish(MQTT_BUTTON_TOPIC, button_state))
        Serial.println(F("Failed to publish button state to broker"));
}


// * Publish the state of the doorbell to the mqtt broker
void send_mqtt_ring_state()
{
    update_json_output_buffer();

    Serial.print(F("MQTT Outgoing: "));
    Serial.println(JSON_OUTPUT_BUFFER);

    bool result = mqtt_client.publish(MQTT_DOORBELL_TOPIC, JSON_OUTPUT_BUFFER, true);

    if (!result)
        Serial.println(F("MQTT publish failed "));
}

// * Reconnect to MQTT server and subscribe to in and out topics
bool mqtt_reconnect()
{
    // * Loop until we're reconnected
    int MQTT_RECONNECT_RETRIES = 0;

    while (!mqtt_client.connected() && MQTT_RECONNECT_RETRIES < MQTT_MAX_RECONNECT_TRIES)
    {
        MQTT_RECONNECT_RETRIES++;

        Serial.printf("MQTT connection attempt %d / %d ...\n", MQTT_RECONNECT_RETRIES, MQTT_MAX_RECONNECT_TRIES);

        // * Attempt to connect
        if (mqtt_client.connect(HOSTNAME, MQTT_USER, MQTT_PASS))
        {
            Serial.println(F("MQTT connected!"));

            // * Once connected, publish an announcement...
            char *message = new char[20 + strlen(HOSTNAME) + 1];
            strcpy(message, "doorbel alive: ");
            strcat(message, HOSTNAME);
            mqtt_client.publish("hass/status", message);

            // * Resubscribe to the mqtt topic
            mqtt_client.subscribe(MQTT_DOORBELL_TOPIC_SET);

            Serial.print(F("MQTT button topic out: "));
            Serial.println(MQTT_BUTTON_TOPIC);

            Serial.print(F("MQTT doorbell topic out: "));
            Serial.println(MQTT_DOORBELL_TOPIC);

            Serial.print(F("MQTT doorbell topic in: "));
            Serial.println(MQTT_DOORBELL_TOPIC_SET);
        }
        else
        {
            Serial.print(F("MQTT Connection failed: rc="));
            Serial.println(mqtt_client.state());
            Serial.println(F(" Retrying in 5 seconds"));

            // * Wait 5 seconds before retrying
            delay(5000);
        }
    }

    if (MQTT_RECONNECT_RETRIES >= MQTT_MAX_RECONNECT_TRIES)
    {
        Serial.printf("*** MQTT connection failed, giving up after %d tries ...\n", MQTT_RECONNECT_RETRIES);
        return false;
    }

    return true;
}

// * Process incoming json
bool process_json_input(char *payload)
{
    StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
    JsonObject &root = jsonBuffer.parseObject(payload);

    if (root.success())
    {
        int pulse = 0;
        unsigned long duration = 0;
        unsigned long pulse_time = DEFAULT_PULSE_TIME;
        unsigned long pulse_wait = DEFAULT_PULSE_WAIT_TIME;

        if (root.containsKey("button_disabled"))
        {
            if (root["button_disabled"] == 1)
                BUTTON_DISABLED = 1;
            else
                BUTTON_DISABLED = 0;
        }

        if (root.containsKey("pulse_wait"))
            pulse_wait = (unsigned long) root["pulse_wait"];

        if (root.containsKey("pulse_time"))
            pulse_time = (unsigned long) root["pulse_time"];

        if (root.containsKey("pulse"))
        {
            if (root["pulse"] == 1)
                pulse = 1;
            else
                pulse = 0;
        }

        if (root.containsKey("duration"))
            duration = root["duration"];

        if (root.containsKey("state"))
        {
            if (strcmp(ON_STATE, root["state"]) == 0)
                start_doorbell_sequence(duration, pulse, pulse_wait, pulse_time);
            else if (strcmp(OFF_STATE, root["state"]) == 0)
                stop_doorbell_sequence();
        }

        return true;
    }

    return false;
}

// * Callback for incoming MQTT messages
void mqtt_callback(char *topic, byte *payload_in, unsigned int length)
{
    char *payload = (char *)malloc(length + 1);
    memcpy(payload, payload_in, length);
    payload[length] = 0;

    Serial.printf("MQTT Incoming on %s: ", topic);
    Serial.println(payload);

    bool result = process_json_input(payload);
    free(payload);

    // * If processing ran successfully, send state to mqtt out topic
    if (!result)
        Serial.println(F("Error input from MQTT broker: parseObject() failed."));
}

// * Manage mqtt connection or reconnect if down
void mqtt_loop()
{
    if (!mqtt_client.connected())
    {
        long now = millis();

        if (now - LAST_RECONNECT_ATTEMPT > 5000)
        {
            LAST_RECONNECT_ATTEMPT = now;

            if (mqtt_reconnect())
                LAST_RECONNECT_ATTEMPT = 0;
        }
    }
    else
        mqtt_client.loop();
}

// **********************************
// * Doorbell ring functions        *
// **********************************

void doorbell_turn_on()
{
    digitalWrite(RELAY_PIN, HIGH);
    Serial.println(F("Doorbell ring turned ON"));

    BELL_SEQUENCE_RING_ON = 1;
    BELL_SEQUENCE_RING_ON_TIME = millis();
}

void doorbell_turn_off()
{
    digitalWrite(RELAY_PIN, LOW);
    Serial.println(F("Doorbell ring turned OFF"));

    BELL_SEQUENCE_RING_ON = 0;
    BELL_SEQUENCE_RING_OFF_TIME = millis();
}

void stop_doorbell_sequence()
{
    doorbell_turn_off();

    BELL_SEQUENCE_ACTIVE = 0;
    BELL_SEQUENCE_DURATION = 0;

    // * Reset pulse settings to default
    PULSE_ACTIVE    = DEFAULT_PULSE;
    PULSE_TIME      = DEFAULT_PULSE_TIME;
    PULSE_WAIT_TIME = DEFAULT_PULSE_WAIT_TIME;

    send_mqtt_ring_state();
}

void start_doorbell_sequence(unsigned long duration, int pulse, unsigned long pulse_wait, unsigned long pulse_time)
{
    doorbell_turn_on();

    BELL_SEQUENCE_ACTIVE = 1;
    BELL_SEQUENCE_STARTED = millis();
    BELL_SEQUENCE_DURATION = duration;

    if (pulse == 1)
    {
        PULSE_ACTIVE = 1;
        PULSE_TIME = pulse_time;
        PULSE_WAIT_TIME = pulse_wait;
    }

    send_mqtt_ring_state();
}

void doorbell_ring_loop()
{
    if (BELL_SEQUENCE_ACTIVE == 1)
    {
        if (BELL_SEQUENCE_RING_ON == 1)
        {
            if (millis() - BELL_SEQUENCE_DURATION >= BELL_SEQUENCE_STARTED)
            {
                stop_doorbell_sequence();
                return;
            }

            if (PULSE_ACTIVE == 1)
            {
                if (millis() - PULSE_TIME >= BELL_SEQUENCE_RING_ON_TIME)
                    doorbell_turn_off();
            }
        }

        else if (BELL_SEQUENCE_RING_ON == 0)
        {
            if (PULSE_ACTIVE == 1)
            {
                if (millis() - PULSE_WAIT_TIME >= BELL_SEQUENCE_RING_OFF_TIME)
                    doorbell_turn_on();
            }
        }
    }
}

// **********************************
// * Button functions               *
// **********************************

void button_turned_on()
{
    BUTTON_PRESS_ACTIVE = 1;
    BUTTON_PRESS_TIMESTAMP = millis();
    BUTTON_PRESS_COUNT++;

    Serial.println(F("Button pressed"));
    send_button_state_to_broker();

    // * Turn doorbell ring on if no throttling
    if ((RING_WHEN_PRESSED == 1) && (BUTTON_PRESS_COUNT <= BUTTON_THROTTLE_MAX))
        start_doorbell_sequence(DEFAULT_RING_DURATION, 0, 0, 0);
}

void button_turned_off()
{
    BUTTON_PRESS_ACTIVE = 0;
    Serial.println(F("Button released"));
    send_button_state_to_broker();
}

void button_loop()
{
    if (BUTTON_DISABLED == 1)
        return;

    // * Read button input and ring if pressed within limits
    if (debouncer.update())
    {
        if (debouncer.read() != HIGH)
            button_turned_on();
        else
            button_turned_off();
    }

    // * Set button press counter to zero so a new button press can be initiated without being throttled
    if ((BUTTON_PRESS_ACTIVE == 0) && (BUTTON_PRESS_COUNT >= BUTTON_THROTTLE_MAX) && (millis() - BUTTON_THROTTLE_TIME >= BUTTON_PRESS_TIMESTAMP))
    {
        BUTTON_PRESS_TIMESTAMP = millis();
        BUTTON_PRESS_COUNT = 0;
    }
}

// **********************************
// * EEPROM helpers                 *
// **********************************

String read_eeprom(int offset, int len)
{
    String res = "";
    for (int i = 0; i < len; ++i)
    {
        res += char(EEPROM.read(i + offset));
    }
    return res;
}

void write_eeprom(int offset, int len, String value)
{
    for (int i = 0; i < len; ++i)
    {
        if ((unsigned)i < value.length())
            EEPROM.write(i + offset, value[i]);
        else
            EEPROM.write(i + offset, 0);
    }
}

// ******************************************
// * Setup WIFI                             *
// ******************************************


// * Callback notifying us of the need to save config
void save_wifi_config_callback ()
{
    Serial.println(F("Should save config"));
    SAVE_WIFI_CONFIG = true;
}

// * Configure wifimanager
void setup_wifi()
{
    // * Get MQTT Server settings
    String settings_available = read_eeprom(134, 1);

    if (settings_available == "1")
    {
        read_eeprom(0, 64).toCharArray(MQTT_HOST, 64);     // * 0-63
        read_eeprom(64, 6).toCharArray(MQTT_PORT, 6);      // * 64-69
        read_eeprom(70, 32).toCharArray(MQTT_USER, 32);    // * 70-101
        read_eeprom(102, 32).toCharArray(MQTT_PASS, 32);   // * 102-133
    }

    WiFiManagerParameter CUSTOM_MQTT_HOST("host", "MQTT hostname", MQTT_HOST, 64);
    WiFiManagerParameter CUSTOM_MQTT_PORT("port", "MQTT port", MQTT_PORT, 6);
    WiFiManagerParameter CUSTOM_MQTT_USER("user", "MQTT user", MQTT_USER, 32);
    WiFiManagerParameter CUSTOM_MQTT_PASS("pass", "MQTT pass", MQTT_PASS, 32);

    // * WiFiManager local initialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;

    // * Reset settings - uncomment for testing
    // wifiManager.resetSettings();

    // * Set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
    wifiManager.setAPCallback(wifi_ap_mode_callback);

    // * Set timeout
    wifiManager.setConfigPortalTimeout(WIFI_TIMEOUT);

    // * Set save config callback
    wifiManager.setSaveConfigCallback(save_wifi_config_callback);

    // * Add all parameters to the wifi panel
    wifiManager.addParameter(&CUSTOM_MQTT_HOST);
    wifiManager.addParameter(&CUSTOM_MQTT_PORT);
    wifiManager.addParameter(&CUSTOM_MQTT_USER);
    wifiManager.addParameter(&CUSTOM_MQTT_PASS);

    // * Fetches SSID and pass and tries to connect
    // * Reset when no connection after 10 seconds
    if (!wifiManager.autoConnect())
    {
        delay(WIFI_TIMEOUT);

        Serial.println(F("Failed to connect to WIFI and hit timeout"));
        ESP.reset();
    }

    // * Read updated parameters
    strcpy(MQTT_HOST, CUSTOM_MQTT_HOST.getValue());
    strcpy(MQTT_PORT, CUSTOM_MQTT_PORT.getValue());
    strcpy(MQTT_USER, CUSTOM_MQTT_USER.getValue());
    strcpy(MQTT_PASS, CUSTOM_MQTT_PASS.getValue());

    // * Verify updated parameters
    if (strlen(MQTT_HOST) == 0 || strlen(MQTT_PORT) == 0 || strlen(MQTT_USER) == 0 || strlen(MQTT_PASS) == 0)
    {
        Serial.println(F("Config Faulty, Kicking config"));
        wifiManager.resetSettings();

        delay(2000);
        ESP.reset();
    }

    // * Save the custom parameters to FS
    if (SAVE_WIFI_CONFIG)
    {
        Serial.println(F("Saving WiFiManager config"));
        write_eeprom(0, 64, MQTT_HOST);     // * 0-63
        write_eeprom(64, 6, MQTT_PORT);     // * 64-69
        write_eeprom(70, 32, MQTT_USER);    // * 70-101
        write_eeprom(102, 32, MQTT_PASS);   // * 102-133
        write_eeprom(134, 1, "1");          // * 134 --> always "1"
        EEPROM.commit();
    }

    // * If you get here you have connected to the WiFi
    Serial.println(F("Connected to WIFI..."));
}

// **********************************
// * Setup OTA                      *
// **********************************

void setup_ota()
{
    Serial.println(F("Arduino OTA activated."));

    // * Port defaults to 8266
    ArduinoOTA.setPort(8266);

    // * Set hostname for OTA
    ArduinoOTA.setHostname(HOSTNAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);

    ArduinoOTA.onStart([]()
    {
        Serial.println(F("Arduino OTA: Start"));
    });

    ArduinoOTA.onEnd([]()
    {
        Serial.println(F("Arduino OTA: End (Running reboot)"));
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
    {
        Serial.printf("Arduino OTA Progress: %u%%\r", (progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error)
    {
        Serial.printf("Arduino OTA Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
            Serial.println(F("Arduino OTA: Auth Failed"));
        else if (error == OTA_BEGIN_ERROR)
            Serial.println(F("Arduino OTA: Begin Failed"));
        else if (error == OTA_CONNECT_ERROR)
            Serial.println(F("Arduino OTA: Connect Failed"));
        else if (error == OTA_RECEIVE_ERROR)
            Serial.println(F("Arduino OTA: Receive Failed"));
        else if (error == OTA_END_ERROR)
            Serial.println(F("Arduino OTA: End Failed"));
    });

    ArduinoOTA.begin();

    Serial.println(F("Arduino OTA finished"));
}

// **********************************
// * Setup MDNS discovery service   *
// **********************************

void setup_mdns()
{
    Serial.println(F("Starting MDNS responder service"));
    bool mdns_result = MDNS.begin(HOSTNAME);

    if (mdns_result)
        MDNS.addService("http", "tcp", 80);
    else
        Serial.println(F("Failed to setup MDNS"));
}

// **********************************
// * Setup Main                     *
// **********************************

void setup()
{
    // * Update watchdog value
    last_loop = millis();

    // * Initiate watchdog
    tickerOSWatch.attach_ms(((OSWATCH_RESET_TIME / 3) * 1000), osWatch);

    // * Configure Serial and EEPROM
    Serial.begin(BAUD_RATE);
    EEPROM.begin(512);

    // * Initiate led pin
    pinMode(LED_BUILTIN, OUTPUT);

    // * Initiate button pin
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    digitalWrite(BUTTON_PIN, HIGH);

    // * Initiate relay pin
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);

    // * Set button debouncer
    debouncer.attach(BUTTON_PIN);
    debouncer.interval(5);

    // * Start ticker with 0.5 because we start in AP mode and try to connect
    ticker.attach(0.6, led_tick);

    // * Start WIFI, load from EEPROM or open AP mode
    setup_wifi();

    // * Keep LED on
    digitalWrite(LED_BUILTIN, LOW);
    ticker.detach();

    // * Setup Webserver
    setup_webserver();

    // * Configure OTA
    setup_ota();

    // * Startup MDNS Service
    setup_mdns();

    // * Setup MQTT
    mqtt_client.setServer(MQTT_HOST, atoi(MQTT_PORT));
    mqtt_client.setCallback(mqtt_callback);

    Serial.printf("MQTT active: %s:%s\n", MQTT_HOST, MQTT_PORT);
}

// **********************************
// * Loop                           *
// **********************************

void loop()
{
    // * Update last loop
    last_loop = millis();

    // * Handle ota offers
    ArduinoOTA.handle();

    // * Handle webserver requests
    webserver.handleClient();

    // * Maintain MQTT Connection
    mqtt_loop();

    // * Button Sequence
    button_loop();

    // * Bell sequence
    doorbell_ring_loop();
}
