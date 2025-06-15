#ifndef FIREBASE_HANDLER_H
#define FIREBASE_HANDLER_H

void firebase_setup();
void firebase_loop();
void update_firebase_lock_state(bool isUnlocked);
void send_notification(const String& title, const String& body);

#endif