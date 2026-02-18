# Gravity: Analog Electrochemical CO<sub>2</sub> Sensor

## Description

This project allows you to read **carbon dioxide (CO<sub>2</sub>) concentration** from the **Gravity: Analog Electrochemical CO<sub>2</sub> Sensor** and display the values on the **Serial Monitor** using an **Arduino**.

- **Sensor Type**: Electrochemical (analog output)
- **Measuring Range**: 0 - 10000 ppm of CO<sub>2</sub>
- **Output Voltage**: Analog voltage corresponding to the CO<sub>2</sub> concentration
- **Communication Mode**: Analog output (direct connection to Arduino's analog input pin)
  
> **Warm-up time**: The sensor requires approximately **5 minutes** to stabilize after powering on. If the sensor has not been used for extended periods, allow it to preheat for **48 hours**.

---

## Wiring

Connect the Gravity CO<sub>2</sub> sensor to your **Arduino Uno R4** as follows:

| Sensor Pin | Arduino UNO R4 Pin |
|------------|---------------------|
| VCC        | 5V                  |
| GND        | GND                 |
| AOUT       | A1 (Analog input)   |

---

## Procedure

### 1) Set Up the Sensor

- **VCC** pin to the **5V** pin on Arduino.
- **GND** pin to the **GND** pin on Arduino.
- **AOUT** (Analog Output) pin to the **A1** pin on Arduino.

### 2) Measure Voltage

The sensor provides an analog voltage that corresponds to the CO<sub>2</sub> concentration. The conversion of the analog voltage to CO<sub>2</sub> concentration in ppm is based on the following:

- **Zero Point Voltage (V)**: This is the voltage corresponding to a baseline CO<sub>2</sub> level, typically around 400 ppm. The sensor outputs a voltage of **2.4V** at 400 ppm of CO<sub>2</sub>.

To read the voltage, use the following code:

```cpp
#define PIN A1

long readVcc_mV() {
  // Measure Vcc using the internal 1.1V reference (ATmega328P)
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1); // 1.1V (Vbg) channel
  delay(2);
  ADCSRA |= _BV(ADSC);
  while (bit_is_set(ADCSRA, ADSC));
  uint16_t adc = ADC;
  long vcc = 1125300L / adc; // mV
  return vcc;
}

float ema = 0;

void setup() {
  Serial.begin(9600);
}

void loop() {
  // Discard first read after mux switch
  analogRead(PIN);

  long sum = 0;
  for (int i = 0; i < 50; i++) {
    sum += analogRead(PIN);
    delay(10);
  }
  float adc = sum / 50.0;
  float vcc = readVcc_mV() / 1000.0;
  float vout = adc * (vcc / 1023.0);

  // Simple smoothing of readings
  if (ema == 0) ema = vout;
  ema = 0.9 * ema + 0.1 * vout;

  Serial.print("Vcc="); Serial.print(vcc, 3); Serial.print(" V  ");
  Serial.print("ADC="); Serial.print((int)adc);
  Serial.print("  AOUT="); Serial.print(vout, 3);
  Serial.print(" V  EMA="); Serial.print(ema, 3);
  Serial.println(" V");

  delay(1000);
}
```

## 3) Calibrate CO<sub>2</sub> Concentration

Before using the sensor for accurate CO<sub>2</sub> readings, you need to calibrate it:

1. Provide stable power to the sensor and allow it to warm-up for about 48 hours.

2. After the sensor has stabilized, measure the output voltage.

3. Modify the code to match your Zero Point Voltage:

For example, if you measure a voltage of 2.4V, divide it by 8.5 to get:

```cpp
#define ZERO_POINT_VOLTAGE (2.4 / 8.5)  // 0.282
```
Update the **ZERO_POINT_VOLTAGE** definition in the code as follows:

```cpp
#define ZERO_POINT_VOLTAGE (0.282)
```

### 4) Measure CO<sub>2</sub> Concentration

Now, you can measure the **CO<sub>2</sub>** concentration based on the sensor's output voltage.

1. Upload the provided code to your **Arduino UNO R4**.

2. Open the **Serial Monitor** in Arduino IDE and set the baud rate to 9600.

Here's the code to calculate and display the CO<sub>2</sub> concentration:


```cpp

#define MG_PIN (A1)           // Analog pin for sensor output
#define BOOL_PIN (2)          // Digital pin for BOOL signal (optional)
#define DC_GAIN (8.5)         // Gain of the sensor's amplifier

#define READ_SAMPLE_INTERVAL (50)   // Interval between sensor readings in ms
#define READ_SAMPLE_TIMES (5)       // Number of samples to average

#define ZERO_POINT_VOLTAGE (0.220)  // Voltage at 400ppm CO2
#define REACTION_VOLTAGE (0.030)    // Voltage change when concentration increases to 1000ppm

// CO2 sensor curve parameters: [a, b, c] where y = a * log(x) + b
float CO2Curve[3] = {2.602, ZERO_POINT_VOLTAGE, (REACTION_VOLTAGE / (2.602 - 3))};

void setup() {
    Serial.begin(9600);                   // Initialize Serial Monitor
    pinMode(BOOL_PIN, INPUT);             // Set BOOL_PIN as input
    digitalWrite(BOOL_PIN, HIGH);         // Enable pull-up resistor on BOOL_PIN (optional)
    Serial.println("CO2 Sensor Demonstration");  // Display introductory message
}

void loop() {
    float volts = MGRead(MG_PIN);         // Read the sensor voltage
    int ppm = MGGetPercentage(volts, CO2Curve);  // Convert voltage to CO2 concentration (ppm)

    // Display the sensor voltage and CO2 concentration
    Serial.print("Voltage: ");
    Serial.print(volts, 3);               // Show voltage with 3 decimal places
    Serial.print(" V  CO2: ");
    if (ppm == -1) {
        Serial.print("<400 ppm");         // If concentration is below 400 ppm, display "<400 ppm"
    } else {
        Serial.print(ppm);                // Show CO2 concentration in ppm
        Serial.print(" ppm");
    }
    Serial.println();

    delay(500);  // Delay between readings
}

// Function to read the analog value from the sensor and calculate voltage
float MGRead(int mg_pin) {
    float v = 0;

    // Average multiple readings for stable results
    for (int i = 0; i < READ_SAMPLE_TIMES; i++) {
        v += analogRead(mg_pin);
        delay(READ_SAMPLE_INTERVAL);  // Short delay between readings
    }
    v = (v / READ_SAMPLE_TIMES) * 5.0 / 1023;  // Convert to voltage (0-5V)
    return v;
}

// Function to calculate CO2 concentration based on the voltage
int MGGetPercentage(float volts, float *pcurve) {
    // If voltage exceeds the ZERO_POINT_VOLTAGE, return -1 (indicating <400 ppm)
    if ((volts / DC_GAIN) >= ZERO_POINT_VOLTAGE) {
        return -1;
    } else {
        // Calculate CO2 concentration using the provided curve (logarithmic model)
        return pow(10, ((volts / DC_GAIN) - pcurve[1]) / pcurve[2] + pcurve[0]);
    }
}

```

## Example Output

```txt
Voltage: 0.400 V  CO2: 400 ppm
Voltage: 0.390 V  CO2: 300 ppm
Voltage: 0.410 V  CO2: 500 ppm
```