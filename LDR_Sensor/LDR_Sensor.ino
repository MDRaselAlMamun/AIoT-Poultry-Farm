int ldrPin = A2;      // LDR connected to analog pin A2
int sensorValue = 0;   // Variable to store the analog value
float voltage = 0.0;   // Voltage across the LDR
float resistance = 0.0; // Resistance of the LDR
float lux = 0.0;       // Calculated lux value

// Fixed resistor value in the voltage divider (in ohms)
const float R_fixed = 10000.0;  // 10k ohms, typical for LDR voltage divider

// Calibration constant (adjust based on your LDR)
const float calibrationFactor = 1000.0;

void setup() {
  Serial.begin(9600);  // Start serial communication
}

void loop() {
  // Read the analog value from LDR
  sensorValue = analogRead(ldrPin);
  
  // Convert the analog value (0-1023) to voltage (0-5V)
  voltage = sensorValue * (5.0 / 1023.0);

  // Calculate the LDR resistance using the voltage divider formula
  // Vout = Vin * (R_LDR / (R_LDR + R_fixed)) ==> Solve for R_LDR
  resistance = R_fixed * ((5.0 / voltage) - 1);

  // Invert the analog value to calculate lux:
  // 0 analog value means bright light, 1023 means dark.
  lux = (1023 - sensorValue) * calibrationFactor / 1023.0;
  
  // Print the results to the Serial Monitor
  Serial.print("Analog Value: ");
  Serial.print(sensorValue);
  Serial.print(" | Voltage: ");
  Serial.print(voltage);
  Serial.print(" | Resistance: ");
  Serial.print(resistance);
  Serial.print(" | Lux: ");
  Serial.println(lux);

  delay(1000);  // Delay for 1 second before reading again
}
