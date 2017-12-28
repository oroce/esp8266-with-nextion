#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <Nextion.h>
#include <FS.h>
#include <types.h>
#include <ago.h>
String mqttClientId = String("ESP8266Client-") + ESP.getChipId();
char *mqttServer = ""; // new char;
uint16_t mqttPort = 0;
char *mqttUser = "";
char *mqttPassword = "";
char *statusTopic = new char;
char *listenToTopic = "nextion/reply-state";
StaticJsonBuffer<400> JSONBuffer;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
#define dbSerial Serial;
#define nexSerial Serial2;
SoftwareSerial nextion(13, 14);// Nextion TX to pin 2 and RX to pin 3 of Arduino

Nextion myNextion(nextion, 9600); //create a Nextion object named myNextion using the nextion serial port @ 9600bps

State state = {
    false, // wifi
    false, // mqtt
    0.00,  // broom temp
    0.00,  // broom humidity
    0.00,  // lroom temp
    0.00,  // lroom humidty
    0.00,  // loggia temp
    0.00,  // loggia humidity
    "n/a", // frontdoor
    NULL   // updatedAt
};
void setup()
{
    Serial.begin(9600);
    mqttClient.setCallback(callback);
    myNextion.init();
    if (SPIFFS.begin()) {
        Serial.println("mounted file system");
        if (SPIFFS.exists("/config.json")) {
            //file exists, reading and loading
            Serial.println("reading config file");
            File configFile = SPIFFS.open("/config.json", "r");
            if (configFile) {
                Serial.println("opened config file");
                size_t size = configFile.size();
                // Allocate a buffer to store contents of the file.
                std::unique_ptr<char[]> buf(new char[size]);

                configFile.readBytes(buf.get(), size);
                DynamicJsonBuffer jsonBuffer;
                JsonObject& json = jsonBuffer.parseObject(buf.get());
                json.printTo(Serial);
                if (json.success()) {
                    Serial.println("\nparsed json");
                    mqttServer = strdup(json["mqtt_server"]);
             
                    mqttUser = strdup(json["mqtt_user"]);
                    mqttPassword = strdup(json["mqtt_password"]);
                    mqttPort = (uint16_t)(json["mqtt_port"]);
                    Serial.print("Server is " + String(mqttServer));
                    Serial.println("User is " + String(mqttUser));
                    Serial.println("PWD is " + String(mqttPassword));

                } else {
                    Serial.println("failed to load json config");
                }
            }
        }
    } else {
        Serial.println("failed to mount FS");
    }
    render();
}

