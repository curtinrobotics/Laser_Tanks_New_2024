#include <Arduino.h>
#include <WiFi.h>
#include <cstdint>
#include <esp_wifi.h>
#include <esp_now.h>
#include "HardwareSerial.h"
#include "tank_comms.h"


#define CONSOLE_CLEAR "\e[1;1H\e[2J"


uint8_t slave_address[] = { 0x80, 0x7D, 0x3A, 0x23, 0x7F, 0x58 };
const int X = 5, Y = 6;


void setup() {
    Serial.begin(9600);
    TankCommsInit(slave_address);
}


void loop()
{
    uint16_t adc_valX = analogRead(X);
    uint16_t adc_valY = analogRead(Y);

    int32_t sX = ((int32_t)adc_valX - 2047) * 127 / 2047;
    int32_t sY = ((int32_t)adc_valY - 2047) * 127 / 2047;

    int32_t L_speed = sY - sX;
    int32_t R_speed = sY + sX;

    if(L_speed > 127) {
        int32_t diff = abs(L_speed - R_speed);
        L_speed = 127;
        R_speed = 127 - diff;
    }
    if(L_speed < -127) {
        int32_t diff = abs(L_speed - R_speed);
        L_speed = -127;
        R_speed = -127 + diff;
    }

    if(R_speed > 127) {
        int32_t diff = abs(R_speed - L_speed);
        R_speed = 127;
        L_speed = 127 - diff;
    }
    if(R_speed < -127) {
        int32_t diff = abs(R_speed - L_speed);
        R_speed = -127;
        L_speed = -127 + diff;
    }

    Serial.printf("X -- adc: %d, sX: %d\n\r", adc_valX, sX);
    Serial.printf("Y -- adc: %d, sY: %d\n\r", adc_valY, sY);
    Serial.printf("L_speed: %d\n\r", L_speed);
    Serial.printf("R_speed: %d\n\r", R_speed);

    SetMotorSpeed(L_speed, R_speed);
    delay(100);
    Serial.printf(CONSOLE_CLEAR);
}
