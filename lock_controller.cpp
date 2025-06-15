#include <ESP32Servo.h>
#include "lock_controller.h"
#include "config.h"
#include "firebase_handler.h" // Perlu untuk update status ke Firebase
#include "device_manager.h"

Servo motorKunci;
bool lockState = false;

void lock_setup() {
  motorKunci.attach(PIN_SERVO);
  lock_door(); 
}

void lock_door() {
  motorKunci.write(SERVO_LOCKED_POS);
  if (lockState != false) { 
    lockState = false;
    Serial.println("SYSTEM: Pintu Terkunci.");
    update_firebase_lock_state(lockState);
    led_show_locked(); // <--- TAMBAHKAN INI
  }
}

void unlock_door() {
  motorKunci.write(SERVO_UNLOCKED_POS);
  if (lockState != true) {
    lockState = true;
    Serial.println("SYSTEM: Pintu Terbuka.");
    update_firebase_lock_state(lockState);
    led_show_unlocked(); // <--- TAMBAHKAN INI
  }
}

bool get_lock_state() {
  return lockState;
}