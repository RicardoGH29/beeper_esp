#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "ble.h"
#include "display.h"
#include "sound.h"

// These variables are now defined in the main .ino file
// Just declare them as external references here
extern BLEServer *pServer;
extern BLECharacteristic *pCharacteristic;
extern bool deviceConnected;
extern bool oldDeviceConnected;

extern String lastAppName;
extern String lastTitle;
extern String lastText;
extern unsigned long lastNotificationTime;

// Implement methods for ServerCallbacks
void ServerCallbacks::onConnect(BLEServer *pServer)
{
  deviceConnected = true;
  Serial.println("Dispositivo conectado");
  updateDisplay();
}

void ServerCallbacks::onDisconnect(BLEServer *pServer)
{
  deviceConnected = false;
  Serial.println("Dispositivo desconectado");
  updateDisplay();
  // Reiniciar publicidad cuando se desconecta
  pServer->startAdvertising();
}

// Implement methods for CharacteristicCallbacks
void CharacteristicCallbacks::onWrite(BLECharacteristic *pCharacteristic)
{
  uint8_t *pData = pCharacteristic->getData();
  size_t dataLen = pCharacteristic->getLength();

  if (dataLen > 0)
  {
    // Crear un buffer temporal para almacenar los datos como una cadena terminada en null
    char *messageBuffer = (char *)malloc(dataLen + 1);
    if (messageBuffer)
    {
      memcpy(messageBuffer, pData, dataLen);
      messageBuffer[dataLen] = 0; // Asegurar que termine con null

      Serial.println("=== Mensaje recibido ===");

      // Buscar el primer delimitador para obtener el tipo
      char *typeEnd = strchr(messageBuffer, '|');
      if (typeEnd)
      {
        *typeEnd = 0; // Terminar la cadena del tipo
        typeEnd++;    // Avanzar al contenido

        String messageType = String(messageBuffer);
        Serial.print("Tipo de mensaje: ");
        Serial.println(messageType);
        Serial.print("Contenido: ");
        Serial.println(typeEnd);

        if (messageType == "notification")
        {
          // Buscar el primer delimitador del contenido (segundo delimitador general)
          char *titlePos = strchr(typeEnd, '|');
          if (titlePos)
          {
            *titlePos = 0; // Terminar la cadena del appName
            titlePos++;    // Avanzar al título

            // Buscar el segundo delimitador del contenido (tercer delimitador general)
            char *textPos = strchr(titlePos, '|');
            if (textPos)
            {
              *textPos = 0; // Terminar la cadena del título
              textPos++;    // Avanzar al texto

              // Ahora tenemos las tres partes separadas
              Serial.print("Aplicación: ");
              Serial.println(typeEnd);

              Serial.print("Título: ");
              Serial.println(titlePos);

              Serial.print("Texto: ");
              Serial.println(textPos);

              // Actualizar variables para la pantalla
              lastAppName = String(typeEnd);
              lastTitle = String(titlePos);
              lastText = String(textPos);
              lastNotificationTime = millis();

              // Actualizar la pantalla
              updateDisplay();

              // Reproducir sonido cuando se recibe una notificación
              playVintageBeep();
            }
          }
        }
        else if (messageType == "time")
        {
          // Procesar mensaje de actualización de hora
          // El formato esperado es time|hora|minuto|segundo|día|mes|año
          Serial.print("Actualizando hora: ");
          Serial.println(typeEnd);

          // Variables para almacenar los componentes del tiempo
          int hours = -1, minutes = -1, seconds = -1, day = -1, month = -1, year = -1;
          bool formatoValido = true;

          // Usar strtok para separar los valores
          char *token = strtok(typeEnd, "|");
          if (token)
            hours = atoi(token);
          else
            formatoValido = false;

          token = strtok(NULL, "|");
          if (token)
            minutes = atoi(token);
          else
            formatoValido = false;

          token = strtok(NULL, "|");
          if (token)
            seconds = atoi(token);
          else
            formatoValido = false;

          token = strtok(NULL, "|");
          if (token)
            day = atoi(token);
          else
            formatoValido = false;

          token = strtok(NULL, "|");
          if (token)
            month = atoi(token);
          else
            formatoValido = false;

          token = strtok(NULL, "|");
          if (token)
            year = atoi(token);
          else
            formatoValido = false;

          if (formatoValido)
          {
            // Imprimir valores para depuración
            Serial.printf("Hora: %02d:%02d:%02d Fecha: %02d/%02d/%04d\n",
                          hours, minutes, seconds, day, month, year);

            // Actualizar el reloj con los valores obtenidos
            setClockFromPhone(hours, minutes, seconds, day, month, year);

            // Actualizar la pantalla
            updateDisplay();
          }
          else
          {
            Serial.println("Error: formato de hora inválido");
          }
        }
        // Puedes añadir más tipos de mensajes según necesites
      }

      Serial.println("===========================");
      free(messageBuffer);
    }
  }
}

void setupBLE()
{
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
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY);

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
  pAdvertising->setMinPreferred(0x06); // ayuda con problemas de conexión iPhone
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("ESP32 BLE listo para recibir notificaciones");
}