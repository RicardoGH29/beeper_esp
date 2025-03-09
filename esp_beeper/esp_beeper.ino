#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Define UUIDs
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#define LEDC_BASE_FREQ  2000  // Frecuencia base para tonos

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

int beeperPin = 14;  // Pin del buzzer

// Declaraciones adelantadas de funciones
void playTone(int frequency, int duration);
void playClassicPagerTone();
void playChirp();
void playVintageBeep();

// Implementación de funciones de sonido
void playTone(int frequency, int duration) {
  ledcWriteTone(beeperPin, frequency);  // Genera el tono
  delay(duration);                      // Mantiene el tono por el tiempo indicado
  ledcWriteTone(beeperPin, 0);          // Apaga el tono
}

// Patrón clásico "Beep largo + dos cortos"
void playClassicPagerTone() {
  Serial.println("Reproduciendo tono de buscapersonas clásico...");
  playTone(1800, 300); // Beep largo
  delay(100);
  playTone(2000, 100); // Beep corto
  delay(100);
  playTone(2200, 100); // Beep corto
  delay(200);
}

// Tono "chirp" con vibrato (efecto old-school)
void playChirp() {
  Serial.println("Reproduciendo chirp con vibrato...");
  for (int i = 0; i < 3; i++) {
    playTone(2100 + (i * 100), 80);
  }
  playTone(2500, 250);
}

// Función principal para reproducir sonido de beeper vintage
void playVintageBeep() {
  playClassicPagerTone();
  delay(200);
  playClassicPagerTone();
}

// Clase para manejar callbacks de conexión BLE
class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Dispositivo conectado");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Dispositivo desconectado");
      // Reiniciar publicidad cuando se desconecta
      pServer->startAdvertising();
    }
};

// Callback para manejar escrituras del cliente
class CharacteristicCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    uint8_t* pData = pCharacteristic->getData();
    size_t dataLen = pCharacteristic->getLength();
    
    if (dataLen > 0) {
        // Crear un buffer temporal para almacenar los datos como una cadena terminada en null
        char* messageBuffer = (char*)malloc(dataLen + 1);
        if (messageBuffer) {
            memcpy(messageBuffer, pData, dataLen);
            messageBuffer[dataLen] = 0; // Asegurar que termine con null
            
            Serial.println("=== Notificación recibida ===");
            
            // Buscar el primer delimitador
            char* titlePos = strchr(messageBuffer, '|');
            if (titlePos) {
                *titlePos = 0; // Terminar la cadena del appName
                titlePos++; // Avanzar al título
                
                // Buscar el segundo delimitador
                char* textPos = strchr(titlePos, '|');
                if (textPos) {
                    *textPos = 0; // Terminar la cadena del título
                    textPos++; // Avanzar al texto
                    
                    // Ahora tenemos las tres partes separadas
                    Serial.print("Aplicación: ");
                    Serial.println(messageBuffer);
                    
                    Serial.print("Título: ");
                    Serial.println(titlePos);
                    
                    Serial.print("Texto: ");
                    Serial.println(textPos);
                    
                    // Reproducir sonido cuando se recibe una notificación
                    playVintageBeep();
                }
            }
            
            Serial.println("===========================");
            free(messageBuffer);
        }
    }
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando ESP32 BLE Server...");

  pinMode(beeperPin, OUTPUT);
  digitalWrite(beeperPin, LOW);

  // Configurar PWM para el buzzer usando la nueva API
  // En la versión 3.1.1, ledcAttach solo toma 3 parámetros: pin, frecuencia, resolución
  ledcAttach(beeperPin, LEDC_BASE_FREQ, 8);  // El canal se asigna automáticamente

  // Inicializar el dispositivo BLE con un nombre visible
  BLEDevice::init("ESP32-VINTAGE-BEEPER");
  
  // Crear el servidor BLE
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
  
  // Crear el servicio BLE
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  // Crear una característica BLE
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  
  // Establecer callbacks para datos recibidos
  pCharacteristic->setCallbacks(new CharacteristicCallbacks());

  // BLE2902 necesario para notificar
  pCharacteristic->addDescriptor(new BLE2902());
  
  // Iniciar el servicio
  pService->start();
  
  // Iniciar publicidad con un nombre de dispositivo visible
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // ayuda con problemas de conexión iPhone
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("ESP32 BLE listo para recibir notificaciones");
  
  // Reproducir un sonido de inicio para confirmar que está funcionando
  playVintageBeep();
}

void loop() {
  // Manejar cambios de estado de conexión
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // Dar tiempo para que la pila BLE se prepare
    pServer->startAdvertising(); // Reiniciar publicidad
    Serial.println("Publicidad reiniciada");
    oldDeviceConnected = deviceConnected;
  }

  // Manejar nueva conexión
  if (deviceConnected && !oldDeviceConnected) {
    Serial.println("Nueva conexión establecida");
    playVintageBeep();  // Reproducir sonido cuando se conecta un dispositivo
    oldDeviceConnected = deviceConnected;
  }
}