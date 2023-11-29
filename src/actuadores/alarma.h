#pragma once

#include <Arduino.h>
#include <MFRC522.h>

#include "../sensores/apertura.h"

#define EXPIRACION_UID 5000UL
#define TIEMPO_DESBLOQUEO 10000UL
#define LIMITE_INTENTOS_FALLIDOS 3

class Alarma {
public:
    Alarma(
        HTTPClient* _http,
        MFRC522* _rfid,
        SensorApertura* _sensorPuerta
    )
    :
        http(_http),
        rfid(_rfid),
        sensorPuerta(_sensorPuerta)
    {} // Constructor, inicializa el boton

    void iniciar() {
        SPI.begin();
        Serial.println("SPI inicializado");
        rfid->PCD_Init();

        sensorPuerta->iniciar();
        estado = true;
    }

    String getUltimoUidLeido(bool ignorarExpiracion = false) {
        if(ignorarExpiracion) {
            return ultimoUidLeido;
        }

        if(millis() - ultimoTiempoUidLeido < EXPIRACION_UID) {
            return ultimoUidLeido;
        }

        return "";
    }

    void actualizar() {
        desbloqueado = millis() - ultimoTiempoDesbloqueado < TIEMPO_DESBLOQUEO;

        if(intentosFallidos >= LIMITE_INTENTOS_FALLIDOS && !alarmaIntentoFallidoEnviada) {
            Serial.println("Intruso detectado");
            //Sonar alarma
            alarmaIntentoFallido = true;
            int codigo = enviarAlarma("rfid");
            if(codigo == 200) {
                alarmaIntentoFallidoEnviada = true;
            }
        }

        if(!desbloqueado) {
            if(sensorPuerta->abierto() && estado == true && !alarmaDesbloqueoEnviada) {
                Serial.println("Intruso detectado");
                //Sonar alarma
                alarmaDesbloqueo = true;
                int codigo = enviarAlarma("puerta");
                if(codigo == 200) {
                    alarmaDesbloqueoEnviada = true;
                }
            }
        }

        if (rfid->PICC_IsNewCardPresent()  && rfid->PICC_ReadCardSerial() && !alarmaIntentoFallido) {
            // Mostrar información de la tarjeta por el monitor serie
            Serial.print(F("UID de la tarjeta:"));
            mostrarByteArray(rfid->uid.uidByte, rfid->uid.size);  // Motrar el UID
            guardarUltimoUid(rfid->uid.uidByte, rfid->uid.size);
            Serial.println();

            rfid->PICC_HaltA(); // Detener la tarjeta actual

            String serverPath = Config::gateway_ip + "/auth/rfid/autenticar/";

            http->begin(serverPath);
            http->addHeader("Content-Type", "application/json");  //Specify content-type header

            DynamicJsonDocument doc(256);
            doc["uid"] = ultimoUidLeido;
            String requestBody;
            serializeJson(doc, requestBody);

            int httpResponseCode = http->POST(requestBody);   //Send the actual POST request
            http->end();
            Serial.println(httpResponseCode);

            if(httpResponseCode == 200) {
                reiniciarAlarma();
                Serial.println("Desbloqueado");
            } else {
                intentosFallidos++;
                ultimoTiempoIntentoFallido = millis();
                Serial.println("No autorizado");
            }
        }
    }

    void reiniciarAlarma() {
        intentosFallidos = 0;
        alarmaIntentoFallido = false;
        alarmaIntentoFallidoEnviada = false;

        desbloqueado = true;
        alarmaDesbloqueo = false;
        alarmaDesbloqueoEnviada = false;
        ultimoTiempoDesbloqueado = millis();
    }

    DynamicJsonDocument obtenerAlarmasActivasJSON() {
        DynamicJsonDocument doc(512);
        JsonArray alarmasActivas = doc.to<JsonArray>();

        if (alarmaDesbloqueo) {
            alarmasActivas.add("puerta");
        }

        if (alarmaIntentoFallido) {
            alarmasActivas.add("rfid");
        }

        return doc;
    }

    // bool accionado() {
    //     return !digitalRead(pin);
    // }

private:
    MFRC522* rfid;
    HTTPClient* http;
    bool estado = false;
    SensorApertura* sensorPuerta;

    String ultimoUidLeido = "";
    unsigned long ultimoTiempoUidLeido = 0;

    bool desbloqueado = false;
    unsigned long ultimoTiempoDesbloqueado = 0;
    bool alarmaDesbloqueo = false;
    bool alarmaDesbloqueoEnviada = false;

    int intentosFallidos = 0;
    bool alarmaIntentoFallido = false;
    unsigned long ultimoTiempoIntentoFallido = 0;
    bool alarmaIntentoFallidoEnviada = false;

    void mostrarByteArray(byte* buffer, byte bufferSize) {
        for (byte i = 0; i < bufferSize; i++) {
            Serial.print(buffer[i] < 0x10 ? " 0" : " ");
            Serial.print(buffer[i], HEX);
        }
    }

    void guardarUltimoUid(byte* uid, byte uidLength) {
        ultimoUidLeido = "";
        for (byte i = 0; i < uidLength; i++) {
            ultimoUidLeido += String(uid[i] < 0x10 ? "0" : "");
            ultimoUidLeido += String(uid[i], HEX);
        }
        ultimoTiempoUidLeido = millis();
    }

    int enviarAlarma(String motivo) {
        String serverPath = Config::gateway_ip + "/auth/alarmas/";

        http->begin(serverPath);
        http->addHeader("Content-Type", "application/json");  //Specify content-type header

        DynamicJsonDocument doc(256);
        doc["motivo"] = motivo;
        String requestBody;
        serializeJson(doc, requestBody);

        int httpResponseCode = http->POST(requestBody);   //Send the actual POST request
        http->end();
        Serial.println(httpResponseCode);
        return httpResponseCode;
    }
};