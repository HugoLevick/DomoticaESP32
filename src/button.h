#pragma once

#include <Arduino.h>

class Button {
public:
    Button(int pinNumber) : pin(pinNumber) {} // Constructor, inicializa el boton

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