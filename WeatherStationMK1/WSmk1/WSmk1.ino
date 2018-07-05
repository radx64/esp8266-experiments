#include <CayenneMQTTESP8266.h>
#include <DHT.h>
#include <Ticker.h>

#define CAYENNE_PRINT Serial
#define DHT11_PIN D1

ADC_MODE(ADC_VCC)
DHT dht(DHT11_PIN, DHT11);
Ticker timer;

char ssid[] = "AP_PASSWORD";
char wifiPassword[] = "AP_PASSWORD";

char username[] = "CAYENNYE_USERNAME";
char password[] = "CAYENNYE_PASSWORD";
char clientID[] = "CAYENNYE_CLIENT_ID";

void wakeUpIn(int sleepTime)
{
  ESP.deepSleep(sleepTime);
}

void connectionTimeout()
{
  wakeUpIn(30e6); // 30e6 is 30 seconds
}

void setup()
{
  Serial.begin(115200);
  Serial.print("Initializing Cayenne...");
  timer.attach(30, connectionTimeout);
  Cayenne.begin(username, password, clientID, ssid, wifiPassword);
  Serial.println("OK");
  dht.begin();
}

void loop()
{
  Cayenne.loop();
}

CAYENNE_OUT_DEFAULT()
{
  float vcc = ESP.getVcc()/1000.0f;
  float temperature = dht.readTemperature();
  float rh = dht.readHumidity();
  Cayenne.celsiusWrite(1, temperature);
  Cayenne.virtualWrite(2, rh, "rel_hum", "p");
  Cayenne.virtualWrite(3, vcc ,"batt", "v");
  delay(5000);
  wakeUpIn(15*60e6); // fifteen minutes
}