void loop()
{
    connectWiFi();
    render();
    if (!mqttClient.connected())
    {
        reconnect();
        render();
    }
    mqttClient.loop();
    render();
    delay(1000);
}
void reconnect()
{
    mqttClient.setServer(mqttServer, mqttPort);
    // Loop until we're reconnected
    while (!mqttClient.connected())
    {
        Serial.print("Attempting MQTT connection to ");
        Serial.print(mqttServer);

        // Attempt to connect
        boolean connected = mqttClient.connect(mqttClientId.c_str(),
                                               mqttUser,
                                               mqttPassword,
                                               statusTopic, // will topic
                                               1,           // qos
                                               true,        // retain
                                               "offline");  // will message
        if (connected)
        {
            Serial.println(" connected");
            // Once connected, publish an announcement...
            //mqttClient.publish(statusTopic, "online");
            // ... and resubscribe
            state.isMQTTConnected = true;
            mqttClient.publish("nextion/request-state", "on");
            boolean subscribed = mqttClient.subscribe(listenToTopic);
            if (subscribed) {
                Serial.println("subscribed");
            } else {
                Serial.println("not subscribed");
            }
        }
        else
        {
            state.isMQTTConnected = false;
            Serial.print(" failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void connectWiFi()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        state.isWifiConnected = true;
        return;
    }
    state.isWifiConnected = false;
    WiFi.mode(WIFI_STA);

    // Start SmartConfig if necessary
    if (WiFi.SSID() == "")
    {
        Serial.println("Beginning SmartConfig");
        WiFi.beginSmartConfig();

        while (WiFi.status() != WL_CONNECTED)
        {
            Serial.print(".");
            if (WiFi.smartConfigDone())
            {
                Serial.println();
                Serial.println("SmartConfig completed");
                WiFi.stopSmartConfig();
                delay(2000);
                break;
            }
            delay(2000);
        }
    }
    else
    {
        Serial.println(String("Using a saved SSID: ") + WiFi.SSID());
    }

    Serial.print("Connecting..");

    WiFi.begin();
    unsigned int cnt = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(10000);
        Serial.print(".");
        if (cnt++ == 30)
        {
            // wipe wifi info after 30 attempts
            // wifi ap may not be available anymore
            Serial.println("Can't connect, resetting WiFi info");
            WiFi.disconnect();
            //ESP.restart();
            delay(3000);
        }
    }
    state.isWifiConnected = true;
    Serial.println("Ok");

    Serial.println(String("My IP: ") + WiFi.localIP().toString());

    delay(3000);
}

void callback(char *topic, byte *payload, unsigned int length)
{
    Serial.println("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    String text = "";
    for (int i = 0; i < length; i++)
    {
        //Serial.print((char) payload[i]);
        text += (char)payload[i];
    }
    Serial.println(text);
    if (strcmp(topic, "nextion/reply-state") != 0) {
        Serial.println("Not nextion reply state, noop");
        return;
    }
    JsonObject& parsed= JSONBuffer.parseObject(text);
    if (!parsed.success()) {
        Serial.println("Parsing failed" + text);
        JSONBuffer.clear();
        return;
    }
    state.bedroomTemperature = (float)parsed["bedroom-temperature"];
    state.bedroomHumidity = (float)parsed["bedroom-humidity"];
    state.livingroomTemperature = (float)parsed["living-room-temperature"];
    state.livingroomHumidity = (float)parsed["living-room-humidity"];
    state.loggiaTemperature = (float)parsed["loggia-temperature"];
    state.loggiaHumidity = (float)parsed["loggia-humidity"];
    state.frontdoorStatus = parsed["frontdoor"];
    state.updatedAt = millis();
    JSONBuffer.clear();
    /*Serial.println("bedroom temp:" + String(bedroomTemp, 1));
    Serial.println("bedroom humidity:" + String(bedroomHumidity, 1));
    Serial.println("livingroom temp:" + String(livingRoomTemp, 1));
    Serial.println("livingroom humidity:" + String(livingRoomHumidity, 1));
    Serial.println("loggia temp:" + String(loggiaTemp, 1));
    Serial.println("loggia humidity:" + String(loggiaHumidity, 1));*/
}

void render() {
    String updatedAt = "waiting for sync";
    if (state.updatedAt != NULL) {
        updatedAt = ago(millis() - state.updatedAt);
    }
    myNextion.setComponentText("t0", String(state.bedroomTemperature, 1));
    myNextion.setComponentText("t3", String(state.bedroomHumidity, 1));
    myNextion.setComponentText("t1", String(state.livingroomTemperature, 1));
    myNextion.setComponentText("t4", String(state.livingroomHumidity, 1));
    myNextion.setComponentText("t2", String(state.loggiaTemperature));
    myNextion.setComponentText("t5", String(state.loggiaHumidity, 1));
    myNextion.setComponentText("t6", state.frontdoorStatus);
    myNextion.setComponentText("t7", updatedAt);
    if (state.isWifiConnected) {
        myNextion.sendCommand("p1.pic=2");
    } else {
        myNextion.sendCommand("p1.pic=1");
    }

    if (state.isMQTTConnected) {
        myNextion.sendCommand("p2.pic=4");
    } else {
        myNextion.sendCommand("p2.pic=3");
    }
}