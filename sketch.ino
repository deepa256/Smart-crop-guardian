#include <WiFi.h>
#include <FirebaseESP32.h>
#include <HardwareSerial.h>
#include <DHT.h>

// ================= 1. NETWORK & FIREBASE CONFIG =================
#define WIFI_SSID "Nandhu"        // உங்கள் WiFi / Hotspot பெயர்
#define WIFI_PASSWORD "12345678"  // WiFi பாஸ்வேர்டு

#define FIREBASE_HOST "smart-crop-guardian-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "ADxocgMHFFHtZnP1lrky40OVLsCEdCkSBpADZxiU" // நீங்கள் காப்பி செய்த Secret Key

// ================= 2. PIN DEFINITIONS =================
#define SOIL_PIN 34       // Soil Moisture Sensor
#define PIR_PIN 27        // PIR Motion Sensor
#define RELAY_PIN 26      // Water Pump Relay
#define DHTPIN 4          // DHT11 Sensor
#define DHTTYPE DHT11

HardwareSerial gsm(2); // SIM800L (Tx2: Pin 17, Rx2: Pin 16)

DHT dht(DHTPIN, DHTTYPE);
FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long lastSmsTime = 0;
const unsigned long smsInterval = 30000;

void setup() {
  Serial.begin(115200);
  gsm.begin(9600, SERIAL_8N1, 16, 17);
  
  pinMode(PIR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  
  dht.begin();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void sendSMS(String message) {
  gsm.println("AT+CMGF=1");
  delay(1000);
  gsm.println("AT+CMGS=\"+917695990561\""); // உங்கள் மொபைல் எண்
  delay(1000);
  gsm.print(message);
  delay(100);
  gsm.write(26);
  delay(3000);
}

void loop() {
  int rawSoil = analogRead(SOIL_PIN);
  int soilPercent = map(rawSoil, 4095, 0, 0, 100);
  soilPercent = constrain(soilPercent, 0, 100);

  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int motion = digitalRead(PIR_PIN);

  if (isnan(temp)) temp = 0.0;
  if (isnan(hum)) hum = 0.0;

  Firebase.setInt(firebaseData, "/Sensor/SoilMoisture", soilPercent);
  Firebase.setFloat(firebaseData, "/Sensor/Temperature", temp);
  Firebase.setFloat(firebaseData, "/Sensor/Humidity", hum);
  Firebase.setInt(firebaseData, "/Sensor/Motion", motion);

  if (soilPercent < 30) {
    digitalWrite(RELAY_PIN, HIGH);
    Firebase.setBool(firebaseData, "/Controls/PumpState", true);
  } else if (soilPercent > 70) {
    digitalWrite(RELAY_PIN, LOW);
    Firebase.setBool(firebaseData, "/Controls/PumpState", false);
  }

  if (motion == HIGH) {
    if (millis() - lastSmsTime > smsInterval) {
      sendSMS("SMART CROP GUARDIAN ALERT: Intruder detected!");
      lastSmsTime = millis();
    }
  }

  delay(2000);
}
