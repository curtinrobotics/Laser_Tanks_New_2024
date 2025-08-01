#include "tank_comms.h"
#include <cmath>



#define TANK_ADDRESS {0x80, 0x7D, 0x3A, 0x23, 0x7F, 0x58}

uint8_t target_address[6];

void TankCommsInit(uint8_t* tank_address) {

    memcpy(target_address, tank_address, 6);


    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    esp_now_init();

    esp_now_peer_info_t info;
    info.channel = 0;
    info.encrypt = false;
    memcpy(info.peer_addr, target_address, 6);
    esp_now_add_peer(&info);


    // Serial.println(target_address);

}

//left_speed: [-127, 127] (Fastest going backwards, Fastest going forwards)
//right_speed: [-127, 127] (Fastest going backwards, Fastest going forwards)
void SetMotorSpeed(int8_t left_speed, int8_t right_speed) {

    // uint8_t absolute_left_speed = abs(left_speed);
    // uint8_t absolute_right_speed = abs(right_speed);
    //
    // uint8_t left_direction = signbit(left_speed);
    // uint8_t right_direction = signbit(right_speed);

    uint8_t left_speed_argument  = left_speed;
    uint8_t right_speed_argument = right_speed;

    // uint8_t left_speed_argument = (absolute_left_speed >> 1) + 0x40 * left_direction;
    // uint8_t right_speed_argument = (absolute_right_speed >> 1) + 0x40 * right_direction;


    uint8_t left_motor_command[] = {0x01, left_speed_argument};
    uint8_t right_motor_command[] = {0x02, right_speed_argument};

    esp_now_send(target_address, left_motor_command, 2);
    esp_now_send(target_address, right_motor_command, 2);

}
