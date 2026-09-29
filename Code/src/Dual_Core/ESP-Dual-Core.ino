// Select Esp32 Dev Module - esp32_bluepad32 for the board when uploading
#include <Bluepad32.h>
#include <Arduino.h>
#include <ESP32Servo.h>

void coreTaskZero(void * pvParameters);
void coreTaskOne(void * pvParameters);

uint8_t LASER1 = 16;
uint8_t LASER2 = 17;
uint8_t PHOTO = 15; //Phototransistor
uint8_t BARREL = 19;
uint8_t THRESHPIN = 35;
uint8_t JS_DEADZONE = 50; // Joystick deadzone
uint8_t RJS_DEADZONE = 50; // Right Joystick deadzone (turret joystick)
uint16_t LASER_THRESH = 200;

// thresh could be 560
int detectionThresh = 300;

ControllerPtr myController = nullptr;
Servo barrelServo;

// TA6586 Control pins
// The Motor driver is a Half H-Brdige, with to input pins
// Having one high and the other lower will cause the motor to spin one way
// Switch polarity to change directions
// When both are high the motor will brake **
// When both are Low the motor will coast ** 
const int BI_L = 14;
const int FI_L = 27;
const int BI_R = 26;
const int FI_R = 25;

// Turret Left and Right pins
const int TI_L = 4;
const int TI_R = 18;

//Intitalising values
int leftPWM = 0;
int rightPWM = 0;
int throttle = 0;
int turn = 0;

void onConnectedController(ControllerPtr ctl) {
  if (myController == nullptr) {
    Serial.print("CALLBACK: Controller is connected, index=");
    Serial.println(ctl->index());
    // Make sure motors stop when connected
    digitalWrite(FI_L, 0);
    digitalWrite(FI_R, 0);
    digitalWrite(BI_L, 0);
    digitalWrite(BI_R, 0);
    myController = ctl;
  }
}

void onDisconnectedController(ControllerPtr ctl) {
  if (myController == ctl) {
    Serial.print("CALLBACK: Controller is disconnected from index=");
    Serial.println(ctl->index());
    // Stop motors when controller disconnects
    digitalWrite(FI_L, 0);
    digitalWrite(FI_R, 0);
    digitalWrite(BI_L, 0);
    digitalWrite(BI_R, 0);
    myController = nullptr;
  }
}

void setup()
{
  Serial.begin(115200);

  BP32.setup(&onConnectedController, &onDisconnectedController);
  BP32.forgetBluetoothKeys();

  // Initialise Servo
  barrelServo.attach(BARREL);

  //Intitalising Pins
  pinMode(FI_L, OUTPUT);
  pinMode(FI_R, OUTPUT);
  pinMode(BI_L, OUTPUT);
  pinMode(BI_R, OUTPUT);

  // put your setup code here, to run once:
  xTaskCreatePinnedToCore(
    coreTaskZero,
    "Main Loop",
    10000,
    NULL,
    2,
    NULL,
    0
  );

  xTaskCreatePinnedToCore(
    coreTaskOne,
    "Laser",
    10000,
    NULL,
    2,
    NULL,
    1
  );
}

inline bool controllerAvailable()
{
  if (myController && myController->isConnected())
  {
    return true;
  }

  return false;
}

inline uint8_t isInDeadzone(int pwmVal)
{
  return abs(pwmVal) <= JS_DEADZONE;
}

void moveMotor(int motorPWM, uint8_t frontPin, uint8_t backPin)
{
  // If in deadzone, set both pins to low
  if (isInDeadzone(motorPWM))
  {
    analogWrite(frontPin, 0);
    analogWrite(backPin, 0);
    return;
  }

  // For PWM >= 0, move forward, otherwise move backwards
  if (motorPWM >= 0)
  {
    analogWrite(frontPin, motorPWM);
    analogWrite(backPin, 0); // using digitalWrite(Pin, LOW) here causes problems for some reason so keep it as an analogWrite(Pin, 0)
  }
  else
  {
    analogWrite(backPin, -motorPWM); // makes PWM value positive
    analogWrite(frontPin, 0);
  }
}

void shootLaser(uint8_t laser_pin, uint8_t value)
{
  // Throttle 0-1023
  if (!controllerAvailable())
  {
    return;
  }
  
  uint16_t throttle = myController->throttle();

  // printf("Throttle value is: %d\n", throttle);

  if (throttle > LASER_THRESH)
  {
    analogWrite(laser_pin, value);
  }
  else
  {
    // HIGH turns the laser off for PNP transistor
    analogWrite(laser_pin, 255);
    //digitalWrite(laser_pin, HIGH);
  }
}

