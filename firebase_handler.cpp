// File: firebase_handler.cpp (Final untuk library FirebaseESP32.h v4.x)

#include <Firebase_ESP_Client.h>
#include "firebase_handler.h"
#include "config.h"
#include "lock_controller.h"

// Definisikan objek Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long lastFirebaseCheck = 0;

void firebase_setup() {
  // --- Konfigurasi untuk koneksi ---
  config.api_key = API_KEY;

  // --- Konfigurasi untuk otentikasi via Service Account ---
  auth.user.email = CLIENT_EMAIL;
  config.service_account.data.private_key = PRIVATE_KEY;
  config.service_account.data.client_email = CLIENT_EMAIL;
  config.service_account.data.project_id = PROJECT_ID;

  // Inisialisasi Firebase dengan konfigurasi baru
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
        Serial.printf("Mendapat data: %s\n", fbdo.payload().c_str());

        FirebaseJson payloadJson;
        payloadJson.setJsonData(fbdo.payload());
        
        FirebaseJsonData jsonData;
        payloadJson.get(jsonData, "fields/isLocked/booleanValue");

        if (jsonData.success) {
          bool firebaseLockState = jsonData.boolValue;
          if (firebaseLockState != get_lock_state()) {
            Serial.println("Menerima perintah dari Firebase!");
            if (firebaseLockState) {
              lock_door();
            } else {
              unlock_door();
            }
          }
        }
      } else {
          // Tambahkan ini untuk melihat error jika gagal mengambil data
          Serial.println(fbdo.errorReason());
      }
    }
  }
}

void update_firebase_lock_state(bool isLocked) {
  if (Firebase.ready()) {
    String documentPath = "lockers/" + String(LOCKER_ID);
    String content = "{\"fields\":{\"isLocked\":{\"booleanValue\":" + String(isLocked ? "true" : "false") + "}}}";

    // --- KODE BARU (Sintaks yang Benar untuk FirebaseESP32.h) ---
    // Panggil patchDocument melalui Firebase.Firestore
    if (Firebase.Firestore.patchDocument(&fbdo, PROJECT_ID, "", documentPath.c_str(), content.c_str(), "isLocked")) {
      Serial.printf("Berhasil update state di Firestore: %s\n", fbdo.payload().c_str());
    } else {
      Serial.printf("Gagal update state di Firestore: %s\n", fbdo.errorReason().c_str());
    }
  }
}

void send_notification(const String& title, const String& body) {
  if (Firebase.ready()) {
    String collectionPath = "notifications";
    String content = "{\"fields\":{\"title\":{\"stringValue\":\"" + title + "\"},\"body\":{\"stringValue\":\"" + body + "\"}}}";

    // --- KODE BARU (Sintaks yang Benar untuk FirebaseESP32.h) ---
    // Panggil createDocument melalui Firebase.Firestore
    if (Firebase.Firestore.createDocument(&fbdo, PROJECT_ID, "", collectionPath.c_str(), content.c_str())) {
      Serial.println("Notifikasi berhasil dikirim ke Firestore.");
    } else {
      Serial.println(fbdo.errorReason());
    }
  }
}