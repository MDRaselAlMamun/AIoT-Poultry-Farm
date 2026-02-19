# DFRobot Gravity: I2C Oxygen / O2 Sensor

## Description
This project reads **oxygen concentration** from the **DFRobot Gravity: I2C Oxygen / O2 Sensor** using an **Arduino UNO R4** and displays the value in the Serial Monitor.

- Communication: **I2C (SDA/SCL)**
- Output unit: **%vol (oxygen concentration percent by volume)**
- The sensor needs time to stabilize (often **~10 minutes**) for more consistent readings.

---

## Wiring (Arduino UNO R4)

| Sensor Pin | Arduino UNO R4 Pin |
|-----------:|---------------------|
| VCC        | 5V |
| GND        | GND |
| SDA        | SDA (D18) |
| SCL        | SCL (D19) |

> You can use the dedicated **SDA/SCL header pins** on the UNO R4 (same I2C bus).

---

## Procedure

### 1) Find the I2C Address (Recommended)
Before coding, run an **I2C scanner** sketch to detect the sensor address (for example, `0x73`).

#### I2C Scanner Sketch
```cpp
#include <Wire.h>

void setup() {
  Serial.begin(9600);
  while (!Serial) {}
  Wire.begin();
  Serial.println("I2C scanner running...");
}

void loop() {
  byte error, address;
  int found = 0;

  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Found I2C device at 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      found++;
    }
  }

  if (found == 0) Serial.println("No I2C devices found.");
  Serial.println("-----");
  delay(2000);
}
```

### 2) Select the Correct Address in Code

Use the detected address (most commonly 0x73) and match it to the library constants below:

```cpp
/**
 * i2c slave Address, The default is ADDRESS_3.
 * ADDRESS_0   0x70  i2c device address.
 * ADDRESS_1   0x71
 * ADDRESS_2   0x72
 * ADDRESS_3   0x73
 */
```

### Example
If the scanner shows **0x73**, set:
```cpp
#define OXYGEN_I2C_ADDRESS ADDRESS_3
```
## 3) Install Library

Install the DFRobot library:

1. Open **Arduino IDE**
2. Go to **Tools → Manage Libraries**
3. Search and install: **DFRobot_OxygenSensor**

---

## 4) Upload and Read Values

1. Upload the oxygen sensor code (below) to your **Arduino UNO R4**
2. Open **Serial Monitor** and set baud rate to **9600**
3. Wait for sensor stabilization (**~10 minutes**) for best consistency

## Code

```cpp
#include "DFRobot_OxygenSensor.h"

#define Oxygen_IICAddress ADDRESS_3  // default 0x73
#define COLLECT_NUMBER  10           // number of readings to average

DFRobot_OxygenSensor oxygen;

void setup() {
  Serial.begin(9600);
  
  // Start sensor with I2C address
  while (!oxygen.begin(Oxygen_IICAddress)) {
    Serial.println("I2C connection failed. Retrying...");
    delay(1000);
  }
  Serial.println("I2C connection success.");
}

void loop() {
  float oxygenPercent = oxygen.getOxygenData(COLLECT_NUMBER);

  Serial.print("Oxygen Concentration: ");
  Serial.print(oxygenPercent);
  Serial.println(" %vol");

  delay(1000);  // 1-second delay
}
```
## Result Display Example

```txt
Starting DFRobot O2 sensor...
O2 sensor connected!
Oxygen concentration: 20.86 %vol
Oxygen concentration: 20.87 %vol
Oxygen concentration: 20.85 %vol
```