#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "display.h"
#include "sound.h"
#include "ble.h"

// Define UUIDs
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// These definitions should stay:
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

int beeperPin = 14;  // Pin del buzzer

// Variables para almacenar la información de la última notificación
String lastAppName = "";
String lastTitle = "";
String lastText = "";
unsigned long lastNotificationTime = 0;
const unsigned long DISPLAY_TIMEOUT = 30000; // 30 segundos para mostrar la notificación

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando ESP32 BLE Server con OLED...");

  // Inicializar la pantalla OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("Error al inicializar la pantalla SSD1306"));
  }

  // Mostrar pantalla de inicio
  showStartupScreen();

  pinMode(beeperPin, OUTPUT);
  digitalWrite(beeperPin, LOW);
  ledcAttach(beeperPin, LEDC_BASE_FREQ, 8);

  // Configurar BLE usando la función en ble.cpp
  setupBLE();

  Serial.println("ESP32 BLE listo para recibir notificaciones");
  updateDisplay();
  playVintageBeep();
}

void loop() {
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    Serial.println("Publicidad reiniciada");
    oldDeviceConnected = deviceConnected;
    updateDisplay();
  }

  if (deviceConnected && !oldDeviceConnected) {
    Serial.println("Nueva conexión establecida");
    playVintageBeep();
    oldDeviceConnected = deviceConnected;
    updateDisplay();
  }
  
  static unsigned long lastDisplayUpdate = 0;
  if (millis() - lastNotificationTime > DISPLAY_TIMEOUT && 
      millis() - lastDisplayUpdate > 5000) {
    updateDisplay();
    lastDisplayUpdate = millis();
  }
}