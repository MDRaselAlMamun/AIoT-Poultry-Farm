# AIoT Poultry Farm System Development

## Description

The AIoT Poultry Farm system is designed to monitor various environmental conditions within a poultry farm using multiple sensors. The system tracks parameters like temperature, humidity, air quality (CO<sub>2</sub>, NH<sub>3</sub>), oxygen levels, dust concentration, and light intensity using a combination of digital and analog sensors. The system continuously logs and displays sensor data, helping farmers maintain optimal conditions for poultry health and productivity.

This system uses the following sensors:
1. **DHT22** for temperature and humidity measurement.
2. **Gravity: NH3 Sensor (Calibrated)** for monitoring NH3 (Ammonia) concentration.
3. **Gravity: Electrochemical Oxygen / O<sub>2</sub> Sensor** for measuring oxygen levels.
4. **Gravity: Analog Electrochemical Carbon Dioxide / CO<sub>2</sub> Sensor** for detecting carbon dioxide concentration.
5. **Sharp GP2Y1010AU0F Optical Dust Sensor - PM2.5/PM10 Detection** for particulate matter (dust) concentration.
6. **Optical Sensitive Resistance Light Detection Photosensitive LDR Sensor Module** for measuring light intensity in the farm.

This project is ideal for real-time monitoring and managing the conditions of a poultry farm, improving overall farm management and poultry health.

## Wiring for 6 Sensors

Below is the wiring guide for the 6 sensors used in this project:

## Wiring (Arduino UNO R4)

| **Sensor Pin**         | **Arduino UNO R4 Pin**        |
|------------------------|-------------------------------|
| VCC                    | 5V                            |
| GND                    | GND                           |

### 1. **DHT22 (Temperature and Humidity Sensor)**
| **Sensor Pin**         | **Arduino UNO R4 Pin**        |
|------------------------|-------------------------------|
| Data                   | Digital Pin 2                |

### 2. **Gravity: NH3 Sensor (Calibrated)**
| **Sensor Pin**         | **Arduino UNO R4 Pin**        |
|------------------------|-------------------------------|
| SDA                    | SDA (D18)                     |
| SCL                    | SCL (D19)                     |

### 3. **Gravity: Electrochemical Oxygen / O₂ Sensor**
| **Sensor Pin**         | **Arduino UNO R4 Pin**        |
|------------------------|-------------------------------|
| SDA                    | SDA (D18)                     |
| SCL                    | SCL (D19)                     |

### 4. **Gravity: Analog Electrochemical CO₂ Sensor**
| **Sensor Pin**         | **Arduino UNO R4 Pin**        |
|------------------------|-------------------------------|
| Vout                   | Analog Pin A1                 |

### 5. **Sharp GP2Y1010AU0F Optical Dust Sensor**
| **Sensor Pin**         | **Arduino UNO R4 Pin**        |
|------------------------|-------------------------------|
| Vout                   | Analog Pin A0                 |
| LED Pin                | Digital Pin 3                 |

### 6. **Optical Sensitive Resistance Light Sensor (LDR)**
| **Sensor Pin**         | **Arduino UNO R4 Pin**        |
|------------------------|-------------------------------|
| Analog Output (A)      | Analog Pin A2                 |


## Procedure (Step by Step)

1. **Assemble the Hardware**:
   - Connect each sensor to the Arduino according to the wiring diagram mentioned above.
   - Ensure that all the connections are secure and there is no short circuit.

2. **Install the Libraries**:
   - In the Arduino IDE, go to **Sketch > Include Library > Manage Libraries**.
   - Install the following libraries:
     - `DHT` for the DHT22 sensor.
     - `DFRobot_MultiGasSensor` for the gas sensor.
     - `DFRobot_OxygenSensor` for the oxygen sensor.

3. **Upload the Code**:
   - Open the Arduino IDE, copy and paste the provided code into a new sketch.
   - Select the correct board and port from **Tools > Board** and **Tools > Port**.
   - Click the **Upload** button to upload the code to your Arduino.

4. **Monitor the Data**:
   - Once the code is uploaded, open the Serial Monitor (set to 9600 baud).
   - The sensor readings will start appearing in the Serial Monitor once the system is initialized.

5. **View the Output**:
   - The system will display real-time data from the sensors every second, providing readings for temperature, humidity, CO2 concentration, NH3 concentration, oxygen levels, dust concentration, and light intensity.

## Code

