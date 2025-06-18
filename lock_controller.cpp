#include <ESP32Servo.h>
#include "lock_controller.h"
#include "config.h"
#include "firebase_handler.h"
#include "device_manager.h" // Asumsi led_show_... ada di sini

Servo motorKunci;
// Inisialisasi state awal. Asumsikan loker mulai dalam keadaan terkunci.
bool lockState = true; // <--- PERUBAHAN DI SINI (true = terkunci)

void lock_setup() {
  motorKunci.attach(PIN_SERVO);
  lock_door(); 
}

void lock_door() {
  motorKunci.write(SERVO_LOCKED_POS);
  // Hanya eksekusi jika state sebelumnya tidak terkunci (false)
  if (lockState != true) {  // <--- PERUBAHAN DI SINI
    lockState = true;       // <--- PERUBAHAN DI SINI (Set state menjadi terkunci)
    Serial.println("SYSTEM: Pintu Terkunci.");
    update_firebase_lock_state(lockState);
    // led_show_locked(); // Aktifkan jika sudah ada fungsinya
  }
}

void unlock_door() {
  motorKunci.write(SERVO_UNLOCKED_POS);
  // Hanya eksekusi jika state sebelumnya tidak terbuka (true)
  if (lockState != false) { // <--- PERUBAHAN DI SINI
    lockState = false;      // <--- PERUBAHAN DI SINI (Set state menjadi terbuka)
    Serial.println("SYSTEM: Pintu Terbuka.");
    update_firebase_lock_state(lockState);

    // --- TAMBAHKAN BARIS INI ---
    // Reset timer auto-lock setiap kali pintu dibuka
    unlockTimestamp = millis();
    
    // led_show_unlocked(); // Aktifkan jika sudah ada fungsinya
  }
}

bool get_lock_state() {
  return lockState;
}