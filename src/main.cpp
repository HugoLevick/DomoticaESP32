#include <Arduino.h>
#include <MFRC522.h>
#include <SPI.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <DHT.h>

#include "config.h"
#include "actuadores/led.h"
#include "actuadores/alarma.h"
#include "actuadores/ac.h"
#include "sensores/tactil.h"
#include "sensores/apertura.h"
#include "sensores/proximidad.h"
#include "sensores/temperatura.h"

//Pines para el RFID
#define RST_PIN	2
#define SS_PIN	5

//Cliente http para hacer peticiones hacia el gateway
HTTPClient http;

MFRC522 rfid(SS_PIN, RST_PIN); //Creamos el objeto para el RFID

//Creacion de alarma
SensorApertura puertaSensorApertura(36);
Alarma alarma(&http, &rfid, &puertaSensorApertura);

//Creacion de AC
SensorTemperatura sensorTempRecamara(27);
Led ledAC("AC", 26);
AireAcondicionado ac(&sensorTempRecamara, &ledAC);


// MFRC522::MIFARE_Key keyA = {keyByte: {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5}};
// MFRC522::MIFARE_Key keyB = {keyByte: {0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5}};

//Creacion de Leds
Led ledCocina("Cocina", 33, &http);

SensorTactil botonPA(35);
Led ledPA("P. A.", 32, &http, &botonPA);

SensorProximidad sensorProximidadBano(13);
Led ledBano("Baño", 25, &http, nullptr, &sensorProximidadBano);

SensorTactil botonSala(22);
Led ledSala("Sala", 26, &http, &botonSala);

//Guardar leds en un array para poder iterar sobre ellos
Led identificadoresLed[] = {ledCocina, ledBano, ledSala, ledPA};
int ledArrayLength = sizeof(identificadoresLed) / sizeof(identificadoresLed[0]);

//Crear servidor web
AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);
  Serial.println();

  //Inicialización de actuadores y sensores
  alarma.iniciar();
  rfid.PCD_DumpVersionToSerial();	//Muestra la version del firmware del lector
  sensorTempRecamara.iniciar();


  for (int i = 0; i < ledArrayLength; i++) {
      Led& led = identificadoresLed[i];
      led.iniciar();
  }

  //Conectar WiFi
  WiFi.begin(Config::ssid, Config::password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  //Imprimir dirección IP
  Serial.println("Connected to WiFi");
  Serial.print("ESP32 IP address: ");
  Serial.println(WiFi.localIP());


  //------------------------------- Agregar rutas al servidor ----------------------------

  //Ruta para obtener el estado de los leds
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
      Led& led = identificadoresLed[i];
      JsonObject ledJson = data.createNestedObject();
      ledJson["nombre"] = led.nombre;
      ledJson["estado"] = led.estado;
    }

    String json;
    serializeJson(root, json);
    request->send(200, "application/json", json);

  });

  //Ruta para cambiar estado de los leds
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
        Led* led = nullptr;
        for (int i = 0; i < ledArrayLength; i++) {
          if(identificadoresLed[i].nombre == dispositivo->value()) {
            led = &identificadoresLed[i];
          }
        }

        if(led) {
          //Si se hacen cambios, cambiar tambien el otro modo de actualizar
          led->flip(true);
          DynamicJsonDocument root(256);
          root["state"] = led->estado;
          String json;
          serializeJson(root, json);
          return request->send(200, "application/json", json);
        } else {
          return request->send(400, "text/plain", "No se encontro el dispositivo");  
        }
      } else if(dispositivo && accion) {
        Led* led = nullptr;
        for (int i = 0; i < ledArrayLength; i++) {
          if(identificadoresLed[i].nombre == dispositivo->value()) {
            led = &identificadoresLed[i];
          }
        }

        if(led && accion->value() == "encender") {
          led->encender(true);
        } else if(led) {
          led->apagar(true);
        } else {
          return request->send(400, "text/plain", "No se encontro el dispositivo");  
        }

        //Si se hacen cambios, cambiar tambien el otro modo de actualizar
        DynamicJsonDocument root(256);
        root["state"] = led->estado;
        String json;
        serializeJson(root, json);
        return request->send(200, "application/json", json);
      } else {
        // Query parameter "dispositivo" is not present
        return request->send(400, "text/plain", "Se necesita un dispositivo y la accion");
      }
  });

  //Ruta para asignar tarjeta nfc a un usuario
  server.on("/asignarRfid", HTTP_GET, [](AsyncWebServerRequest *request){
    // Check if the user is authenticated
    if (!request->authenticate(Config::http_username, Config::http_password)) {
      // User is not authenticated, respond with Unauthorized
      return request->send(401, "Autorizacion fallida");
    }

    String uid = alarma.getUltimoUidLeido();
    if(uid != "") {
      Serial.println("Asignando tarjeta");
      DynamicJsonDocument root(1024); // Adjust the capacity as needed
      root["uid"] = uid;
      String json;
      serializeJson(root, json);
      return request->send(200, "application/json", json);
    } else {
      return request->send(400, "text/plain", "No se detecto ninguna tarjeta");
    }
  });

  //Ruta para obtener las alarmas
  server.on("/alarmas", HTTP_GET, [](AsyncWebServerRequest *request){
    String json;
    serializeJson(alarma.obtenerAlarmasActivasJSON(), json);
    request->send(200, "application/json", json);
  });

  //Ruta para silenciar las alarmas
  server.on("/silenciarAlarmas", HTTP_GET, [](AsyncWebServerRequest *request){
    alarma.reiniciarAlarma();
    request->send(200);
  });

  //Ruta para obtener estadísticas (temperatura, humedad, etc)
  server.on("/estadisticas", HTTP_GET, [](AsyncWebServerRequest *request){
    DynamicJsonDocument root(256); // Adjust the capacity as needed
    root["temperatura"] = sensorTempRecamara.getTemperatura();
    root["humedad"] = sensorTempRecamara.getHumedad();
    root["acceso"] = alarma.getUltimoUidLeido(true);
    String json;
    serializeJson(root, json);
    request->send(200, "application/json", json);
  });

  server.begin();
}

void loop() {
  //Actualizar actuadores
  alarma.actualizar();
  ac.actualizar();

  // Actualizar los leds (revisar si se presiono el boton asignado)
  for (int i = 0; i < ledArrayLength; i++) {
    identificadoresLed[i].actualizar();
  }
}