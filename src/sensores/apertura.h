#pragma once

#include <Arduino.h>

class SensorApertura {
public:
    SensorApertura(int _pin) : pin(_pin) {} // Constructor, inicializa el boton

    void iniciar() {
        pinMode(pin, INPUT_PULLUP);
    }

    bool abierto() {
        return !digitalRead(pin);
    }

private:
    int pin;
};