#pragma once

#include <Arduino.h>

class SensorProximidad {
public:
    SensorProximidad(int _pin) : pin(_pin) {} // Constructor, inicializa el boton

    void iniciar() {
        pinMode(pin, INPUT);
    }

    bool accionado() {
        return !digitalRead(pin);
    }

private:
    int pin;
};