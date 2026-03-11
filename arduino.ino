/*
  ============================================================
  Soil Moisture, Temperature & Humidity Monitor
  Hardware : NodeMCU ESP8266 + DHT11/22 + Soil Moisture Sensor
  Platform : Blynk IoT (blynk.cloud) + ThingSpeak
  
  Blynk Virtual Pins:
    V0 - Soil Moisture (%)
    V1 - Temperature (°C)
    V2 - Humidity (%)
    V3 - Soil Status (Wet / Moist / Dry)

  ThingSpeak Fields:
    Field 1 - Soil Moisture (%)
    Field 2 - Temperature (°C)
    Field 3 - Humidity (%)
    Field 4 - Soil Raw ADC value
  ============================================================
*/

// ==================== BLYNK CONFIG ====================
#define BLYNK_TEMPLATE_ID "TMPL3ngYp32s8"
#define BLYNK_TEMPLATE_NAME "Smart Plant Monitoring"
#define BLYNK_AUTH_TOKEN "re509CxqzW_KdBWJO8ObYzf7hnHOEB4P"
#define BLYNK_PRINT Serial

// ==================== LIBRARIES ====================
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>
#include <ThingSpeak.h>

// ==================== WiFi CREDENTIALS ====================
char ssid[] = "iPhone";
char pass[] = "123456789";

// ==================== THINGSPEAK CONFIG ====================
unsigned long TS_CHANNEL_ID  = 3293742;          // Your ThingSpeak Channel ID
const char*   TS_WRITE_KEY   = "Q3TEFCGQ9HXF5WNE"; // Write API Key

// ==================== PIN DEFINITIONS ====================
#define DHTPIN     D2     // DHT sensor data pin
#define DHTTYPE    DHT11  // Change to DHT22 if using DHT22
#define SOIL_PIN   A0     // Soil moisture analog pin

// ==================== SOIL THRESHOLDS ====================
#define SOIL_DRY   800    // Raw ADC above this = DRY
#define SOIL_WET   400    // Raw ADC below this = WET

// ==================== TIMING ====================
// ThingSpeak free plan allows updates every 15 seconds minimum
#define BLYNK_INTERVAL      2000   // Send to Blynk every 2 sec
#define THINGSPEAK_INTERVAL 8000  // Send to ThingSpeak every 16 sec

// ==================== OBJECTS ====================
DHT dht(DHTPIN, DHTTYPE);
BlynkTimer timer;
WiFiClient tsClient;

// ==================== GLOBAL VARIABLES ====================
float temperature = 0.0;
float humidity    = 0.0;
int   soilRaw     = 0;
int   soilPercent = 0;
String soilStatus = "";

// =====================================================
//   FUNCTION: Read Sensors
// =====================================================
void readSensors() {

  // ---------- Read DHT11/22 ----------
  float newTemp = dht.readTemperature();
  float newHum  = dht.readHumidity();

  if (isnan(newTemp) || isnan(newHum)) {
    Serial.println("WARNING: DHT sensor read failed! Check wiring.");
  } else {
    temperature = newTemp;
    humidity    = newHum;
  }

  // ---------- Read Soil Moisture ----------
  soilRaw     = analogRead(SOIL_PIN);
  soilPercent = map(soilRaw, 1023, 0, 0, 100);
  soilPercent = constrain(soilPercent, 0, 100);

  // ---------- Soil Status ----------
  if (soilRaw > SOIL_DRY) {
    soilStatus = "DRY";
  } else if (soilRaw < SOIL_WET) {
    soilStatus = "WET";
  } else {
    soilStatus = "MOIST";
  }

  // ---------- Serial Output ----------
  Serial.println("==============================");
  Serial.print("Temperature : "); Serial.print(temperature); Serial.println(" C");
  Serial.print("Humidity    : "); Serial.print(humidity);    Serial.println(" %");
  Serial.print("Soil Raw    : "); Serial.println(soilRaw);
  Serial.print("Soil Level  : "); Serial.print(soilPercent); Serial.println(" %");
  Serial.print("Soil Status : "); Serial.println(soilStatus);
  Serial.println("==============================\n");
}

// =====================================================
//   FUNCTION: Send Data to Blynk (every 2 sec)
// =====================================================
void sendToBlynk() {
  readSensors();  // Always read fresh data before sending

  Blynk.virtualWrite(V0, soilPercent);
  Blynk.virtualWrite(V1, temperature);
  Blynk.virtualWrite(V2, humidity);
  Blynk.virtualWrite(V3, soilStatus);

  Serial.println(">> Sent to Blynk");
}

// =====================================================
//   FUNCTION: Send Data to ThingSpeak (every 16 sec)
// =====================================================
void sendToThingSpeak() {

  // Set each field
  ThingSpeak.setField(1, soilPercent);   // Field 1: Soil Moisture %
  ThingSpeak.setField(2, temperature);   // Field 2: Temperature
  ThingSpeak.setField(3, humidity);      // Field 3: Humidity
  ThingSpeak.setField(4, soilRaw);       // Field 4: Raw ADC value

  // Set status message
  ThingSpeak.setStatus(soilStatus);

  // Write to ThingSpeak channel
  int responseCode = ThingSpeak.writeFields(TS_CHANNEL_ID, TS_WRITE_KEY);

  if (responseCode == 200) {
    Serial.println(">> Sent to ThingSpeak: OK");
  } else {
    Serial.print(">> ThingSpeak Error Code: ");
    Serial.println(responseCode);
  }
}

// =====================================================
//   SETUP
// =====================================================
void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println("\nStarting Soil Monitor (Blynk + ThingSpeak)...");

  // Start DHT
  dht.begin();

  // Connect to WiFi + Blynk
  Serial.println("Connecting to WiFi & Blynk...");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  Serial.println("Blynk Connected!");

  // Initialize ThingSpeak
  ThingSpeak.begin(tsClient);
  Serial.println("ThingSpeak Initialized!");

  // Timer 1: Blynk every 2 seconds
  timer.setInterval(BLYNK_INTERVAL, sendToBlynk);

  // Timer 2: ThingSpeak every 16 seconds
  timer.setInterval(THINGSPEAK_INTERVAL, sendToThingSpeak);

  Serial.println("Setup Complete! Monitoring started...\n");
}

// =====================================================
//   LOOP
// =====================================================
void loop() {
  Blynk.run();
  timer.run();
}