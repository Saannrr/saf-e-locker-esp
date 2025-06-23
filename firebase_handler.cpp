// File: firebase_handler.cpp (Revisi untuk FirebaseESP32 v4.x Real-time Listener)

#include <Firebase_ESP_Client.h>
#include <time.h>
#include "firebase_handler.h"
#include "config.h"
#include "lock_controller.h"
#include "device_manager.h"

// Objek Firebase global
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

String activePin = "";  // Untuk menyimpan PIN aktif secara global

// --- Deklarasi Callback Stream ---
void firestore_data_callback(FirebaseStream data);
void stream_timeout_callback(bool timeout);

unsigned long lastFirebaseCheck = 0;

void firebase_setup() {
  config.api_key = API_KEY;

  auth.user.email = CLIENT_EMAIL;
  config.service_account.data.project_id = PROJECT_ID;
  config.service_account.data.client_email = CLIENT_EMAIL;
  config.service_account.data.private_key = PRIVATE_KEY;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void firebase_loop() {
  if (WiFi.status() == WL_CONNECTED && millis() - lastFirebaseCheck >= 2000) {
    lastFirebaseCheck = millis();

    if (Firebase.ready()) {
      String documentPath = "lockers/" + String(LOCKER_ID);

      // --- KODE BARU (Sintaks yang Benar untuk FirebaseESP32.h) ---
      // Panggil getDocument melalui Firebase.Firestore
      if (Firebase.Firestore.getDocument(&fbdo, PROJECT_ID, "", documentPath.c_str())) {
        Serial.printf("\n[Polling] Mendapat data loker: %s\n", fbdo.payload().c_str());

        FirebaseJson payloadJson;
        payloadJson.setJsonData(fbdo.payload());
        FirebaseJsonData jsonData;

        // --- Parsing isLocked ---
        if (payloadJson.get(jsonData, "fields/isLocked/booleanValue")) {  // Gunakan slash
          bool firebaseLockState = jsonData.to<bool>();
          if (firebaseLockState != get_lock_state()) {
            Serial.printf("Perintah kunci: %s\n", firebaseLockState ? "LOCK" : "UNLOCK");
            if (firebaseLockState) lock_door(); else unlock_door();
          }
        } else {
          Serial.println("Field 'isLocked' tidak ditemukan/tidak valid");
        }

        // --- Parsing active_pin ---
        if (payloadJson.get(jsonData, "fields/active_pin/stringValue")) {  // Gunakan slash
          String newPin = jsonData.to<String>();
          if (activePin != newPin) {
            activePin = newPin;
            Serial.printf("PIN baru: %s\n", activePin.c_str());
          }
        } else {
          Serial.println("Field 'active_pin' tidak ditemukan");
          activePin = "";  // Reset jika field tidak ada
        }
      } else {
          // Tambahkan ini untuk melihat error jika gagal mengambil data
          Serial.printf("Gagal baca Firestore: %s\n", fbdo.errorReason().c_str());
      }
    }
  }
}

void update_firebase_lock_state(bool isLocked) {
  if (!Firebase.ready()) return;

  String documentPath = "lockers/" + String(LOCKER_ID);
  String content =
    "{\"fields\":{\"isLocked\":{\"booleanValue\":" + String(isLocked ? "true" : "false") + "}}}";

  if (Firebase.Firestore.patchDocument(&fbdo, PROJECT_ID, "", documentPath.c_str(), content.c_str(), "isLocked")) {
    Serial.println("Berhasil update state isLocked ke Firestore.");
  } else {
    Serial.printf("Gagal update state: %s\n", fbdo.errorReason().c_str());
  }
}

void send_notification(const String& title, const String& body) {
  if (!Firebase.ready()) return;

  // Sinkronisasi waktu
  configTime(0, 0, "pool.ntp.org");
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Gagal sinkronisasi NTP");
    return;
  }

  char timeStr[30];
  strftime(timeStr, sizeof(timeStr), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);

  String content =
    "{\"fields\":{"
    "\"title\":{\"stringValue\":\"" + title + "\"},"
    "\"body\":{\"stringValue\":\"" + body + "\"},"
    "\"locker_id\":{\"stringValue\":\"" + String(LOCKER_ID) + "\"},"
    "\"timestamp\":{\"timestampValue\":\"" + String(timeStr) + "\"}"
    "}}";

  if (Firebase.Firestore.createDocument(&fbdo, PROJECT_ID, "", "notifications", content.c_str())) {
    Serial.println("Notifikasi berhasil dikirim.");
  } else {
    Serial.printf("Gagal kirim notifikasi: %s\n", fbdo.errorReason().c_str());
  }
}
