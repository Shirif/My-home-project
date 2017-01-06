#include <SPI.h>

#define MY_DEBUG
#define MY_RADIO_NRF24

#define MY_GATEWAY_MQTT_CLIENT

// Set this nodes subscripe and publish topic prefix
#define MY_MQTT_PUBLISH_TOPIC_PREFIX "mygateway1-out"
#define MY_MQTT_SUBSCRIBE_TOPIC_PREFIX "mygateway1-in"

// Set MQTT client id
#define MY_MQTT_CLIENT_ID "mysensors-1"

// W5100 Ethernet module SPI enable (optional if using a shield/module that manages SPI_EN signal)
//#define MY_W5100_SPI_EN 4

// Enable Soft SPI for NRF radio (note different radio wiring is required)
// The W5100 ethernet module seems to have a hard time co-operate with
// radio on the same spi bus.
#if !defined(MY_W5100_SPI_EN) && !defined(ARDUINO_ARCH_SAMD)
#define MY_SOFTSPI
#define MY_SOFT_SPI_SCK_PIN 54
#define MY_SOFT_SPI_MISO_PIN 56
#define MY_SOFT_SPI_MOSI_PIN 55
#endif

// When W5100 is connected we have to move CE/CSN pins for NRF radio
#ifndef MY_RF24_CE_PIN
#define MY_RF24_CE_PIN 5
#endif
#ifndef MY_RF24_CS_PIN
#define MY_RF24_CS_PIN 6
#endif

// Enable these if your MQTT broker requires usenrame/password
//#define MY_MQTT_USER "username"
//#define MY_MQTT_PASSWORD "password"

// Enable MY_IP_ADDRESS here if you want a static ip address (no DHCP)
//#define MY_IP_ADDRESS 192,168,0,178

// If using static ip you need to define Gateway and Subnet address as well
//#define MY_IP_GATEWAY_ADDRESS 192,168,0,1
//#define MY_IP_SUBNET_ADDRESS 255,255,255,0

// MQTT broker ip address or url. Define one or the other.
//#define MY_CONTROLLER_URL_ADDRESS "m20.cloudmqtt.com"
#define MY_CONTROLLER_IP_ADDRESS 192,168,0,108

// The MQTT broker port to to open
#define MY_PORT 1883

#include <Ethernet.h>
#include <MySensors.h>
#include <DHT.h>

// Set this to the pin you connected the DHT's data pin to
#define DHT_DATA_PIN1 30
#define DHT_DATA_PIN2 32
#define DHT_DATA_PIN3 34
#define DHT_DATA_PIN4 36

// Sleep time between reads (in milliseconds)
unsigned long SLEEP_TIME = 120000;

unsigned long T = 1;

#define CHILD_ID_HUM1 0
#define CHILD_ID_HUM2 1
#define CHILD_ID_HUM3 2
#define CHILD_ID_HUM4 3

#define CHILD_ID_TEMP1 4
#define CHILD_ID_TEMP2 5
#define CHILD_ID_TEMP3 6
#define CHILD_ID_TEMP4 7

#define CHILD_ID_MOT1 8
#define CHILD_ID_MOT2 9
#define CHILD_ID_MOT3 10
#define CHILD_ID_MOT4 11

#define CHILD_ID_LIGHT1 12
#define CHILD_ID_LIGHT2 13
#define CHILD_ID_LIGHT3 14
#define CHILD_ID_LIGHT4 15

#define DIGITAL_INPUT_SENSOR1 21   // The digital input you attached your motion sensor
#define DIGITAL_INPUT_SENSOR2 20   // The digital input you attached your motion sensor
#define DIGITAL_INPUT_SENSOR3 19   // The digital input you attached your motion sensor
#define DIGITAL_INPUT_SENSOR4 18   // The digital input you attached your motion sensor

#define LIGHT_SENSOR_ANALOG_PIN1 62
#define LIGHT_SENSOR_ANALOG_PIN2 63
#define LIGHT_SENSOR_ANALOG_PIN3 64
#define LIGHT_SENSOR_ANALOG_PIN4 65

float lastTemp1;
float lastTemp2;
float lastTemp3;
float lastTemp4;

float lastHum1;
float lastHum2;
float lastHum3;
float lastHum4;

int lastLightLevel1;
int lastLightLevel2;
int lastLightLevel3;
int lastLightLevel4;

bool lastTripped1;
bool lastTripped2;
bool lastTripped3;
bool lastTripped4;

bool metric = true;

MyMessage msgHum1(CHILD_ID_HUM1, V_HUM);
MyMessage msgHum2(CHILD_ID_HUM2, V_HUM);
MyMessage msgHum3(CHILD_ID_HUM3, V_HUM);
MyMessage msgHum4(CHILD_ID_HUM4, V_HUM);

