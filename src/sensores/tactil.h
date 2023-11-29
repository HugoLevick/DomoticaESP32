#pragma once

#include <Arduino.h>

class SensorTactil {
public:
    SensorTactil(int _pin) : pin(_pin) {} // Constructor, inicializa el boton

    void iniciar() {
        pinMode(pin, INPUT);
    }

    bool presionado() {
        bool estadoActual = digitalRead(pin);
        bool presionado = !estadoAnterior && estadoActual;
        estadoAnterior = estadoActual;
        return presionado;
    }

private:
    int pin;
    bool estadoAnterior = false;
};