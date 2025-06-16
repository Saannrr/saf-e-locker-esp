// File: device_manager.cpp

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

// Peta tombol untuk keypad 4x4
char keys[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

// Pin koneksi keypad di I/O Expander PCF8574
byte rowPins[KEYPAD_ROWS] = {0, 1, 2, 3};
byte colPins[KEYPAD_COLS] = {4, 5, 6, 7};

Keypad_I2C customKeypad(makeKeymap(keys), rowPins, colPins, KEYPAD_ROWS, KEYPAD_COLS, PCF8574_I2C_ADDR, PCF8574);
Preferences preferences;

// Variabel Internal
String passwordLock;
String currentInput = "";
int lastPirState = LOW;
unsigned long pirDebounceTime = 0;
unsigned long unlockTimestamp = 0;

// --- Implementasi Fungsi LED Satu Warna ---
void led_show_locked() {
  digitalWrite(PIN_LED, LOW); // LED MATI saat terkunci
  Serial.println("LED: Mati (Terkunci)");
}

void led_show_unlocked() {
  digitalWrite(PIN_LED, HIGH); // LED NYALA saat terbuka
  Serial.println("LED: Nyala (Terbuka)");
}

void led_show_motion() {
  Serial.println("LED: Berkedip (Gerakan Terdeteksi)");
  for(int i=0; i<3; i++) {
    digitalWrite(PIN_LED, HIGH);
    delay(200);
    digitalWrite(PIN_LED, LOW);
    delay(200);
  }
}

// --- Fungsi untuk menampilkan teks di OLED ---
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

// --- Fungsi untuk memeriksa password dari keypad ---
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
  currentInput = ""; // Reset input setelah pengecekan
  
  // Setelah beberapa saat, kembalikan tampilan ke state normal
  // Cek dulu apakah pintu akhirnya terbuka atau masih terkunci
  if (get_lock_state()) {
      update_oled_display("Pintu Terbuka", "Otomatis terkunci...");
  } else {
      update_oled_display("Masukkan PIN:", "");
  }
}

// --- Fungsi Setup Perangkat ---
void device_setup() {
  // Setup Pin
  pinMode(PIN_PIR, INPUT_PULLDOWN);
  pinMode(PIN_LED, OUTPUT);
  
  // Inisialisasi Wire (I2C)
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  
  // Inisialisasi OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
    Serial.println(F("Alokasi SSD1306 gagal"));
    for(;;);
  }
  update_oled_display("SAF-E LOCKER", "Booting...", 2, 1);

  // Inisialisasi Keypad
  customKeypad.begin();

  // Inisialisasi Preferences (untuk menyimpan password)
  preferences.begin("saf-e-locker", false);
  passwordLock = preferences.getString("password", DEFAULT_PASSWORD);

  // Set status LED awal
  led_show_locked(); 
  delay(2000);
  update_oled_display("Masukkan PIN:", "");
}

// --- Fungsi Loop Perangkat ---
void device_loop() {
  // 1. Logika utama untuk membaca keypad (TELAH DIPULIHKAN)
  char customKey = customKeypad.getKey();
  if (customKey) {
    if (customKey >= '0' && customKey <= '9') {
      currentInput += customKey;
      String maskedInput = "";
      for (int i = 0; i < currentInput.length(); i++) {
        maskedInput += "*";
      }
      update_oled_display("Masukkan PIN:", maskedInput);
    } else if (customKey == '#') { // Tombol '#' untuk konfirmasi PIN
      update_oled_display("Memeriksa PIN...");
      check_password();
    } else if (customKey == '*') { // Tombol '*' untuk mereset input
      currentInput = "";
      update_oled_display("Input Direset", "");
      delay(1000);
      update_oled_display("Masukkan PIN:", "");
    }
  }

  // 2. Logika Auto-Lock
  if (get_lock_state() && (millis() - unlockTimestamp >= AUTO_LOCK_DELAY_MS)) {
    update_oled_display("Mengunci Otomatis...");
    lock_door();
    delay(1000);
    update_oled_display("Masukkan PIN:", "");
  }

  // 3. Logika Sensor PIR
  if (millis() - pirDebounceTime > 2000) {
    int pirVal = digitalRead(PIN_PIR);
    if (pirVal == HIGH && lastPirState == LOW) { // Hanya picu saat perubahan dari LOW ke HIGH
        if (!get_lock_state()) { // Hanya jika pintu sedang terkunci
          led_show_motion();
          Serial.println("Gerakan terdeteksi!");
          send_notification("Gerakan Terdeteksi", "Ada gerakan di dekat pintu.");
          update_oled_display("Gerakan Terdeteksi!", "");
          delay(2000); // Tampilkan pesan selama 2 detik
          update_oled_display("Masukkan PIN:", currentInput); // Kembali ke tampilan PIN
        }
        pirDebounceTime = millis();
    }
    lastPirState = pirVal;
  }
}