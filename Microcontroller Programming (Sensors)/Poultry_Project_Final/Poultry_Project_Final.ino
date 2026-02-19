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

// CO2 sensor functions
float MGRead(int mg_pin) {
  float v = 0;
  for (int i = 0; i < READ_SAMPLE_TIMES; i++) {
    v += analogRead(mg_pin);
    delay(READ_SAMPLE_INTERVAL);
  }
  v = (v / READ_SAMPLE_TIMES) * 5.0 / 1023;
  return v;
}

int MGGetPercentage(float volts, float *pcurve) {
  if ((volts / DC_GAIN) >= ZERO_POINT_VOLTAGE) {
    return -1;
  } else {
    return pow(10, ((volts / DC_GAIN) - pcurve[1]) / pcurve[2] + pcurve[0]);
  }
}

// Dust sensor function
float readDust() {
  float sumV = 0.0f;
  for (uint8_t i = 0; i < 50; i++) sumV += readVoltageOnce();
  float vo = sumV / 50;
  float deltaV = vo - voc;
  if (deltaV < 0) deltaV = 0;
  return deltaV * (0.1f / K_V_PER_0_1MG);
}

float readVoltageOnce() {
  digitalWrite(LED_PIN, LOW);
  delayMicroseconds(280);
  uint16_t raw = analogRead(VOUT_PIN);
  delayMicroseconds(40);
  digitalWrite(LED_PIN, HIGH);
  delayMicroseconds(9680);
  return (raw * 5.0f) / 1023;
}

float calibrateVoc(uint16_t samples) {
  float sum = 0.0f;
  for (uint16_t i = 0; i < samples; i++) sum += readVoltageOnce();
  return sum / samples;
}
