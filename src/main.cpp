#include <Arduino.h>
#include <MFRC522.h>
#include <SPI.h>
#include <ESPAsyncWebServer.h>
#include <map>

#include "config.h"
#include "led.h"
#include "button.h"

#define RST_PIN	2
#define SS_PIN	5

MFRC522 rfid(SS_PIN, RST_PIN); //Creamos el objeto para el RFID

Led identificadoresLed[1] = {
  Led("ledAzul", 21, Button(22))
};

int ledArrayLength = sizeof(identificadoresLed) / sizeof(identificadoresLed[0]);

AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);
  Serial.println();
  SPI.begin();
  Serial.println("SPI inicializado");
  rfid.PCD_Init();
  Serial.println("RFID inicializado");

  for (int i = 0; i < ledArrayLength; i++) {
      Led led = identificadoresLed[i];
      led.iniciar();
  }
  

  // Connect to Wi-Fi
  WiFi.begin(Config::ssid, Config::password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  // Print the ESP32's IP address
  Serial.println("Connected to WiFi");
  Serial.print("ESP32 IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/leds", HTTP_GET, [](AsyncWebServerRequest *request){
    // Check if the user is authenticated
    if (!request->authenticate(Config::http_username, Config::http_password)) {
      // User is not authenticated, respond with Unauthorized
      return request->send(401, "Autorizacion fallida");
    }

    DynamicJsonDocument root(1024); // Adjust the capacity as needed

    // Create an array
    JsonArray data = root.to<JsonArray>();

    //Traverse leds
    int i = 0;
    for (int i = 0; i < ledArrayLength; i++) {
      Led led = identificadoresLed[i];
      JsonObject ledJson = data.createNestedObject();
      ledJson["nombre"] = led.nombre;
      ledJson["estado"] = led.estado;
    }

    String json;
    serializeJson(root, json);
    request->send(200, "application/json", json);

  });

  // Define routes and handlers
  server.on("/leds", HTTP_POST, [](AsyncWebServerRequest *request){
     // Check if the user is authenticated
      if (!request->authenticate(Config::http_username, Config::http_password)) {
        // User is not authenticated, respond with Unauthorized
        return request->send(401, "Autorizacion fallida");
      }

      AsyncWebParameter* dispositivo = request->getParam("dispositivo");
      AsyncWebParameter* accion = request->getParam("accion");

      if (dispositivo && !accion) {
        //Query parameter "dispositivo" is present
        Led* led;
        for (int i = 0; i < ledArrayLength; i++) {
          if(identificadoresLed[i].nombre == dispositivo->value()) {
            led = &identificadoresLed[i];
          }
        }

        if(led) {
          led->flip();
          return request->send(200);
        } else {
          return request->send(400, "text/plain", "No se encontro el dispositivo");  
        }
      } else if(dispositivo && accion) {
        Led* led;
        for (int i = 0; i < ledArrayLength; i++) {
          if(identificadoresLed[i].nombre == dispositivo->value()) {
            led = &identificadoresLed[i];
          }
        }

        if(led && accion->value() == "encender") {
          led->encender();
          return request->send(200);
        } else if(led) {
          led->apagar();
          return request->send(200);
        } else {
          return request->send(400, "text/plain", "No se encontro el dispositivo");  
        }
      } else {
        // Query parameter "dispositivo" is not present
        return request->send(400, "text/plain", "Se necesita un dispositivo y la accion");
      }
  });

  server.begin();

}

void loop() {
  //Revisamos si hay nuevas tarjetas  presentes
	if ( rfid.PICC_IsNewCardPresent()) 
    Serial.println("NFC Detectado");
    {
    //Seleccionamos una tarjeta
    if ( rfid.PICC_ReadCardSerial()) 
    {
      // Enviamos serialemente su UID
      Serial.print("Card UID:");
      for (byte i = 0; i < rfid.uid.size; i++) {
              Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
              Serial.print(rfid.uid.uidByte[i], HEX);   
      }
      Serial.println();
      // Terminamos la lectura de la tarjeta  actual
      rfid.PICC_HaltA();
    }
	}

  // Actualizar los leds (revisar si se presiono el boton asignado)
  for (int i = 0; i < ledArrayLength; i++) {
    identificadoresLed[i].actualizar();
  }
}
