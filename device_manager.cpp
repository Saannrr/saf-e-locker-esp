#include <Keypad.h>
#include <Keypad_I2C.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>
#include "device_manager.h"
#include "config.h"
#include "lock_controller.h"
#include "firebase_handler.h"

// Inisialisasi Objek Perangkat Keras
Adafruit_SSD1306 display(128, 64, &Wire, -1);
char keys[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1','2','3','A'}, {'4','5','6','B'}, {'7','8','9','C'}, {'*','0','#','D'}
};
byte rowPins[KEYPAD_ROWS] = {0, 1, 2, 3}; // Pin di PCF8574
byte colPins[KEYPAD_COLS] = {4, 5, 6, 7}; // Pin di PCF8574
Keypad_I2C customKeypad(makeKeymap(keys), rowPins, colPins, KEYPAD_ROWS, KEYPAD_COLS, PCF8574_I2C_ADDR, PCF8574);
Preferences preferences;

// Variabel Internal
String passwordLock;
String currentInput = "";
int lastPirState = LOW;
unsigned long pirDebounceTime = 0;
unsigned long unlockTimestamp = 0;

// --- IMPLEMENTASI FUNGSI BARU UNTUK LED ---
// Fungsi dasar untuk set warna. Ingat, LOW = NYALA, HIGH = MATI
void led_set_color(int red, int green, int blue) {
  digitalWrite(PIN_RGB_R, red);
  digitalWrite(PIN_RGB_G, green);
  digitalWrite(PIN_RGB_B, blue);
}

void led_show_locked() {
  // BIRU = R(mati), G(mati), B(nyala)
  led_set_color(HIGH, HIGH, LOW);
  Serial.println("LED: Biru (Terkunci)");
}

void led_show_unlocked() {
  // HIJAU = R(mati), G(nyala), B(mati)
  led_set_color(HIGH, LOW, HIGH);
  Serial.println("LED: Hijau (Terbuka)");
}

void led_show_motion() {
  // MERAH = R(nyala), G(mati), B(mati)
  led_set_color(LOW, HIGH, HIGH);
  Serial.println("LED: Merah (Gerakan Terdeteksi)");
}
// --- AKHIR FUNGSI BARU ---

void update_oled_display(const String& line1, const String& line2 = "", int size1 = 1, int size2 = 2) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(size1);
    display.setCursor(0, 0);
    display.println(line1);
    display.setTextSize(size2);
    display.setCursor(0, 16);
    display.print(line2);
    display.display();
}

void check_password() {
  if (currentInput == passwordLock) {
    update_oled_display("Akses Diterima!");
    unlock_door();
    send_notification("Pintu Dibuka", "Pintu dibuka dengan keypad.");
    unlockTimestamp = millis(); // Mulai timer untuk kunci otomatis
  } else {
    update_oled_display("PIN Salah!", "");
    send_notification("Akses Ditolak", "Percobaan PIN salah.");
    delay(2000);
  }
  currentInput = "";
  // Tampilan kembali normal setelah beberapa saat
  if (!get_lock_state()) { // jika masih terkunci
     update_oled_display("Masukkan PIN:", "");
  }
}

// --- UBAH FUNGSI device_setup() ---
void device_setup() {
  // Setup Pin
  pinMode(PIN_PIR, INPUT);
  // Atur semua pin RGB sebagai OUTPUT
  pinMode(PIN_RGB_R, OUTPUT);
  pinMode(PIN_RGB_G, OUTPUT);
  pinMode(PIN_RGB_B, OUTPUT);
  
  // ... (sisa kode setup lainnya tetap sama) ...
  // Di akhir setup, panggil fungsi untuk set warna LED awal
  led_show_locked(); 
}

// --- UBAH FUNGSI device_loop() ---
void device_loop() {
  // ... (logika keypad tetap sama) ...

  // Logika Auto-Lock
  if (get_lock_state() && (millis() - unlockTimestamp >= AUTO_LOCK_DELAY_MS)) {
    update_oled_display("Mengunci Otomatis...");
    lock_door(); // lock_door() akan otomatis memanggil led_show_locked()
    delay(1000);
    update_oled_display("Masukkan PIN:", "");
  }

  // Logika Sensor PIR (dimodifikasi)
  if (millis() - pirDebounceTime > 2000) { // Cek setiap 2 detik
    int pirVal = digitalRead(PIN_PIR);
    if (pirVal != lastPirState) {
      if (pirVal == HIGH) {
        // Jika ada gerakan, paksa warna jadi MERAH
        led_show_motion(); 
        Serial.println("Gerakan terdeteksi!");
        send_notification("Gerakan Terdeteksi", "Ada gerakan di dekat pintu.");
      } else {
        // Jika tidak ada gerakan, kembalikan warna LED ke status kunci
        get_lock_state() ? led_show_unlocked() : led_show_locked();
      }
      lastPirState = pirVal;
    }
    pirDebounceTime = millis();
  }
}