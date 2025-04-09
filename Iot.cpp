#include <Wire.h>
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "DHTesp.h"
#include <LiquidCrystal_I2C.h>

#define PPM_PIN 16 // Pin connected to MQ135 sensor

#define PIR_PIN 2
#define ULTRASONIC_TRIG_PIN 5
#define ULTRASONIC_ECHO_PIN 13
#define LED_RED 12
#define LED_YELLOW 4
#define LED_GREEN 27

const char *ssid = "IOT";
const char *password = "";
const char *webhookURL = "https://webhookwizard.com/api/webhook/in?key=sk_384fc8b2-402d-4151-9b6f-a82ddb67b8ef";
const int DHT_PIN = 15;

long distance;
DHTesp dhtSensor;
LiquidCrystal_I2C lcd(0x27, 16, 2); // Address 0x27, 16 columns, 2 rows

void setup_wifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println("Connected");
}

void sendWebhook(String data) {
  HTTPClient http;
  http.begin(webhookURL);
  http.addHeader("Content-Type", "application/json");
  http.POST(data);
  http.end();
}

void processSensorData(long distance) {
  // Traffic light control based on distance
  if (distance < 20) {
    // Unsafe Distance - Stop Traffic
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_RED, HIGH);
    lcd.clear();
    lcd.print("Unsafe Distance");
    lcd.setCursor(0, 1);
    lcd.print("Stop Traffic");
    Serial.println("Unsafe Distance - Stop Traffic");
  } else if (distance > 20 && distance <= 50) {
    // Moderate Distance - Use Caution
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_YELLOW, HIGH);
    digitalWrite(LED_RED, LOW);
    lcd.clear();
    lcd.print("Moderate Distance");
    lcd.setCursor(0, 1);
    lcd.print("Use Caution");
    Serial.println("Moderate Distance - Use Caution");
  } else if (distance > 50 && distance < 400) {
    // Safe Distance - Allow Traffic
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_RED, LOW);
    lcd.clear();
    lcd.print("Safe Distance");
    lcd.setCursor(0, 1);
    lcd.print("Allow Traffic");
    Serial.println("Safe Distance - Allow Traffic");
  }
}

void setup() {
  Wire.begin(21, 22); // I2C pins (SDA, SCL)
  Serial.begin(115200);
  lcd.init(); // Initialize the LCD
  lcd.backlight(); // Turn on backlight
  pinMode(PIR_PIN, INPUT);
  pinMode(ULTRASONIC_TRIG_PIN, OUTPUT);
  pinMode(ULTRASONIC_ECHO_PIN, INPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  // Initialize DHT sensor
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);

  // Initialize WiFi
  setup_wifi();
}

void loop() {
  // Read PIR sensor
  int pirValue = digitalRead(PIR_PIN);
  if (pirValue == HIGH) {
    // Motion detected - Stop Traffic
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_GREEN, LOW);
    lcd.clear();
    lcd.print("Motion Detected");
    lcd.setCursor(0, 1);
    lcd.print("Stop Traffic");
    Serial.println("Motion detected - Stop Traffic");
  } else {
    // Read ultrasonic sensor
    digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(ULTRASONIC_TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
    long duration = pulseIn(ULTRASONIC_ECHO_PIN, HIGH);
    distance = duration * 0.034 / 2;

    // Process sensor data and control traffic lights
    processSensorData(distance);

    // Read PPM value from MQ135 sensor
    int16_t ppmValue = analogRead(PPM_PIN);
    // Map the correct value as the ADC expects a max voltage of 3.3 V, but we're sending 5 V
    int mappedppmValue = ppmValue / 4.095;
    Serial.print("PPM: ");
    Serial.println(mappedppmValue);

    // Send sensor data via webhook
    String sensorData = "{\"event\": \"SensorData\", \"distance\": " + String(distance) + ", \"ppm\": " + String(mappedppmValue) + "}";
    sendWebhook(sensorData);
  }

  delay(1000);

  // Read temperature and humidity
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  float temperature = data.temperature;
  float humidity = data.humidity;

  // Send sensor data via webhook
  String sensorData = "{\"event\": \"SensorData\", \"distance\": " + String(distance) + ", \"temperature\": " + String(temperature) + ", \"humidity\": " + String(humidity) + "}";
  sendWebhook(sensorData);

  delay(1000);
}