void driveLoop()
{
  // Check if the controller exists and is connected
  // No longer uses myControllerhasUpdate as updates were not being sent often enough and readings were off
  if (!controllerAvailable())
  {
    moveMotor(0, FI_R, BI_R);
    moveMotor(0, FI_L, BI_L);
    return;
  }

  // Converts the axis signed 9 bit (512) reading into a signed 8 bit(256) reading
  turn = myController->axisX() >> 1;
  throttle = -myController->axisY() >> 1; // Top is negtive for Y-axis, so it need to be inverted
  
  // Converting controller readings to PWM values for the tank steer using two motors
  // Serial.println(turn);
  // Serial.println(throttle);
  leftPWM = throttle + turn;
  rightPWM = throttle - turn;
  leftPWM = constrain(leftPWM, -255, 255);
  rightPWM = constrain(rightPWM, -255, 255);

  moveMotor(rightPWM, FI_R, BI_R);
  moveMotor(leftPWM, FI_L, BI_L);
}

void moveBarrelServo()
{
  if (!controllerAvailable())
  {
    return;
  }

  int16_t rawAxisY = myController->axisRY();
  uint8_t min = 0, max = 70;
  uint8_t zeroPos = (max - min) / 2;

  // Up is -512 & Down is 511
  printf("Up down is: %d\n", rawAxisY);

  // If in the deadzone, return to zero / home position
  if (abs(rawAxisY) < RJS_DEADZONE / 2)
  {
    // printf("Moving servo to zero pos\n");

    barrelServo.write(zeroPos);
  }
  else
  {
    // Max and min turret angles
    uint8_t servoPos = (rawAxisY + 512) * (max - min) / 1023;

    servoPos = constrain(servoPos, min, max);

    // printf("Moving servo to pos: %d\n", servoPos);
    barrelServo.write(servoPos);
  }
}

void moveTurretServo()
{
  if (!controllerAvailable())
  {
    return;
  }

  // Gets the raw X axis values
  int16_t rawAxisX = myController->axisRX();

  int16_t max = 128, min = -128;
  
  
  int16_t turnPWM = (rawAxisX + 512) * (max - min) / 1023 + min;

  // Constrain max and min to ensure PWM does not go over them
  turnPWM = constrain(turnPWM, min, max);

  moveMotor(turnPWM, TI_L, TI_R);
}

void loop()
{
  // put your main code here, to run repeatedly:
  // int resVal = analogRead(4);
  // printf("Potentiometer Val: %d\n", resVal);
}

void coreTaskZero(void * pvParameters)
{
  pinMode(LASER1, OUTPUT);
  pinMode(LASER2, OUTPUT);

  while(1)
  {
    BP32.update();

    // Write PWM signals to Laser pins
    shootLaser(LASER1, 100);
    // analogWrite(LASER1,0);
    // digitalWrite(LASER1, 255);
    // analogWrite(LASER1,100);
    analogWrite(LASER2,200);
    
    driveLoop();
    moveBarrelServo();
    moveTurretServo();

    // detectionThresh = analogRead(THRESHPIN);
    
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void coreTaskOne(void * pvParameters)
{
  int i = 0;
  int j = 0;
  int avgI = 0;

  // int thresh = 15;
  // int thresh = 2000;
  // int thresh = 300;
  
  unsigned long lastPrint = 0;

  int doPrint = 0;

  int currTime = 0, prevTime = 0;

  uint16_t maxRead = 0;

  while(1)
  {
    // Read the phototransistor value and set the read time
    int read = analogRead(PHOTO);
    currTime = millis();

    // If the current read is the max thus far, update the max
    if (read > maxRead)
    {
      maxRead = read;
    }

    // Increment i if the read value is above the threshold
    if (read >= detectionThresh)
    {
      i++;
    }
    
    // If a hit has been detected and it has been more than 100ms since the previous PWM calculation.
    if (i > 0 && currTime - prevTime >= 100)
    {
      // Duty cycle equivalent -> averages number of hits per time period
      uint32_t dc = i / (currTime - prevTime);

      printf("Ambient read: %d & thresh: %d & i: %d & maxRead: %d\t", read, detectionThresh, i, maxRead);
      printf("DC equiv.: %d\n", dc);

      // Set number of hits back to 0 and update the previous PWM calculation time
      i = 0;
      prevTime = millis();
      maxRead = 0;
    }
  }
}
