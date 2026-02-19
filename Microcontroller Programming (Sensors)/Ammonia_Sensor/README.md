# Gravity: NH3 Sensor (Calibrated) — I2C & UART 

## Description
This project reads **ammonia (NH3) concentration** from the **DFRobot Gravity: NH3 Sensor (Calibrated) - I2C** and prints the values to the **Serial Monitor**.

- Interfaces: **I2C / UART / Analog** (module supports multiple outputs).
- Default communication mode: **I2C** when **SEL = 0**.
- Default I2C address: **0x74** (changeable via DIP switches). 

> Warm-up: initial power-on typically needs **> 5 minutes** preheating; if unused for a long time, **> 24 hours** preheat is recommended.

---

## Wiring

### I2C (Recommended)
Set sensor **SEL = 0** for I2C.

| Sensor Pin | Arduino UNO R4 Pin |
|-----------:|---------------------|
| VCC        | 5V |
| GND        | GND |
| SDA        | SDA (D18 / A4) |
| SCL        | SCL (D19 / A5) |

## Procedure

### 1) Set Communication Mode
- **SEL = 0** → I2C mode (default)

### 2) Find the I2C Address (Run an I2C Scanner)
The default I2C address is **0x74**, and it can be changed using A0/A1 DIP switches. 

Use this scanner to detect the address on your setup:

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
## 3) Address Table (A0/A1 → I2C Address)

Use this mapping when selecting the address in code:

```cpp
/**
 * i2c slave Address (default is 0x74).
 * ADDRESS_0   0x74  (A0=0, A1=0)
 * ADDRESS_1   0x75  (A0=0, A1=1)
 * ADDRESS_2   0x76  (A0=1, A1=0)
 * ADDRESS_3   0x77  (A0=1, A1=1)
 */
```
> After changing SEL or the I2C address, power-cycle the sensor/module.

## 4) Install Library

Open **Arduino IDE**

Go to **Tools → Manage Libraries**

Install **DFRobot_MultiGasSensor** (DFRobot Gas Sensor Library)

## 5) Upload and Read Values

Upload the code below to your **Arduino UNO R4**

Open Serial Monitor and set baud rate to **9600**

Allow the sensor to **warm up (> 5 minutes)** for stable readings

## Code

```cpp
#include "DFRobot_MultiGasSensor.h"

// ===== I2C Settings =====
#define I2C_COMMUNICATION          // Use I2C communication mode
#define I2C_ADDRESS 0x74           // Default I2C address of the multi-gas sensor

// Create I2C sensor object (Wire = I2C bus)
DFRobot_GAS_I2C gas(&Wire, I2C_ADDRESS);

void setup() {
  Serial.begin(9600);

  // Initialize sensor and keep trying until it responds
  while (!gas.begin()) {
    Serial.println("Sensor not detected! Check I2C wiring/address.");
    delay(1000);
  }
  Serial.println("Sensor connected successfully!");

  // Set acquisition mode:
  // INITIATIVE = read values when you request them (recommended for simple loops)
  gas.changeAcquireMode(gas.INITIATIVE);
  delay(1000);

  // Enable temperature compensation for more accurate readings
  gas.setTempCompensation(gas.ON);
}

void loop() {
  // Print gas type name detected/selected by the board
  Serial.print("Ambient ");
  Serial.print(gas.queryGasType());

  // Read gas concentration in PPM
  Serial.print(" concentration: ");
  Serial.print(gas.readGasConcentrationPPM());
  Serial.println(" ppm");

  // Read board temperature in Celsius
  Serial.print("Board temperature: ");
  Serial.print(gas.readTempC());
  Serial.println(" °C");

  // Read sensor voltage (useful for diagnostics)
  Serial.print("Sensor voltage: ");
  Serial.print(gas.getSensorVoltage());
  Serial.println(" V");

  Serial.println();
  delay(1000);
}
```

## Result Display Example

```txt
Sensor connected successfully!
Ambient NH3 concentration: 2.10 ppm
Board temperature: 29.6 °C
Sensor voltage: 0.34 V

Ambient NH3 concentration: 2.06 ppm
Board temperature: 29.6 °C
Sensor voltage: 0.34 V

```