MyMessage msgTemp1(CHILD_ID_TEMP1, V_TEMP);
MyMessage msgTemp2(CHILD_ID_TEMP2, V_TEMP);
MyMessage msgTemp3(CHILD_ID_TEMP3, V_TEMP);
MyMessage msgTemp4(CHILD_ID_TEMP4, V_TEMP);

MyMessage msgMot1(CHILD_ID_MOT1, V_TRIPPED);
MyMessage msgMot2(CHILD_ID_MOT2, V_TRIPPED);
MyMessage msgMot3(CHILD_ID_MOT3, V_TRIPPED);
MyMessage msgMot4(CHILD_ID_MOT4, V_TRIPPED);

MyMessage msgLight1(CHILD_ID_LIGHT1, V_LIGHT_LEVEL);
MyMessage msgLight2(CHILD_ID_LIGHT2, V_LIGHT_LEVEL);
MyMessage msgLight3(CHILD_ID_LIGHT3, V_LIGHT_LEVEL);
MyMessage msgLight4(CHILD_ID_LIGHT4, V_LIGHT_LEVEL);

DHT dht1;
DHT dht2;
DHT dht3;
DHT dht4;

void presentation() {

  // Send the Sketch Version Information to the Gateway
  sendSketchInfo("12-1 Sensor", "1.0");

  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_HUM1, S_HUM);
  present(CHILD_ID_HUM2, S_HUM);
  present(CHILD_ID_HUM3, S_HUM);
  present(CHILD_ID_HUM4, S_HUM);
  present(CHILD_ID_TEMP1, S_TEMP);
  present(CHILD_ID_TEMP2, S_TEMP);
  present(CHILD_ID_TEMP3, S_TEMP);
  present(CHILD_ID_TEMP4, S_TEMP);
  present(CHILD_ID_MOT1, S_MOTION);
  present(CHILD_ID_MOT2, S_MOTION);
  present(CHILD_ID_MOT3, S_MOTION);
  present(CHILD_ID_MOT4, S_MOTION);
  present(CHILD_ID_LIGHT1, S_LIGHT_LEVEL);
  present(CHILD_ID_LIGHT2, S_LIGHT_LEVEL);
  present(CHILD_ID_LIGHT3, S_LIGHT_LEVEL);
  present(CHILD_ID_LIGHT4, S_LIGHT_LEVEL);

  metric = getConfig().isMetric;
}

void setup()
{
  dht1.setup(DHT_DATA_PIN1);
  dht2.setup(DHT_DATA_PIN2);
  dht3.setup(DHT_DATA_PIN3);
  dht4.setup(DHT_DATA_PIN4);

  sleep(dht1.getMinimumSamplingPeriod());

  pinMode(DIGITAL_INPUT_SENSOR1, INPUT);
  pinMode(DIGITAL_INPUT_SENSOR2, INPUT);
  pinMode(DIGITAL_INPUT_SENSOR3, INPUT);
  pinMode(DIGITAL_INPUT_SENSOR4, INPUT);

  //attachInterrupt(digitalPinToInterrupt(DIGITAL_INPUT_SENSOR1), loop, CHANGE);

}



