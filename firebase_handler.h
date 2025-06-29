#ifndef FIREBASE_HANDLER_H
#define FIREBASE_HANDLER_H

#include <Arduino.h> // Diperlukan untuk tipe data 'String'

extern String activePin;
String get_locker_status();

void firebase_setup();
void firebase_loop();
void update_firebase_lock_state(bool isUnlocked);
void send_notification(const String& title, const String& body);

#endif