#pragma once
#include <Arduino.h>

class Config {
public:
    static const char* ssid;
    static const char* password;
    static const char* http_username;
    static const char* http_password;
    static String gateway_ip;
};
