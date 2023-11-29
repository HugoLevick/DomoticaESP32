#include <Arduino.h>
#include "config.h"

const char* Config::ssid = "redESP";
const char* Config::password = "pepepecas";
const char* Config::http_username = "admin";
const char* Config::http_password = "admin";
String Config::gateway_ip = "http://192.168.52.220:3000/api";