void loop()
{

  // Read digital motion value

  boolean tripped1 = digitalRead(DIGITAL_INPUT_SENSOR1) == HIGH;
  if (tripped1 != lastTripped1) {
    Serial.print("tripped 1: ");
    Serial.println(tripped1);
    send(msgMot1.set(tripped1 ? "1" : "0")); // Send tripped value to gw
    lastTripped1 = tripped1;
  }

  boolean tripped2 = digitalRead(DIGITAL_INPUT_SENSOR2) == HIGH;
  if (tripped2 != lastTripped2) {
    Serial.print("tripped 2: ");
    Serial.println(tripped2);
    send(msgMot2.set(tripped2 ? "1" : "0")); // Send tripped value to gw
    lastTripped2 = tripped2;
  }

  boolean tripped3 = digitalRead(DIGITAL_INPUT_SENSOR3) == HIGH;
  if (tripped3 != lastTripped3) {
    Serial.print("tripped 3: ");
    Serial.println(tripped3);
    send(msgMot3.set(tripped3 ? "1" : "0")); // Send tripped value to gw
    lastTripped3 = tripped3;
  }

  boolean tripped4 = digitalRead(DIGITAL_INPUT_SENSOR4) == HIGH;
  if (tripped4 != lastTripped4) {
    Serial.print("tripped 4: ");
    Serial.println(tripped4);
    send(msgMot4.set(tripped4 ? "1" : "0")); // Send tripped value to gw
    lastTripped4 = tripped4;
  }


  T++;
  if (T > SLEEP_TIME) {
    T = 1;

    // Read analog light value

    int lightLevel1 = (analogRead(LIGHT_SENSOR_ANALOG_PIN1)) / 10.23;
    if (lightLevel1 != lastLightLevel1) {
      send(msgLight1.set(lightLevel1));
      lastLightLevel1 = lightLevel1;
      Serial.print("lightLevel 1: ");
      Serial.println(lightLevel1);
    }

    int lightLevel2 = (analogRead(LIGHT_SENSOR_ANALOG_PIN2)) / 10.23;
    if (lightLevel2 != lastLightLevel2) {
      send(msgLight2.set(lightLevel2));
      lastLightLevel2 = lightLevel2;
      Serial.print("lightLevel 2: ");
      Serial.println(lightLevel2);
    }

    int lightLevel3 = (analogRead(LIGHT_SENSOR_ANALOG_PIN3)) / 10.23;
    if (lightLevel3 != lastLightLevel3) {
      send(msgLight3.set(lightLevel3));
      lastLightLevel3 = lightLevel3;
      Serial.print("lightLevel 3: ");
      Serial.println(lightLevel3);
    }

    int lightLevel4 = (analogRead(LIGHT_SENSOR_ANALOG_PIN4)) / 10.23;
    if (lightLevel4 != lastLightLevel4) {
      send(msgLight4.set(lightLevel4));
      lastLightLevel4 = lightLevel4;
      Serial.print("lightLevel 4: ");
      Serial.println(lightLevel4);
    }

     float temperature1 = dht1.getTemperature();
    if (isnan(temperature1)) {
      Serial.println("Failed reading temperature from DHT1!");
    } else if (temperature1 != lastTemp1) {
      lastTemp1 = temperature1;
      if (!metric) {
        temperature1 = dht1.toFahrenheit(temperature1);
      }
      send(msgTemp1.set(temperature1, 1));
      Serial.print("T1: ");
      Serial.println(temperature1);
    }

    float humidity1 = dht1.getHumidity();
    if (isnan(humidity1)) {
      Serial.println("Failed reading humidity from DHT1");
    } else if (humidity1 != lastHum1) {
      lastHum1 = humidity1;
      send(msgHum1.set(humidity1, 1));
      Serial.print("H1: ");
      Serial.println(humidity1);
    }



    float temperature2 = dht2.getTemperature();
    if (isnan(temperature2)) {
      Serial.println("Failed reading temperature from DHT2!");
    } else if (temperature2 != lastTemp2) {
      lastTemp2 = temperature2;
      if (!metric) {
        temperature2 = dht2.toFahrenheit(temperature2);
      }
      send(msgTemp2.set(temperature2, 1));
      Serial.print("T2: ");
      Serial.println(temperature2);
    }

    float humidity2 = dht2.getHumidity();
    if (isnan(humidity2)) {
      Serial.println("Failed reading humidity from DHT2");
    } else if (humidity2 != lastHum2) {
      lastHum2 = humidity2;
      send(msgHum2.set(humidity2, 1));
      Serial.print("H2: ");
      Serial.println(humidity2);
    }


    float temperature3 = dht3.getTemperature();
    if (isnan(temperature3)) {
      Serial.println("Failed reading temperature from DHT3!");
    } else if (temperature3 != lastTemp3) {
      lastTemp3 = temperature3;
      if (!metric) {
        temperature3 = dht3.toFahrenheit(temperature3);
      }
      send(msgTemp3.set(temperature3, 1));
      Serial.print("T3: ");
      Serial.println(temperature3);
    }

    float humidity3 = dht3.getHumidity();
    if (isnan(humidity3)) {
      Serial.println("Failed reading humidity from DHT3");
    } else if (humidity3 != lastHum3) {
      lastHum3 = humidity3;
      send(msgHum3.set(humidity3, 1));
      Serial.print("H3: ");
      Serial.println(humidity3);
    }



    float temperature4 = dht4.getTemperature();
    if (isnan(temperature4)) {
      Serial.println("Failed reading temperature from DHT4!");
    } else if (temperature4 != lastTemp4) {
      lastTemp4 = temperature4;
      if (!metric) {
        temperature4 = dht4.toFahrenheit(temperature4);
      }
      send(msgTemp4.set(temperature4, 1));
      Serial.print("T4: ");
      Serial.println(temperature4);
    }

    float humidity4 = dht4.getHumidity();
    if (isnan(humidity4)) {
      Serial.println("Failed reading humidity from DHT4");
    } else if (humidity4 != lastHum4) {
      lastHum4 = humidity4;
      send(msgHum4.set(humidity4, 1));
      Serial.print("H4: ");
      Serial.println(humidity4);
    }

  }

  //sleep(UPDATE_INTERVAL);
  //sleep(digitalPinToInterrupt(DIGITAL_INPUT_SENSOR1), CHANGE, UPDATE_INTERVAL);
}

