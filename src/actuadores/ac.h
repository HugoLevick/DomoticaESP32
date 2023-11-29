#pragma once

#include <Arduino.h>
#include <DHT.h>

#include "../sensores/temperatura.h"
#include "../actuadores/led.h"

class AireAcondicionado {
    public:
        AireAcondicionado(SensorTemperatura* _temp, Led* _led) : temp(_temp), led(_led){};

        bool estado = false;

        void actualizar() {
            float temperatura = temp->getTemperatura();

            bool encenderAire = temperatura > 25.0;
            if(encenderAire && encenderAire != estado) {
                estado = encenderAire;
                led->encender();
                Serial.println("Encendiendo aire acondicionado");
            } else if(encenderAire != estado) {
                estado = encenderAire;
                led->apagar();
                Serial.println("Apagando aire acondicionado");
            }
        }

    private:
        SensorTemperatura* temp;
        Led* led;
};