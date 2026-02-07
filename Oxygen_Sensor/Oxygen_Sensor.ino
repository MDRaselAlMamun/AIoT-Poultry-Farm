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
