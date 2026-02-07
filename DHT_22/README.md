# DHT22 Temperature, Humidity & Heat Index (Arduino)

This project reads **temperature (°C)**, **humidity (%)**, and **heat index (°C)** from a **DHT22 (AM2302)** sensor using an Arduino and displays the values on the Serial Monitor.

## Wiring

| DHT22 Pin | Arduino Pin |
|----------|-------------|
| VCC      | 5V (or 3.3V) |
| GND      | GND |
| DATA     | D2 |

> The DATA pin is defined as digital pin 2 in the code. Modify it if needed.

## Procedure

1. Connect the DHT22 sensor to the Arduino according to the wiring table.
2. Open **Arduino IDE → Tools → Manage Libraries**.
3. Search and install **DHT sensor library** (by **Adafruit**).
4. Also install **Adafruit Unified Sensor** (required dependency).
5. Create a new Arduino sketch and paste the code below.
6. Select the correct **Board** and **Port** from the Tools menu.
7. Upload the code to the Arduino.
8. Open **Serial Monitor** and set the baud rate to **9600**.
9. Values will be displayed every **2 seconds**.


## Code

```cpp
#include "DHT.h" 

#define DHTPIN 2       // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22  // DHT 22 (AM2302)

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);
  dht.begin();
}

void loop() {
  // Wait a few seconds between measurements.
  delay(2000);

  // Read temperature as Celsius
  float t = dht.readTemperature();

  // Read humidity as % 
  float h = dht.readHumidity();

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Compute heat index in Celsius
  float hic = dht.computeHeatIndex(t, h, false);

  Serial.print(F("Temperature: "));
  Serial.print(t);
  Serial.print(F("°C Heat index: "));
  Serial.print(hic);
  Serial.print(F("°C Humidity: "));
  Serial.print(h);
  Serial.println(F("%"));
}
```

## Result Display Example

```txt
Temperature: 27.10°C Heat index: 28.30°C Humidity: 62.00%
Temperature: 27.20°C Heat index: 28.40°C Humidity: 61.80%