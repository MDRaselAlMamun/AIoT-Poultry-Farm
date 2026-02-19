/*
  GP2Y1010AU0F + Gravity Dust Sensor Adapter + Arduino UNO R4
  Vo  -> A0
  LED -> D3 (often active-LOW: LOW=ON, HIGH=OFF)
  Output: mg/m3
*/

const uint8_t VOUT_PIN = A0;
const uint8_t LED_PIN  = 3;

// Sharp timing: T=10ms, PW=0.32ms, sample at 0.28ms after LED ON. :contentReference[oaicite:9]{index=9}
const uint16_t SAMPLING_US = 280;
const uint16_t DELTA_US    = 40;     // keeps LED ON about 320us total
const uint16_t SLEEP_US    = 9680;   // total ~10ms cycle

// UNO R4 default is 10-bit unless changed; we force 10-bit here. :contentReference[oaicite:10]{index=10}
const uint8_t  ADC_BITS = 10;
const uint16_t ADC_MAX  = (1u << ADC_BITS) - 1u;  // 1023

// Measure your actual 5V for best accuracy.
const float VREF = 5.0f;

// Sharp datasheet typical sensitivity K=0.5 V per 0.1 mg/m3. :contentReference[oaicite:11]{index=11}
float K_V_PER_0_1MG = 0.5f;
float voc = 0.0f;

float readVoltageOnce() {
  digitalWrite(LED_PIN, LOW);                 // LED ON (active-LOW on many adapters)
  delayMicroseconds(SAMPLING_US);

  uint16_t raw = analogRead(VOUT_PIN);

  delayMicroseconds(DELTA_US);
  digitalWrite(LED_PIN, HIGH);                // LED OFF
  delayMicroseconds(SLEEP_US);

  return (raw * VREF) / ADC_MAX;
}

float calibrateVoc(uint16_t samples) {
  float sum = 0.0f;
  for (uint16_t i = 0; i < samples; i++) sum += readVoltageOnce();
  return sum / samples;
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);                // LED OFF

  analogReadResolution(ADC_BITS);

  delay(1000);                                 // device ready within ~1s (application note). :contentReference[oaicite:12]{index=12}
  voc = calibrateVoc(200);

  Serial.print("Voc (baseline) = ");
  Serial.print(voc, 3);
  Serial.println(" V");
}

void loop() {
  const uint8_t N = 50;
  float sumV = 0.0f;
  for (uint8_t i = 0; i < N; i++) sumV += readVoltageOnce();
  float vo = sumV / N;

  float deltaV = vo - voc;
  if (deltaV < 0) deltaV = 0;

  // mg/m3 = ΔV * (0.1 / K)
  float dust_mg_m3 = deltaV * (0.1f / K_V_PER_0_1MG);

  Serial.print("Vo=");
  Serial.print(vo, 3);
  Serial.print("V  Voc=");
  Serial.print(voc, 3);
  Serial.print("V  Dust=");
  Serial.print(dust_mg_m3, 3);
  Serial.println(" mg/m3");

  delay(500);
}
