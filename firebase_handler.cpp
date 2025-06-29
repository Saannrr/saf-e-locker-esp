// File: firebase_handler.cpp (Revisi untuk FirebaseESP32 v4.x Real-time Listener)

#include <Firebase_ESP_Client.h>
#include <time.h>
#include <HTTPClient.h>
#include "firebase_handler.h"
#include "config.h"
#include "lock_controller.h"
#include "device_manager.h"

// Objek Firebase global
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

String activePin = "";  // Untuk menyimpan PIN aktif secara global
String lockerStatus = "available"; // Nilai default
unsigned long lastUnlockRequestTimestamp = 0;

// --- Deklarasi Callback Stream ---
void firestore_data_callback(FirebaseStream data);
void stream_timeout_callback(bool timeout);

unsigned long lastFirebaseCheck = 0;
const char* REPORT_MOTION_URL = "https://us-central1-saf-e-locker.cloudfunctions.net/reportSuspiciousMotion";

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

      FirebaseJson payloadJson;
      payloadJson.setJsonData(fbdo.payload());
      FirebaseJsonData jsonData;

      // --- KODE BARU (Sintaks yang Benar untuk FirebaseESP32.h) ---
      // Panggil getDocument melalui Firebase.Firestore
      if (Firebase.Firestore.getDocument(&fbdo, PROJECT_ID, "", documentPath.c_str())) {
        Serial.printf("\n[Polling] Mendapat data loker: %s\n", fbdo.payload().c_str());

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

        if (payloadJson.get(jsonData, "fields/status/stringValue")) {
          lockerStatus = jsonData.to<String>();
        } else {
            lockerStatus = "unknown"; // Jika field status tidak ada
          }
      } else {
          // Tambahkan ini untuk melihat error jika gagal mengambil data
          Serial.printf("Gagal baca Firestore: %s\n", fbdo.errorReason().c_str());
      }

      // ...
      // --- Parsing isLocked ---
      if (payloadJson.get(jsonData, "fields/isLocked/booleanValue")) {
          bool firebaseLockState = jsonData.to<bool>();
          // Jika status di Firebase (false) berbeda dengan status fisik (true)...
          if (firebaseLockState != get_lock_state()) {
              Serial.printf("Perintah kunci dari server: %s\n", firebaseLockState ? "LOCK" : "UNLOCK");
              // ...maka eksekusi perintahnya.
              if (firebaseLockState) lock_door(); else unlock_door();
          }
        }
      // ...
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

// --- PERUBAHAN 3: Modifikasi Total Fungsi send_notification ---
// Fungsi ini sekarang tidak lagi menulis ke Firestore.
// Tugasnya adalah memanggil Cloud Function yang akan melakukan semua pekerjaan berat.
void send_notification(const String& title, const String& body) {
  // Kita tidak lagi butuh parameter title dan body, karena Cloud Function
  // yang akan menentukan isi notifikasinya. Tapi kita biarkan agar tidak merusak
  // panggilan fungsi dari tempat lain di kode Anda.
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Gagal kirim notifikasi: WiFi tidak terhubung.");
    return;
  }

  HTTPClient http;

  // Mulai koneksi ke endpoint Cloud Function
  http.begin(REPORT_MOTION_URL);
  
  // Set header bahwa kita mengirim data JSON
  http.addHeader("Content-Type", "application/json");

  // Buat payload JSON yang sederhana, hanya berisi ID loker dan ESP32.
  // Ini jauh lebih efisien daripada membuat JSON Firestore yang kompleks.
  String payload = "{\"lockerId\":\"" + String(LOCKER_ID) + "\",\"esp32_id\":\"" + String(ESP32_ID) + "\"}";

  Serial.println("Mengirim peringatan gerakan ke Cloud Function...");
  Serial.print("Payload: ");
  Serial.println(payload);

  // Kirim request HTTP POST
  int httpResponseCode = http.POST(payload);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    Serial.print("Response dari Cloud Function: ");
    Serial.println(response);
  } else {
    Serial.print("Error saat mengirim POST ke Cloud Function: ");
    Serial.println(httpResponseCode);
  }

  // Tutup koneksi
  http.end();
}

String get_locker_status() {
  return lockerStatus;
}