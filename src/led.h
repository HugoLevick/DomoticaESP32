#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "button.h"

class Led {
public:
    Led(String _nombre, int _pin, Button _button) : nombre(_nombre), pin(_pin), button(_button) {} // Constructor, inicializa el led con su boton

    String nombre;
    bool estado = false;
    int pin;
    Button button;

    void iniciar() {
        pinMode(pin, OUTPUT);
        button.iniciar();
    }
    
    void encender() {
        estado = HIGH;
        digitalWrite(pin, estado);
    }

    void apagar() {
        estado = LOW;
        digitalWrite(pin, estado);
    }

    void flip() {
        estado = !estado;
        digitalWrite(pin, estado);
    }

    void actualizar() {
        if(button.presionado()) {
            flip();
            HTTPClient http;

            String serverPath = Config::gateway_ip + "/leds/actualizar/";

            http.begin(serverPath);
            http.addHeader("Content-Type", "application/json");  //Specify content-type header

            DynamicJsonDocument doc(1024);
            doc["ledName"] = nombre;
            doc["state"] = estado ? true : false;
            String requestBody;
            serializeJson(doc, requestBody);

            int httpResponseCode = http.PUT(requestBody);   //Send the actual POST request
            Serial.println(httpResponseCode);
        }
    }
};