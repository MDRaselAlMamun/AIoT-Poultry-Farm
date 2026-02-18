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
