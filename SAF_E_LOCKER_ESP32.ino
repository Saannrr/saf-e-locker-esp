#include "config.h"
#include "device_manager.h"
#include "lock_controller.h"
#include "wifi_manager.h"
#include "firebase_handler.h"

void setup() {
  Serial.begin(115200);
  Serial.println("\n\nBooting SAF-E LOCKER...");

  // Inisialisasi modul dalam urutan yang logis
  device_setup();
  lock_setup();
  wifi_setup();
  firebase_setup();
  
  Serial.println("Sistem Siap Digunakan.");
}

void loop() {
  // Panggil fungsi loop dari setiap modul yang memerlukannya
  device_loop();
  firebase_loop();
}