#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define DHTPIN 23
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define PIR_PIN 32
#define LDR_PIN 34
#define RELAY1_PIN 19
#define RELAY2_PIN 18
#define BUZZER_PIN 26

LiquidCrystal_I2C lcd(0x27, 16, 2);

int lastDisplayState = -1;
unsigned long lastMessageSwitch = 0;
int displayState = 0;

unsigned long lastRelay2OnTime = 0;
bool relay2Active = false;

int vehicleCount = 0;
bool lastPirState = false;
unsigned long lastCountTime = 0;
const unsigned long debounceDelay = 3000; 
float fuzzyTrapezoidal(float x, float a, float b, float c, float d) {
  if (x <= a || x >= d) return 0.0;
  else if (x >= b && x <= c) return 1.0;
  else if (x > a && x < b) return (x - a) / (b - a);
  else return (d - x) / (d - c);
}

float dryMembership(float h)     { return fuzzyTrapezoidal(h, 0, 0, 30, 45); }
float normalMembership(float h)  { return fuzzyTrapezoidal(h, 40, 45, 55, 60); }
float foggyMembership(float h)   { return fuzzyTrapezoidal(h, 55, 70, 100, 100); }

float darkMembership(float l)   { return fuzzyTrapezoidal(l, 0, 20, 80, 150); }
float dimMembership(float l)    { return fuzzyTrapezoidal(l, 200, 400, 500, 650); }
float brightMembership(float l) { return fuzzyTrapezoidal(l, 500, 600, 700, 800); }

void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(PIR_PIN, INPUT);
  pinMode(LDR_PIN, INPUT);
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(RELAY1_PIN, HIGH);  
  digitalWrite(RELAY2_PIN, HIGH);
  digitalWrite(BUZZER_PIN, LOW);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(3, 0);
  lcd.print("Penerangan");
  lcd.setCursor(1, 1);
  lcd.print("Jalan Otomatis");
  delay(2000);
  lcd.clear();
  
  lcd.setCursor(4, 0);
  lcd.print("Memulai");
  lcd.setCursor(3, 1);
  lcd.print("Sistem...");
  delay(2000);
  lcd.clear();
}

void loop() {
  int ldrValue = analogRead(LDR_PIN);  
  if (isnan(ldrValue)) ldrValue = 600;

  float humidityValue = dht.readHumidity();
  if (isnan(humidityValue)) humidityValue = 50.0;

  float temperatureValue = dht.readTemperature();
  if (isnan(temperatureValue)) temperatureValue = 0.0;

  bool pirDetected = digitalRead(PIR_PIN);

  float foggy = foggyMembership(humidityValue);
  float dark = darkMembership(ldrValue);
  float dim = dimMembership(ldrValue);

  bool currentPirState = digitalRead(PIR_PIN);
  if (currentPirState != lastPirState && currentPirState == HIGH && (millis() - lastCountTime > debounceDelay)) {
  vehicleCount++;
  lastCountTime = millis();
  }
  lastPirState = currentPirState;

  bool relay1On = (ldrValue < 500) || (foggy > 0.1);
  digitalWrite(RELAY1_PIN, relay1On ? HIGH : LOW); 

  if (relay1On && pirDetected) {
    lastRelay2OnTime = millis();
    relay2Active = true;
  }

  if (relay2Active && (millis() - lastRelay2OnTime >= 2000)) {
    relay2Active = false;
  }

  digitalWrite(RELAY2_PIN, (relay1On && relay2Active) ? HIGH : LOW);

  if (relay1On && relay2Active) {
    tone(BUZZER_PIN, 1000, 200);
    delay(250);
    tone(BUZZER_PIN, 1000, 200);
  } else if (relay1On) {
    tone(BUZZER_PIN, 1000, 200);
  } else {
    noTone(BUZZER_PIN);
  }

  if (millis() - lastMessageSwitch >= 5000) {
    displayState = (displayState + 1) % 3;
    lastMessageSwitch = millis();
  }

  String blynkMessage = (foggy > 0.1) ? "Kabut Tebal!" : "SELAMAT JALAN";

  if (displayState != lastDisplayState) {
    lcd.clear();
    if (displayState == 0) {
      lcd.setCursor(0, 0);
      lcd.print(blynkMessage.substring(0, 16));
      lcd.setCursor(0, 1);
      if (blynkMessage.length() > 16) {
        lcd.print(blynkMessage.substring(16, 32));
      } else {
        lcd.print("                ");
      }
    } else if (displayState == 1) {
      lcd.setCursor(0, 0);
      lcd.print("T:");
      lcd.print(temperatureValue, 1);
      lcd.print((char)223);
      lcd.print("C K:");
      lcd.print(ldrValue);
      lcd.setCursor(5, 1);
      lcd.print("H:");
      lcd.print((int)humidityValue);
      lcd.print("%");
    }
    lastDisplayState = displayState;
  }
  else if (displayState == 2) {
    lcd.setCursor(0, 0);
    lcd.print("Jumlah kendaraan:");
    lcd.setCursor(0, 1);
    lcd.print("       ");
    lcd.print(vehicleCount);
  }
  // Debug serial
  Serial.print("LDR: ");
  Serial.print(ldrValue);
  Serial.print(" | Relay1: ");
  Serial.print(relay1On);
  Serial.print(" | PIR: ");
  Serial.print(pirDetected);
  Serial.print(" | Relay2: ");
  Serial.print(relay2Active);
  Serial.print(" | Normal: ");
  Serial.print(dim);
  Serial.print(" | DARK: ");
  Serial.println(dark);


  delay(500);
}