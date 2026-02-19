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
