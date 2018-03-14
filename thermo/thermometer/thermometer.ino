#include "DHTesp.h"
DHTesp dht;

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println("Status\tHumidity (%)\tTemperature (C)");
  delay(2000);
  Serial.println("-----");
  dht.setup(2); // Connect DHT sensor to GPIO 2
}

void loop()
{
  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();

  Serial.print(dht.getStatusString());
  Serial.print("\t");
  Serial.print(humidity, 1);
  Serial.print("\t\t");
  Serial.print(temperature, 1);
  Serial.println("\t\t");

  delay(5000);
}
