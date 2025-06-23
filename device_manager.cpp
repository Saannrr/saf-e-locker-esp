// File: device_manager.cpp (Versi Perbaikan)

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
String currentInput = "";
int lastPirState = LOW;
unsigned long pirDebounceTime = 0;
unsigned long unlockTimestamp = 0;

// --- IMPLEMENTASI FUNGSI BARU UNTUK LED RGB ---
// Helper function untuk mengatur warna (asumsi Common Anode: LOW = ON)
void led_set_color(int r, int g, int b) {
  digitalWrite(PIN_RGB_R, r);
  digitalWrite(PIN_RGB_G, g);
  digitalWrite(PIN_RGB_B, b);
}

// Menyalakan LED HIJAU menandakan tersedia
void led_show_available() {
  // R=OFF, G=ON, B=OFF
  led_set_color(HIGH, LOW, HIGH);
  Serial.println("LED: Hijau (Tersedia)");
}

// Menyalakan LED MERAH menandakan terpakai
void led_show_occupied() {
  // R=ON, G=OFF, B=OFF
  led_set_color(LOW, HIGH, HIGH);
  Serial.println("LED: Merah (Terpakai)");
}

// Menyalakan LED MERAH menandakan maintenance
void led_show_maintenance() {
  // Sama seperti occupied
  led_set_color(LOW, HIGH, HIGH);
  Serial.println("LED: Merah (Maintenance)");
}

// Mematikan semua LED
void led_turn_off() {
  led_set_color(HIGH, HIGH, HIGH);
  Serial.println("LED: Mati");
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
  // Cek dulu apakah ada PIN yang aktif. Jika tidak, jangan lakukan apa-apa.
  if (activePin == "") {
    update_oled_display("Loker tidak aktif", "");
    delay(2000);
    currentInput = "";
    update_oled_display("Masukkan PIN:", "");
    return;
  }

  if (currentInput == activePin) {
    update_oled_display("Akses Diterima!");
    unlock_door();
    // Timestamp tidak perlu direset di sini, karena sudah direset di lock_controller
  } else {
    update_oled_display("PIN Salah!", "");
    send_notification("Akses Ditolak", "Percobaan PIN salah dari keypad.");
    delay(2000);
  }

  currentInput = ""; // Selalu reset input setelah pengecekan

  // Update tampilan OLED setelah cek
  if (!get_lock_state()) { // Jika pintu sekarang terbuka
     update_oled_display("Pintu Terbuka", "");
  } else {
     update_oled_display("Masukkan PIN:", "");
  }
}

// --- Fungsi Setup Perangkat ---
void device_setup() {
  // Setup Pin
  pinMode(PIN_PIR, INPUT_PULLDOWN);
  // Atur semua pin RGB sebagai OUTPUT
  pinMode(PIN_RGB_R, OUTPUT);
  pinMode(PIN_RGB_G, OUTPUT);
  pinMode(PIN_RGB_B, OUTPUT);
  
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

  // Inisialisasi Preferences (untuk menyimpan password) (disable, dikarenakan mau pakai passkey dinamis di firebase)
  // preferences.begin("saf-e-locker", false);
  // passwordLock = preferences.getString("password", DEFAULT_PASSWORD);

  // Set status LED awal
  // led_show_locked(); 
  // delay(2000);
  // update_oled_display("Masukkan PIN:", "");

  led_show_available(); 
  delay(2000);
  update_oled_display("Silakan Pilih Loker", "");
}

// --- Fungsi Loop Perangkat ---
void device_loop() {
  // 1. Logika utama untuk membaca keypad
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
  // --- PERUBAHAN LOGIKA ---
  // Cek jika pintu dalam keadaan TERBUKA (!get_lock_state() -> !false -> true) dan timer sudah lewat
  if (!get_lock_state() && (millis() - unlockTimestamp >= AUTO_LOCK_DELAY_MS)) {
    update_oled_display("Mengunci Otomatis...");
    lock_door();
    delay(1000);
    update_oled_display("Masukkan PIN:", "");
  }

  // 3. Logika Sensor PIR (TIDAK LAGI MENGATUR LED)
  if (millis() - pirDebounceTime > 2000) {
    int pirVal = digitalRead(PIN_PIR);
    if (pirVal == HIGH && lastPirState == LOW) {
      if (get_lock_state()) { // Cek jika pintu sedang TERKUNCI
        Serial.println("Gerakan terdeteksi!");
        send_notification("Gerakan Terdeteksi", "Ada gerakan di dekat pintu Anda!");
      }
      pirDebounceTime = millis();
    }
    lastPirState = pirVal;
  }
}