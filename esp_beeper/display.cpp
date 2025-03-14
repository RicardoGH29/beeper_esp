#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "display.h"

// Configuración de la pantalla OLED
#define SCREEN_WIDTH 128 // Ancho de la pantalla OLED en píxeles
#define SCREEN_HEIGHT 64 // Alto de la pantalla OLED en píxeles
#define OLED_RESET    -1 // Pin de reset (o -1 si comparte con el reset de Arduino)
#define SCREEN_ADDRESS 0x3C // Dirección I2C de la pantalla (típicamente 0x3C o 0x3D)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Variables para almacenar la información de la última notificación
extern String lastAppName;
extern String lastTitle;
extern String lastText;
extern unsigned long lastNotificationTime;
extern bool deviceConnected;

// Variables para el reloj y los iconos
unsigned long clockStartMillis = 0;  // Cuando se inició el reloj
unsigned long clockOffset = 0;       // Offset en segundos desde el inicio
bool clockSet = false;               // Si el reloj ha sido configurado
bool wifiConnected = false;          // Estado de conexión WiFi
int batteryLevel = 75;               // Nivel de batería (0-100%)
int currentDay = 1;
int currentMonth = 1;
int currentYear = 2025;

const unsigned long DISPLAY_TIMEOUT = 30000; // 30 segundos para mostrar la notificación

// Funciones para dibujar iconos
void drawBatteryIcon(int x, int y, int level) {
  // Dibuja el contorno de la batería
  display.drawRect(x, y, 12, 6, SSD1306_WHITE);
  display.drawRect(x+12, y+1, 2, 4, SSD1306_WHITE);
  
  // Relleno basado en el nivel (0-100%)
  int fillWidth = map(level, 0, 100, 0, 10);
  if (fillWidth > 0) {
    display.fillRect(x+1, y+1, fillWidth, 4, SSD1306_WHITE);
  }
}

void drawBluetoothIcon(int x, int y, bool connected) {
  // Símbolo de Bluetooth
  display.drawLine(x+2, y, x+2, y+6, SSD1306_WHITE);
  display.drawLine(x+2, y, x+4, y+2, SSD1306_WHITE);
  display.drawLine(x+2, y+6, x+4, y+4, SSD1306_WHITE);
  display.drawLine(x, y+1, x+3, y+4, SSD1306_WHITE);
  display.drawLine(x, y+5, x+3, y+2, SSD1306_WHITE);
  
  // Si no está conectado, dibuja una línea cruzada
  if (!connected) {
    display.drawLine(x, y, x+6, y+6, SSD1306_WHITE);
  }
}

void drawWifiIcon(int x, int y, bool connected) {
  if (connected) {
    // Tres arcos para WiFi
    display.drawCircleHelper(x+3, y+5, 6, 1, SSD1306_WHITE);
    display.drawCircleHelper(x+3, y+5, 4, 1, SSD1306_WHITE);
    display.drawCircleHelper(x+3, y+5, 2, 1, SSD1306_WHITE);
    display.drawPixel(x+3, y+5, SSD1306_WHITE);
  } else {
    // WiFi con línea cruzada
    display.drawCircleHelper(x+3, y+5, 6, 1, SSD1306_WHITE);
    display.drawCircleHelper(x+3, y+5, 4, 1, SSD1306_WHITE);
    display.drawLine(x, y, x+6, y+6, SSD1306_WHITE);
  }
}

// Función para configurar el reloj con la hora del teléfono
void setClockFromPhone(int hours, int minutes, int seconds, int day, int month, int year) {
  clockStartMillis = millis();
  clockOffset = (hours * 3600L) + (minutes * 60L) + seconds;
  currentDay = day;
  currentMonth = month;
  currentYear = year;
  clockSet = true;
}

// Función para actualizar y mostrar el reloj
void updateClock() {
  unsigned long currentMillis = millis();
  unsigned long elapsedSeconds = (currentMillis - clockStartMillis) / 1000;
  
  // Calcular la hora actual
  unsigned long totalSeconds = elapsedSeconds + clockOffset;
  
  int hours = (totalSeconds / 3600) % 24;
  int minutes = (totalSeconds / 60) % 60;
  
  // Mostrar el reloj
  display.setTextSize(2);
  char timeStr[9];
  sprintf(timeStr, "%02d:%02d", hours, minutes);
  
  // Centrar el reloj
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);
  int x = (SCREEN_WIDTH - w) / 2;
  
  display.setCursor(x, 30);
  display.print(timeStr);
}
void showDate() {
  display.setTextSize(1);
  
  char dateStr[12];
  sprintf(dateStr, "%02d/%02d/%04d", currentDay, currentMonth, currentYear);
  
  // Centrar la fecha
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(dateStr, 0, 0, &x1, &y1, &w, &h);
  int x = (SCREEN_WIDTH - w) / 2;
  
  display.setCursor(x, 50);
  display.print(dateStr);
}

void standByDisplay() {
  if (!clockSet && clockStartMillis == 0) {
    // Inicializar el reloj la primera vez
    clockStartMillis = millis();
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Dibujar iconos en la parte superior
  drawBluetoothIcon(5, 2, deviceConnected);
  drawWifiIcon(20, 2, wifiConnected);
  drawBatteryIcon(110, 2, batteryLevel);
  
  // Dibujar una línea separadora
  display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);
  
  // Actualizar y mostrar el reloj
  updateClock();
  
  // Mostrar la fecha
  showDate();
  
  display.display();
}

bool isEmptyNotification() {
  return lastAppName.length() == 0 && lastTitle.length() == 0 && lastText.length() == 0;
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  if ((millis() - lastNotificationTime < DISPLAY_TIMEOUT) && !isEmptyNotification()) {
    display.setCursor(0, 0);
    display.print("App: ");
    display.println(lastAppName);
    
    display.setCursor(0, 16);
    display.print("Titulo: ");
    display.println(lastTitle);
    
    display.setCursor(0, 32);
    display.println("Mensaje:");
    
    String textLine1 = lastText.length() > 21 ? lastText.substring(0, 21) : lastText;
    int maxLength = 42;
    int textLength = (int)lastText.length();
    int endPos = (textLength < maxLength) ? textLength : maxLength;
    String textLine2 = lastText.length() > 21 ? lastText.substring(21, endPos) : "";
    
    display.setCursor(0, 42);
    display.println(textLine1);
    
    if (textLine2.length() > 0) {
      display.setCursor(0, 52);
      display.println(textLine2);
    }
  } else {
    // Aquí podemos llamar a standByDisplay() en vez del código anterior
    standByDisplay();
    return;
  }
  
  display.display();
}

void showStartupScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  display.setCursor(8, 16);
  display.println("VINTAGE BEEPER");
  display.setCursor(20, 32);
  display.println("ESP32 + OLED");
  display.setCursor(15, 48);
  display.println("Iniciando...");
  display.display();
  delay(2000);
}