#include <WiFi.h>
#include "wifi_manager.h"
#include "config.h"

void wifi_setup() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Menyambungkan ke WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi terhubung!");
  Serial.print("Alamat IP: ");
  Serial.println(WiFi.localIP());
}