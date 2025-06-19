// File: lock_controller.cpp (Versi Perbaikan)

#include <ESP32Servo.h>
#include "lock_controller.h"
#include "config.h"
#include "firebase_handler.h"
#include "device_manager.h" // Pastikan ini di-include

Servo motorKunci;
bool lockState = true; 

void lock_setup() {
  motorKunci.attach(PIN_SERVO);
  lock_door(); 
}

void lock_door() {
  motorKunci.write(SERVO_LOCKED_POS);
  if (lockState != true) {
    lockState = true;
    Serial.println("SYSTEM: Pintu Terkunci.");
    update_firebase_lock_state(lockState);
    
    // --- TAMBAHKAN PEMICU LED DI SINI ---
    led_show_occupied();
  }
}

void unlock_door() {
  motorKunci.write(SERVO_UNLOCKED_POS);
  if (lockState != false) {
    lockState = false;
    Serial.println("SYSTEM: Pintu Terbuka.");
    update_firebase_lock_state(lockState);
    unlockTimestamp = millis();
    
    // --- TAMBAHKAN PEMICU LED DI SINI ---
    // Setelah terbuka, loker menjadi tersedia lagi
    led_show_available();
  }
}

bool get_lock_state() {
  return lockState;
}