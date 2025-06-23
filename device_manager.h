// Tambahkan deklarasi fungsi ini di dalam file device_manager.h

#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

// --- TAMBAHKAN DEKLARASI EXTERN DI SINI ---
extern unsigned long unlockTimestamp;

void device_setup();
void device_loop();
void check_password();

// --- DEKLARASI FUNGSI BARU UNTUK LED RGB ---
void led_set_color(int r, int g, int b);
void led_show_available();
void led_show_occupied();
void led_show_maintenance(); // Meskipun sama dengan occupied, ini untuk kejelasan
void led_turn_off();

#endif