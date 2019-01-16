#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include "secrets.h"

const char *wifi_ssid = WIFI_SSID;
const char *wifi_password = WIFI_PASSWORD;
const char *mqtt_server = MQTT_SERVER;

const char *mqtt_client = "Clothesline";
const char *temperature_topic = "clothesline/temperature";
const char *humidity_topic = "clothesline/humidity";
const char *heat_index_topic = "clothesline/heat_index";

struct dht_reading
{
  float temperature;
  float humidity;
  float heat_index;
};

#define DHTPIN D7
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

WiFiClient espClient;
PubSubClient client(espClient);

void setup()
{
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  delay(1);
  dht.begin();
}

void start_wifi()
{
  WiFi.forceSleepWake();
  delay(1);
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);
  Serial.print("WiFi connecting: ");
  Serial.println(wifi_ssid);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void stop_wifi()
{
  WiFi.disconnect();
}

void start_pub()
{
  client.setServer(mqtt_server, 1883);
  while (!client.connected())
  {
    Serial.print("MQTT connecting: ");
    Serial.println(mqtt_server);
    if (client.connect(mqtt_client))
    {
      Serial.println("MQTT connected");
    }
    else
    {
      Serial.print("MQTT failed, reason: ");
      Serial.println(client.state());
      Serial.println("MQTT retrying in 5 seconds");
      delay(5000);
    }
  }
  client.loop();
}

void stop_pub()
{
  client.disconnect();
}

dht_reading read()
{
  Serial.print("DHT22 connecting: ");
  Serial.println(DHTPIN);
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  return dht_reading{
      temperature,
      humidity,
      dht.computeHeatIndex(temperature, humidity, false)};
}

void publish(dht_reading reading)
{
  if (!isnan(reading.temperature))
  {
    Serial.print("Temperature: ");
    Serial.println(String(reading.temperature).c_str());
    client.publish(temperature_topic, String(reading.temperature).c_str(), true);
    delay(50);
  }
  if (!isnan(reading.humidity))
  {
    Serial.print("Humidity: ");
    Serial.println(String(reading.humidity).c_str());
    client.publish(humidity_topic, String(reading.humidity).c_str(), true);
    delay(50);
  }
  if (!isnan(reading.heat_index))
  {
    Serial.print("Heat Index: ");
    Serial.println(String(reading.heat_index).c_str());
    client.publish(heat_index_topic, String(reading.heat_index).c_str(), true);
    delay(50);
  }
}

void loop()
{
  Serial.begin(74880);
  dht_reading reading = read();
  start_wifi();
  start_pub();
  publish(reading);
  stop_pub();
  stop_wifi();

  Serial.println("Deep sleeping for 60 seconds");
  Serial.flush();
  Serial.end();
  ESP.deepSleep(1000 * 1000 * 60, WAKE_RF_DISABLED);
}
