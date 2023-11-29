#pragma once

#include <Arduino.h>
#include <DHT.h>

#define INTERVALO_LECTURA 20000

#define DHTTYPE DHT11

class SensorTemperatura {
    public:
        SensorTemperatura(int _pinDHT) : dht(_pinDHT, DHTTYPE) {}

        void iniciar() {
            dht.begin();
        }

        float getTemperatura() {
            actualizarDatos();
            return temperatura;
        }

        float getHumedad() {
            actualizarDatos();
            return humedad;
        }

    private:
        DHT dht;
        unsigned long ultimaVezLeido = 0;
        float temperatura = 0;
        float humedad = 0;

        void actualizarDatos() {
            if(millis() - ultimaVezLeido > INTERVALO_LECTURA || ultimaVezLeido == 0) {
                // Leemos la temperatura en grados centígrados
                temperatura = dht.readTemperature();
                humedad = dht.readHumidity();
                ultimaVezLeido = millis();

                // Comprobamos si ha habido algún error en la lectura
                if (isnan(temperatura)) {
                    Serial.println("Error obteniendo los datos del sensor DHT11");
                }
            }
        }
};