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

/*
  // Flash leds on rx/tx/err
  #define MY_LEDS_BLINKING_FEATURE
  // Set blinking period
  #define MY_DEFAULT_LED_BLINK_PERIOD 300

  // Enable inclusion mode
  #define MY_INCLUSION_MODE_FEATURE
  // Enable Inclusion mode button on gateway
  #define MY_INCLUSION_BUTTON_FEATURE
  // Set inclusion mode duration (in seconds)
  #define MY_INCLUSION_MODE_DURATION 60
  // Digital pin used for inclusion mode button
  #define MY_INCLUSION_MODE_BUTTON_PIN  3

  // Uncomment to override default HW configurations
  //#define MY_DEFAULT_ERR_LED_PIN 16  // Error led pin
  //#define MY_DEFAULT_RX_LED_PIN  16  // Receive led pin
  //#define MY_DEFAULT_TX_LED_PIN  16  // the PCB, on board LED
*/

#include <Ethernet.h>
#include <MySensors.h>
#include <DHT.h>

//**********************************
//#include <NewRemoteTransmitter.h>
//#include <RemoteReceiver.h>
//#include <NewRemoteReceiver.h>
//#include <SensorReceiver.h>
#include <InterruptChain.h>
//**********************************



// Set this to the pin you connected the DHT's data pin to
#define DHT_DATA_PIN 30

// Set this offset if the sensor has a permanent small offset to the real temperatures
#define SENSOR_TEMP_OFFSET 0

// Sleep time between sensor updates (in milliseconds)
// Must be >1000ms for DHT22 and >2000ms for DHT11
static const uint64_t UPDATE_INTERVAL = 6000;

// Force sending an update of the temperature after n sensor reads, so a controller showing the
// timestamp of the last update doesn't show something like 3 hours in the unlikely case, that
// the value didn't change since;
// i.e. the sensor would force sending an update every UPDATE_INTERVAL*FORCE_UPDATE_N_READS [ms]
static const uint8_t FORCE_UPDATE_N_READS = 10;


// Sleep time between reads (in milliseconds)
unsigned long SLEEP_TIME = 60000; 

unsigned long T = 1;



#define CHILD_ID_HUM 0
#define CHILD_ID_TEMP 1

#define CHILD_ID_MOT 2

#define CHILD_ID_LIGHT 3

#define DIGITAL_INPUT_SENSOR 21   // The digital input you attached your motion sensor.  (Only 2 and 3 generates interrupt!)

#define LIGHT_SENSOR_ANALOG_PIN 62

float lastTemp;
float lastHum;
uint8_t nNoUpdatesTemp;
uint8_t nNoUpdatesHum;
bool metric = true;

int lastLightLevel;
bool lastTripped;

MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
MyMessage msgMot(CHILD_ID_MOT, V_TRIPPED);
MyMessage msgLight(CHILD_ID_LIGHT, V_LIGHT_LEVEL);
DHT dht;

void presentation() {

  // Send the Sketch Version Information to the Gateway
  sendSketchInfo("3-1 Sensor", "1.0");

  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_HUM, S_HUM);
  present(CHILD_ID_TEMP, S_TEMP);
  present(CHILD_ID_MOT, S_MOTION);
  present(CHILD_ID_LIGHT, S_LIGHT_LEVEL);

  metric = getConfig().isMetric;
}

void setup()
{
  dht.setup(DHT_DATA_PIN); // set data pin of DHT sensor
  if (UPDATE_INTERVAL <= dht.getMinimumSamplingPeriod()) {
    Serial.println("Warning: UPDATE_INTERVAL is smaller than supported by the sensor!");
  }
  // Sleep for the time of the minimum sampling period to give the sensor time to power up
  // (otherwise, timeout errors might occure for the first reading)
  sleep(dht.getMinimumSamplingPeriod());

  pinMode(DIGITAL_INPUT_SENSOR, INPUT);      // sets the motion sensor digital pin as input
  //attachInterrupt(digitalPinToInterrupt(DIGITAL_INPUT_SENSOR), loop, CHANGE);
   
}



void loop()
{

// Read digital motion value
  boolean tripped = digitalRead(DIGITAL_INPUT_SENSOR) == HIGH; 
  
  if (tripped != lastTripped) {     
  Serial.print("tripped: ");
  Serial.println(tripped);
  send(msgMot.set(tripped?"1":"0"));  // Send tripped value to gw 
  lastTripped = tripped;
  }
 

  
T++;
  if (T > SLEEP_TIME) {
    T = 1;
 
 // Read analog light value
  int lightLevel = (1023-analogRead(LIGHT_SENSOR_ANALOG_PIN))/10.23; 
  
  if (lightLevel != lastLightLevel) {
      send(msgLight.set(lightLevel));
      lastLightLevel = lightLevel;
      Serial.print("lightLevel: ");
      Serial.println(lightLevel);
  }
      
 // Force reading sensor, so it works also after sleep()
  dht.readSensor(true);
  
 // Get temperature from DHT library
  float temperature = dht.getTemperature();
  if (isnan(temperature)) {
    Serial.println("Failed reading temperature from DHT!");
  } else if (temperature != lastTemp || nNoUpdatesTemp == FORCE_UPDATE_N_READS) {
    // Only send temperature if it changed since the last measurement or if we didn't send an update for n times
    lastTemp = temperature;
    if (!metric) {
      temperature = dht.toFahrenheit(temperature);
    }
    // Reset no updates counter
    nNoUpdatesTemp = 0;
    temperature += SENSOR_TEMP_OFFSET;
    send(msgTemp.set(temperature, 1));

    #ifdef MY_DEBUG
    Serial.print("T: ");
    Serial.println(temperature);
    #endif
  } else {
    // Increase no update counter if the temperature stayed the same
    nNoUpdatesTemp++;
  }

  // Get humidity from DHT library
  float humidity = dht.getHumidity();
  if (isnan(humidity)) {
    Serial.println("Failed reading humidity from DHT");
  } else if (humidity != lastHum || nNoUpdatesHum == FORCE_UPDATE_N_READS) {
    // Only send humidity if it changed since the last measurement or if we didn't send an update for n times
    lastHum = humidity;
    // Reset no updates counter
    nNoUpdatesHum = 0;
    send(msgHum.set(humidity, 1));
    
    #ifdef MY_DEBUG
    Serial.print("H: ");
    Serial.println(humidity);
    #endif
  } else {
    // Increase no update counter if the humidity stayed the same
    nNoUpdatesHum++;
  }
  }
    // Sleep for a while
  //sleep(UPDATE_INTERVAL);
  //sleep(digitalPinToInterrupt(DIGITAL_INPUT_SENSOR), CHANGE, UPDATE_INTERVAL);
}

