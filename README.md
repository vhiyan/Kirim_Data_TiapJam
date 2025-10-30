# Kirim_Data_TiapJam (ESP32)

Project: Monitoring kelembaban tanah, suhu, dan kelembaban udara. Mengirim data ke Google Sheets dan Blynk, serta kontrol pompa (manual via Blynk atau otomatis berdasarkan threshold soil).

## Fitur utama
- Pembacaan sensor tanah (ADC) dan DHT11
- Tampil di OLED SSD1306
- Kirim data ke Google Sheets (via Google Script)
- Kirim data ke Blynk (virtual pin V2)
- Kontrol pompa: manual lewat V0 (Blynk) atau otomatis saat soil < threshold

## Dependensi (install via Library Manager)
- Adafruit GFX
- Adafruit SSD1306
- DHT sensor library (by Adafruit or equivalent)
- Blynk (BlynkSimpleEsp32)
- Arduino core for ESP32 (board package)
- (HTTPClient, WiFi, Wire sudah termasuk di core ESP32)

## Wiring (sesuai kode)
- SOIL_POWER_PIN = GPIO46 (power sensor tanah)
- SOIL_ADC_PIN   = ADC1_CH0 (di kode: 3)
- DHT data       = GPIO18
- PUMP_MOTOR_PIN = GPIO20 (digital output)
- OLED I2C      = SDA/SCL sesuai board (default Wire)

## Cara pakai / setup kredensial
1. Buka file `secrets_example.h` yang ada di folder proyek.
2. Copy dan rename:
   - Windows Explorer: klik kanan -> Copy, lalu Paste -> ganti nama menjadi `secrets.h`
   - Atau di terminal:
     - copy secrets_example.h secrets.h
3. Edit `secrets.h` dan isi sesuai kredensial Anda:
   - WIFI_SSID
   - WIFI_PASSWORD
   - BLYNK_AUTH_TOKEN
   - (opsional) SERVER_URL_STR jika menggunakan URL Google Script Anda sendiri

Contoh (jangan commit file ini ke repositori):
```cpp
// secrets.h (lokal, private)
#define WIFI_SSID "Nama_WiFi"
#define WIFI_PASSWORD "Password_WiFi"
#define BLYNK_AUTH_TOKEN "Token_Blynk_Anda"
#define SERVER_URL_STR "https://script.google.com/...."
```


## Konfigurasi Blynk (singkat)
- Buat project baru di Blynk, tambahkan device ESP32 dan catat auth token.
- Tambahkan widget:
  - Switch pada V0 -> kontrol pompa manual
  - Value display / Gauge pada V2 -> menampilkan soil %
- Saat switch V0 diubah, kode akan berpindah ke mode manual. Untuk kembali ke mode auto, reboot device (atau Anda bisa tambahkan mekanisme toggle mode di aplikasi jika perlu).

## Pengaturan interval & threshold
- `sendInterval` dan `SOIL_MOISTURE_THRESHOLD` diubah di `.ino` untuk menyesuaikan frekuensi kirim dan batas aktif pompa.
- Periksa tipe dan nilai konstanta saat mengubah.

## Upload ke ESP32
1. Pilih board ESP32 di Arduino IDE dan pilih port COM.
2. Install library yang diperlukan.
3. Pastikan `secrets.h` sudah ada dan terisi.
4. Upload sketch.

## Troubleshooting singkat
- Jika data Blynk tidak muncul: periksa koneksi WiFi, token Blynk, dan Virtual Pin benar.
- Jika pompa tidak menyala: periksa wiring, gunakan transistor/driver jika pompa memerlukan arus lebih besar, cek PUMP_MOTOR_PIN.
- Jika ADC reading aneh: pastikan referensi dan power sensor benar, periksa nilai wet/dry calibration.

Kode sumber dan file contoh kredensial ada di folder proyek. Simpan `secrets.h` secara lokal dan jangan bagikan kredensial publik.
