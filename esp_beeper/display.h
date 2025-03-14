#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>

// Display constants
#define SCREEN_WIDTH 128 // Ancho de la pantalla OLED en píxeles
#define SCREEN_HEIGHT 64 // Alto de la pantalla OLED en píxeles
#define OLED_RESET    -1 // Pin de reset (o -1 si comparte con el reset de Arduino)
#define SCREEN_ADDRESS 0x3C // Dirección I2C de la pantalla (típicamente 0x3C o 0x3D)
#define SSD1306_SWITCHCAPVCC 0x2

// Declare display as external so it can be used in other files
extern Adafruit_SSD1306 display;

// Function declarations
void updateDisplay();
void showStartupScreen();
void setClockFromPhone(int hours, int minutes, int seconds, int day, int month, int year);

#endif // DISPLAY_H