```cpp
#include "DHT.h"
#include "DFRobot_MultiGasSensor.h"
#include "DFRobot_OxygenSensor.h"

#define DHTPIN 2       // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22  // DHT 22 (AM2302)
#define MG_PIN (A1)           // Analog pin for CO2 sensor
#define BOOL_PIN (2)          // Digital pin for BOOL signal (optional)
#define DC_GAIN (8.5)         // Gain of the CO2 sensor's amplifier
#define READ_SAMPLE_INTERVAL (50)   // Interval between sensor readings in ms
#define READ_SAMPLE_TIMES (5)       // Number of samples to average
#define ZERO_POINT_VOLTAGE (0.220)  // Voltage at 400ppm CO2
#define REACTION_VOLTAGE (0.030)    // Voltage change when concentration increases to 1000ppm
#define Oxygen_IICAddress 0x73  // Default I2C address of the oxygen sensor
#define COLLECT_NUMBER 10           // Number of readings to average

// CO2 sensor curve parameters
float CO2Curve[3] = {2.602, ZERO_POINT_VOLTAGE, (REACTION_VOLTAGE / (2.602 - 3))};
DFRobot_GAS_I2C gas(&Wire, 0x74);  // Multi-gas sensor I2C address
DFRobot_OxygenSensor oxygen;

const uint8_t VOUT_PIN = A0;
const uint8_t LED_PIN  = 3;
const float K_V_PER_0_1MG = 0.5f;
float voc = 0.0f;
int ldrPin = A2;      // LDR connected to analog pin A2
int sensorValue = 0;   // Variable to store the analog value
float voltage = 0.0;   // Voltage across the LDR
float resistance = 0.0; // Resistance of the LDR
float lux = 0.0;       // Calculated lux value

const float R_fixed = 10000.0;  // 10k ohms, typical for LDR voltage divider
const float calibrationFactor = 1000.0; // Calibration constant for LDR

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);
  
  // Initialize sensors
  dht.begin();
  
  // Initialize Multi-gas sensor
  while (!gas.begin()) {
    Serial.println("Sensor not detected! Check I2C wiring/address.");
    delay(1000);
  }
  
  // Set acquisition mode for gas sensor
  gas.changeAcquireMode(gas.INITIATIVE);
  gas.setTempCompensation(gas.ON);
  delay(1000);

  // Initialize Oxygen sensor
  while (!oxygen.begin(Oxygen_IICAddress)) {
    Serial.println("Oxygen sensor not detected! Retrying...");
    delay(1000);
  }
  Serial.println("Oxygen sensor connected successfully.");
  
  // Set the LED pin mode for Dust sensor
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);                // LED OFF
  analogReadResolution(10);  // Set ADC resolution to 10-bit
  voc = calibrateVoc(200);  // Calibrate Dust sensor
}

void loop() {
  // Read and display DHT22 sensor values
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  float hic = dht.computeHeatIndex(t, h, false);

  // Read and display CO2 sensor values
  float volts = MGRead(MG_PIN);
  int ppm = MGGetPercentage(volts, CO2Curve);

  // Read and display NH3 sensor values
  float nh3Concentration = gas.readGasConcentrationPPM();

  // Read and display O2 sensor values
  float oxygenPercent = oxygen.getOxygenData(COLLECT_NUMBER);

  // Read and display Dust sensor values
  float dust_mg_m3 = readDust();

  // Read and display LDR sensor values
  sensorValue = analogRead(ldrPin);
  voltage = sensorValue * (5.0 / 1023.0);
  resistance = R_fixed * ((5.0 / voltage) - 1);
  lux = (1023 - sensorValue) * calibrationFactor / 1023.0;

  // Print all the readings in one line with proper units
  Serial.print("Temp: ");
  Serial.print(t);
  Serial.print("°C  Hum: ");
  Serial.print(h);
  Serial.print("%  Heat Index: ");
  Serial.print(hic);
  Serial.print("°C  CO2: ");
  if (ppm == -1) {
    Serial.print("<400 ppm");
  } else {
    Serial.print(ppm);
    Serial.print(" ppm");
  }
  Serial.print("  NH3: ");
  Serial.print(nh3Concentration);
  Serial.print(" ppm  O2: ");
  Serial.print(oxygenPercent);
  Serial.print("%vol  Dust: ");
  Serial.print(dust_mg_m3);
  Serial.print(" mg/m3  LDR: ");
  Serial.print(lux);
  Serial.println(" lux");

  delay(1000);  // Delay for 1 second before next reading
}
```
## Result Display Example

```txt
Temp: 25.90°C  Hum: 53.80%  Heat Index: 25.95°C  CO2: 456 ppm  NH3: 0.67 ppm  O2: 21.17%vol  Dust: 0.00 mg/m3  LDR: 434.02 lux
Temp: 25.90°C  Hum: 53.80%  Heat Index: 25.95°C  CO2: 543 ppm  NH3: 0.67 ppm  O2: 21.16%vol  Dust: 0.00 mg/m3  LDR: 435.00 lux
Temp: 25.90°C  Hum: 53.80%  Heat Index: 25.95°C  CO2: 555 ppm  NH3: 0.67 ppm  O2: 21.06%vol  Dust: 0.00 mg/m3  LDR: 435.97 lux
```