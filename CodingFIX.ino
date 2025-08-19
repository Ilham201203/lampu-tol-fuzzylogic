#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Pin dan tipe sensor
#define DHTPIN 23
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define PIR_PIN 32
#define LDR_PIN 34
#define RELAY1_PIN 19
#define RELAY2_PIN 18
#define BUZZER_PIN 26

LiquidCrystal_I2C lcd(0x27, 16, 2);

int lastDisplayState = -1;  // Menyimpan tampilan sebelumnya
unsigned long lastMessageSwitch = 0;
int displayState = 0;  // 0: lingkungan, 1: pesan, 2: jumlah kendaraan

// Fungsi keanggotaan fuzzy trapezoidal
float fuzzyTrapezoidal(float x, float a, float b, float c, float d) {
  if (x <= a || x >= d) return 0.0;
  else if (x >= b && x <= c) return 1.0;
  else if (x > a && x < b) return (x - a) / (b - a);
  else return (d - x) / (d - c);
}

// Keanggotaan kelembaban
float dryMembership(float h)     { return fuzzyTrapezoidal(h, 0, 0, 30, 45); }
float normalMembership(float h)  { return fuzzyTrapezoidal(h, 40, 45, 55, 60); }
float foggyMembership(float h)   { return fuzzyTrapezoidal(h, 55, 70, 100, 100); }

// Keanggotaan cahaya
float darkMembership(float l)    { return fuzzyTrapezoidal(l, 0, 0, 100, 300); }
float dimMembership(float l)     { return fuzzyTrapezoidal(l, 400, 600, 800, 1000); }
float brightMembership(float l)  { return fuzzyTrapezoidal(l, 800, 1100, 1300, 1500); }

void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(PIR_PIN, INPUT);
  pinMode(LDR_PIN, INPUT);
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(RELAY1_PIN, HIGH); // Mati (karena aktif LOW)
  digitalWrite(RELAY2_PIN, HIGH);
  digitalWrite(BUZZER_PIN, LOW);

    lcd.init();        // Inisialisasi LCD
  lcd.backlight();   // Nyalakan lampu latar LCD

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
  // Baca nilai sensor
  int ldrValue = analogRead(LDR_PIN);
  if (isnan(ldrValue)) ldrValue = 600;

  float humidityValue = dht.readHumidity();
  if (isnan(humidityValue)) humidityValue = 50.0;

  float temperatureValue = dht.readTemperature();
  if (isnan(temperatureValue)) temperatureValue = 0.0;

  bool pirDetected = digitalRead(PIR_PIN);

  // Hitung nilai fuzzy
  float dark = darkMembership(ldrValue);
  float dim = dimMembership(ldrValue);
  float bright = brightMembership(ldrValue);

  float dry = dryMembership(humidityValue);
  float normal = normalMembership(humidityValue);
  float foggy = foggyMembership(humidityValue);

  // Aturan fuzzy
  bool relay1On = (foggy > 0.1) || (dark < 0.3); // Relay1 ON jika kabut atau gelap
  digitalWrite(RELAY1_PIN, relay1On ? HIGH : LOW); // Aktif LOW

  bool relay2On = relay1On && pirDetected;
  digitalWrite(RELAY2_PIN, relay2On ? HIGH : LOW);

  // Buzzer sesuai relay
  if (relay2On) {
    tone(BUZZER_PIN, 1000, 200); // bunyi 200ms
    delay(250);
    tone(BUZZER_PIN, 1000, 200); // bunyi lagi
  } else if (relay1On) {
    tone(BUZZER_PIN, 1000, 200); // bunyi sekali
  } else {
    noTone(BUZZER_PIN);
  }

  if (millis() - lastMessageSwitch >= 5000) {
  displayState = (displayState + 1) % 3;  // 0 → 1 → 2 → 0
  lastMessageSwitch = millis();
}
  String blynkMessage = (foggy > 0.7) ? "Kabut Tebal!" : "HATI-HATI DI JALAN";


// Hanya clear dan update jika state berubah
if (displayState != lastDisplayState) {
  lcd.clear();

  if (displayState == 0) {
  lcd.setCursor(0, 0);
  lcd.print("                "); // clear
  lcd.setCursor(0, 0);
  lcd.print(blynkMessage.substring(0, 16)); // Baris pertama

  lcd.setCursor(0, 1);
  if (blynkMessage.length() > 16) {
    lcd.print(blynkMessage.substring(16, 32)); // Baris kedua jika ada
  } else {
    lcd.print("                "); // Kosongkan
  }
} 
  else if (displayState == 1) {

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

Serial.print("Relay1: ");
Serial.print(relay1On);
Serial.print(" | PIR: ");
Serial.print(pirDetected);
Serial.print(" | Relay2: ");
Serial.println(relay2On);
Serial.print(" | Dark: ");
Serial.println(dark);
delay(500);
}
}
