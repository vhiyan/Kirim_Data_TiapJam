#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHT.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "time.h"

// ======== KONFIGURASI BLYNK ============== 
#include "secrets.h"

// ==== KONFIGURASI WIFI ====
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

#include <BlynkSimpleEsp32.h>

// ==== GOOGLE SCRIPT URL ====
String serverName = String(SERVER_URL_STR);

// ==== KONFIGURASI PIN ====
const int SOIL_POWER_PIN = 46;   // pin power sensor tanah
const int SOIL_ADC_PIN   = 3;   // GPIO1 = ADC1_CH0
const int DHTPIN         = 18;   // pin data DHT11
const int DHTTYPE        = DHT11;
const int PUMP_MOTOR_PIN = 20;
const int  SOIL_MOISTURE_THRESHOLD = 50.0;  // Threshold for soil moisture to activate pump

// ==== KALIBRASI SENSOR TANAH =
const int wetValue = 725;   // 100% basah
const int dryValue = 4095;  // 0% kering

// ==== OBJEK SENSOR ====
DHT dht(DHTPIN, DHTTYPE);

// ==== OLED SSD1306 ====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ==== WAKTU (NTP) ====
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7 * 3600;  // GMT+7
const int daylightOffset_sec = 0;

bool firstSend = true;  // kirim sekali di awal
int lastHourSent = -1;  // catat jam terakhir
// bool motor_status = false;

// Tambahkan setelah deklarasi variabel global
unsigned long lastSendTime = 0;  // Waktu terakhir kirim data
const long sendInterval = 1 * 10 * 1000;  // Interval kirim (1/2 jam dalam milidetik)

// ==== FUNGSI SENSOR TANAH ====
int readSoilRaw() {
  digitalWrite(SOIL_POWER_PIN, HIGH);
  delay(40);
  int value = analogRead(SOIL_ADC_PIN);
  digitalWrite(SOIL_POWER_PIN, LOW);
  return value;
}

float soilPercent(int adcValue) {
  float pct = (float)(adcValue - dryValue) * 100.0 / (wetValue - dryValue);
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  return pct;
}

// ==== KIRIM DATA ====
void sendToGoogle(float temperature, float humidity, float soil) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = serverName + "?temp=" + String(temperature, 1) +
                 "&humid=" + String(humidity, 1) +
                 "&soil=" + String(soil, 1);

    Serial.println("Mengirim URL: " + url);
    http.begin(url.c_str());
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      Serial.println("Response: " + payload);
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi terputus, mencoba reconnect...");
    WiFi.begin(ssid, password);
  }
}

// ==== Fungsi ambil jam ====
String getTimeHM() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "00:00";
  }
  char buffer[6];
  strftime(buffer, sizeof(buffer), "%H:%M", &timeinfo);
  return String(buffer);
}

// ==== SETUP ====
void setup() {
  Serial.begin(115200);

  pinMode(SOIL_POWER_PIN, OUTPUT);
  digitalWrite(SOIL_POWER_PIN, LOW);

  dht.begin();

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);
  pinMode(PUMP_MOTOR_PIN, OUTPUT); // Initialise digital pin 4 as an output pin

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 gagal ditemukan!");
    for (;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Monitoring Pot...");
  display.display();
  delay(2000);

  // Koneksi WiFi
  WiFi.begin(ssid, password);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Connecting WiFi...");
  display.display();

  Serial.print("Menghubungkan ke WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Terhubung!");
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("WiFi Connected");
  display.display();  
  delay(1000);

  // Konfigurasi NTP (set sekali)
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Time configured.");

  // Kirim data pertama kali saat device hidup
  int soilADC = readSoilRaw();
  float soilPct = soilPercent(soilADC);
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  
  Blynk.virtualWrite(V2, soilPct);

  lastSendTime = millis(); // Update waktu terakhir kirim
}

bool autoMode = true;  // default mode auto
bool manualPumpState = false;

// Fungsi untuk kontrol pompa
void controlPump(bool state) {
  digitalWrite(PUMP_MOTOR_PIN, state);
  Serial.println(state ? "Pump ON" : "Pump OFF");
}

BLYNK_WRITE(V0) {
  manualPumpState = param.asInt();
  autoMode = false;  // switch to manual mode when V0 is triggered
  controlPump(manualPumpState);
}

// ==== LOOP ====
void loop() {
  Blynk.run();
  // Baca sensor tanah
  int soilADC = readSoilRaw();
  float soilPct = soilPercent(soilADC);

  // Baca DHT
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // ==== Tampilkan di Serial ====
  Serial.print("Soil: "); Serial.print(soilPct, 1);
  Serial.print("% | Air Hum: "); Serial.print(humidity, 1);
  Serial.print("% | Temp: "); Serial.print(temperature, 1);
  Serial.println(" C");

  // ==== Tampilkan di OLED ====
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);



  // Baris pertama: "Monitor Bayam   HH:MM"
  String jamNow = getTimeHM();
  display.print("Monitor Bayam | ");
  display.println(jamNow);

  display.setCursor(0, 16);
  display.print("Suhu : ");
  display.print(temperature, 1);
  display.println(" C");

  display.setCursor(0, 28);
  display.print("Humid: ");
  display.print(humidity, 1);
  display.println(" %");

  display.setCursor(0, 40);
  display.print("Soil : ");
  display.print(soilPct, 1);
  display.println(" %");

  display.setCursor(0, 54);
  if (WiFi.status() == WL_CONNECTED) {
    display.println("WiFi: OK");
  } else {
    display.println("WiFi: ERROR");
  }
  display.display();

  unsigned long currentMillis = millis();
  
  // Kirim data ke Blynk setiap 1/2 jam
  if (currentMillis - lastSendTime >= sendInterval) {
    Blynk.virtualWrite(V2, soilPct);
    lastSendTime = currentMillis;
  }

  // ==== Ambil waktu sekarang ====
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    // Kirim data ke Google Sheet pertama kali atau setiap jam bulat
    if (firstSend || (timeinfo.tm_min == 0 && timeinfo.tm_sec == 0 && timeinfo.tm_hour != lastHourSent)) {
      sendToGoogle(temperature, humidity, soilPct);
      lastHourSent = timeinfo.tm_hour;
      firstSend = false;
      delay(1000); // beri jeda 1 detik supaya tidak dobel
    }
  } else {
    Serial.println("Gagal mendapatkan waktu NTP");
  }

  // Kontrol pompa berdasarkan mode
  if (autoMode) {
    // Mode auto: nyalakan pompa jika soil < 50%
    if (soilPct < SOIL_MOISTURE_THRESHOLD) {
      controlPump(true);
    } else {
      controlPump(false);
    }
  }
  
  delay(1000); // loop cek tiap 1 detik
}
