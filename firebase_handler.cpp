#include <FirebaseESP32.h>
#include "firebase_handler.h"
#include "config.h"
#include "lock_controller.h" // Perlu untuk mengontrol kunci

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long lastFirebaseCheck = 0;

void firebase_setup() {
  config.database_url = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void firebase_loop() {
  if (millis() - lastFirebaseCheck >= 2000) { // Cek Firebase setiap 2 detik
      lastFirebaseCheck = millis();
    if (Firebase.ready()) {
      if (Firebase.getBool(fbdo, "/saf-e-locker/isLocked")) { // Path disesuaikan
        // Cek apakah nilai dari Firebase berbeda dengan state lokal
        if (fbdo.boolData() != get_lock_state()) {
            Serial.println("Menerima perintah dari Firebase!");
            // Logika BARU yang sudah benar dan intuitif:
            // Jika isLocked dari Firebase itu true -> panggil lock_door()
            // Jika isLocked dari Firebase itu false -> panggil unlock_door()
            if (fbdo.boolData()) {
                lock_door();
            } else {
                unlock_door();
          }
        }
      }
    }
  }
}

void update_firebase_lock_state(bool isUnlocked) {
  if (Firebase.ready()) {
    Firebase.setBool(fbdo, "/saf-e-locker/isLocked", isUnlocked);
  }
}

void send_notification(const String& title, const String& body) {
  if (Firebase.ready()) {
    FirebaseJson json;
    json.add("title", title);
    json.add("body", body);
    json.set("timestamp/.sv", "timestamp");
    Firebase.pushAsync(fbdo, "/notifications", json);
    Serial.println("Notifikasi dikirim: " + title);
  }
}