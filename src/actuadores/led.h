#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "../config.h"
#include "../sensores/tactil.h"
#include "../sensores/proximidad.h"

#define PROXIMITY_DELAY 2000UL

class Led {
    public:
        Led(
            String _nombre,
            int _pin,
            HTTPClient* _http = nullptr,
            SensorTactil* _boton = nullptr,
            SensorProximidad* _proximitySensor = nullptr
        )
        :
            nombre(_nombre),
            pin(_pin),
            http(_http),
            boton(_boton),
            sensorProximidad(_proximitySensor)
        {} // Constructor, inicializa el led con sus atributos

        String nombre;
        bool estado = false;

        void iniciar(bool web = false) {
            //Declarar modo de pin y sensores asociados
            pinMode(pin, OUTPUT);
            if(boton) {
                boton->iniciar();
            }
            if(sensorProximidad) {
                sensorProximidad->iniciar();
            }
        }
        
        //Variable web se usa para saber si la petición fue enviada desde fuera (internet, etc.)
        void encender(bool web = false) {
            if(web) {
                establecerEstadoWeb(HIGH, estado);
            } else {
                estado = HIGH;
            }
            digitalWrite(pin, estado);
        }

        void apagar(bool web = false) {
            if(web) {
                establecerEstadoWeb(LOW, estado);
            } else {
                estado = LOW;
            }
            digitalWrite(pin, estado);
        }

        void flip(bool web = false) {
            if(web) {
                establecerEstadoWeb(!estado, estado);
            } else {
                estado = !estado;
            }
            digitalWrite(pin, estado);
        }

        void actualizar() {
            //Verificar si se presionó el botón asociado y cambiar el estado
            if(boton && boton->presionado()) {
                flip();
                enviarEstado();   
            }
            
            //Verificar si hubo un cambio en el sensor y cambiar el estado
            if (sensorProximidad && !actuadoWeb) {
                unsigned long tiempoActual = millis();
                if(tiempoActual - sensorProxUltVezActuado >= PROXIMITY_DELAY) {
                    bool nuevoEstado = sensorProximidad->accionado();

                    if(nuevoEstado != estado) {
                        nuevoEstado ? encender() : apagar();
                        sensorProxUltVezActuado = tiempoActual;
                        enviarEstado();
                    }
                }
            }
        
        }


        //Función para enviar el estado del led al gateway
        void enviarEstado() {
            if(!http) {
                return;
            }
            String serverPath = Config::gateway_ip + "/leds/notificar/";

            http->begin(serverPath);
            http->addHeader("Content-Type", "application/json");  //Specify content-type header

            DynamicJsonDocument doc(256);
            doc["ledName"] = nombre;
            doc["state"] = estado ? true : false;
            String requestBody;
            serializeJson(doc, requestBody);

            int httpResponseCode = http->PUT(requestBody);   //Send the actual POST request
            http->end();
            Serial.println(httpResponseCode);
        }

    private:
        int pin;
        HTTPClient* http;
        SensorTactil* boton;
        SensorProximidad* sensorProximidad;
        unsigned long sensorProxUltVezActuado = 0;
        //Usado por el sensor de proximidad para saber si actúa ante el cambio)
        bool actuadoWeb = false;

        //Establece el estado del led en caso de que se hiciera petición de cambio por el servidor web
        void establecerEstadoWeb(bool nuevoEstado, bool estadoAnterior) {
            if(actuadoWeb && estadoAnterior != nuevoEstado) {
                actuadoWeb = false;
            } else {
                actuadoWeb = true;
            }
            estado = nuevoEstado;
        }
};