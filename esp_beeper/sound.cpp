#include <Arduino.h>

extern int beeperPin;  // Pin del buzzer

void playTone(int frequency, int duration) {
  ledcWriteTone(beeperPin, frequency);  // Genera el tono
  delay(duration);                      // Mantiene el tono por el tiempo indicado
  ledcWriteTone(beeperPin, 0);          // Apaga el tono
}

void playClassicPagerTone() {
  Serial.println("Reproduciendo tono de buscapersonas clásico...");
  playTone(1800, 300); // Beep largo
  delay(100);
  playTone(2000, 100); // Beep corto
  delay(100);
  playTone(2200, 100); // Beep corto
  delay(200);
}

void playChirp() {
  Serial.println("Reproduciendo chirp con vibrato...");
  for (int i = 0; i < 3; i++) {
    playTone(2100 + (i * 100), 80);
  }
  playTone(2500, 250);
}

void playVintageBeep() {
  playClassicPagerTone();
  delay(200);
  playClassicPagerTone();
}