#pragma once

#include <Arduino.h>

class SensorTactil {
public:
    SensorTactil(int _pin) : pin(_pin) {} // Constructor, inicializa el boton

    void iniciar() {
        pinMode(pin, INPUT);
    }

    bool presionado() {
        //Evaluar si cambió el estado al anterior y el estado actual es presionado
        bool estadoActual = digitalRead(pin);
        bool presionado = !estadoAnterior && estadoActual;
        estadoAnterior = estadoActual;
        return presionado;
    }

private:
    int pin;
    bool estadoAnterior = false;
};