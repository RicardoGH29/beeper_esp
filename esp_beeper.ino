#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Define UUIDs
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// Callback para cuando un dispositivo se conecta o desconecta
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

// Callback para manejar escrituras del cliente - VERSIÓN CORREGIDA
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

  // Inicializar el dispositivo BLE con un nombre visible
  BLEDevice::init("ESP32-BEEPER");

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

  Serial.println("ESP32 BLE listo para recibir datos del teléfono");
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
    oldDeviceConnected = deviceConnected;
  }
}