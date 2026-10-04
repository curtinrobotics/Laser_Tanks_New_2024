// Select Esp32 Dev Module - esp32_bluepad32 for the board when uploading
#include <Bluepad32.h>
#include <Arduino.h>
#include <ESP32Servo.h>

void coreTaskZero(void * pvParameters);
void coreTaskOne(void * pvParameters);

//PINS
const int LASER_Pin = 16;
const int PT1 = 13; //PHOTO TRANSISTOR
const int PT2 = 33;
const int PT3 = 32;
const int SERVO_Pin = 19;
// TA6586 Control pins
// The Motor driver is a Half H-Brdige, with to input pins
// Having one high and the other lower will cause the motor to spin one way
// Switch polarity to change directions
// When both are high the motor will brake **
// When both are Low the motor will coast ** 
const int BI1_Pin = 14; // INPUT FOR DRIVE MOTOR 1
const int FI1_Pin = 27;
const int BI2_Pin = 26; // INPUT FOR DRIVE MOTOR 2
const int FI2_Pin = 25;
const int TRTL_Pin = 18; // INPUT FOR TURRET MOTOR
const int TRTR_Pin = 4;
//const int SDA = 21; // these would be used for a colour sensor
//const int SCL = 22;

// ledc setup values
const int driveFreq = 10000; // frequency for the drive motor and turret
const int laserFreq = 300; // set low so its easier to read
// channels
// not using channels 0 or 1 as the servo libary messes with them though it isnt meant to :(
const int LASER_CH = 2;
const int BI1_CH = 3;
const int FI1_CH = 4;
const int BI2_CH = 5;
const int FI2_CH = 6;
const int TRTL_CH = 7;
const int TRTR_CH = 8;


const int DEADZONE = 100; // Joystick deadzone
const int FIRE_THRESH = 200; // thresh hold for the trigger for firing
const int detectionThresh = 300; // thresh could be 560
const int ServoMin = 0; //this might not be zero but always allow the servo to phyiscally move to its zero position 
const int ServoMax = 70;

//int LASER_DUTY = 1000; //tank 1
//int LASER_DUTY = 700; //tank 2
//int LASER_DUTY = 400; //tank 3
int LASER_DUTY = 100; //tank 4

int update_damage_ticks = 0; 
int detected_duty_cycle = 0;
bool active = 0;
int Tank_1 = 0;
int Tank_2 = 0;
int Tank_3 = 0;
int Tank_4 = 0;
int Health = 20000; // ~3 seconds of being lasered
int Damage = 0;


ControllerPtr myController = nullptr;
Servo barrelServo;


void onConnectedController(ControllerPtr ctl) {
  if (myController == nullptr) {
    Serial.print("CALLBACK: Controller is connected, index=");
    Serial.println(ctl->index());
    // Make sure motors stop when connected
    killMotors();
    active = 1;
    myController = ctl;
  }
}

void onDisconnectedController(ControllerPtr ctl) {
  if (myController == ctl) {
    Serial.print("CALLBACK: Controller is disconnected from index=");
    Serial.println(ctl->index());
    // Stop motors when controller disconnects
    killMotors();
    active = 0;
    myController = nullptr;
  }
}

void setup() {
  Serial.begin(115200);

  BP32.setup(&onConnectedController, &onDisconnectedController);
  BP32.forgetBluetoothKeys();

  // Initialise Servo
  barrelServo.attach(SERVO_Pin);

  //Intitalising ledc
  ledcSetup(LASER_CH, laserFreq, 10); 
  ledcSetup(BI1_CH, driveFreq, 9);
  ledcSetup(FI1_CH, driveFreq, 9);
  ledcSetup(BI2_CH, driveFreq, 9);
  ledcSetup(FI2_CH, driveFreq, 9);
  ledcSetup(TRTL_CH, driveFreq, 10); // 10 bit as the joy stick values are only 9 bit but we dont want to run the motor to fast
  ledcSetup(TRTR_CH, driveFreq, 10);

  ledcAttachPin(LASER_Pin, LASER_CH);
  ledcAttachPin(BI1_Pin, BI1_CH);
  ledcAttachPin(FI1_Pin, FI1_CH);
  ledcAttachPin(BI2_Pin, BI2_CH);
  ledcAttachPin(FI2_Pin, FI2_CH);
  ledcAttachPin(TRTL_Pin, TRTL_CH);
  ledcAttachPin(TRTR_Pin, TRTR_CH);

  killMotors();

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

void killMotors() {
  barrelServo.write(0);
  ledcWrite(LASER_CH, 0);
  ledcWrite(BI1_CH, 0);
  ledcWrite(FI1_CH, 0);
  ledcWrite(BI2_CH, 0);
  ledcWrite(FI2_CH, 0);
  ledcWrite(TRTL_CH, 0);
  ledcWrite(TRTR_CH, 0);
}

int deadZone(int val) {
  if (val < DEADZONE){
    return 0;
  }
  else {
    return constrain(val, -511, 511);
  }
}

void moveMotor(int motorPWM, uint8_t channel_1, uint8_t channel_2) {
  // For PWM >= 0, move forward, otherwise move backwards
  if (motorPWM >= 0)
  {
    ledcWrite(channel_1, motorPWM);
    ledcWrite(channel_2, 0);
  }
  else
  {
    ledcWrite(channel_2, -motorPWM); // makes PWM value positive
    ledcWrite(channel_1, 0);
  }
}

void shootLaser() {
  int throttle = myController->throttle(); // Throttle 0-1023
  if (throttle < FIRE_THRESH) {
    ledcWrite(LASER_CH, LASER_DUTY);
  }
  else {
    ledcWrite(LASER_CH, 0);
  }
}

void loop() {
  // dont use 
}

void healthUpdate() {
  if (update_damage_ticks != 0 && detected_duty_cycle != 0) {
    Damage += update_damage_ticks;
    if (detected_duty_cycle < 250) {
        Tank_4 += update_damage_ticks;
        Serial.print("Tank_4 damage: ");
        Serial.println(Tank_4);
    }
    else if (detected_duty_cycle < 550) {
        Tank_3 += update_damage_ticks;
        Serial.print("Tank_3 damage: ");
        Serial.println(Tank_3);
    }
    else if (detected_duty_cycle < 850) {
        Tank_2 += update_damage_ticks;
        Serial.print("Tank_2 damage: ");
        Serial.println(Tank_2);
    }
    else {
        Tank_1 += update_damage_ticks;
        Serial.print("Tank_1 damage: ");
        Serial.println(Tank_1);
    }
    update_damage_ticks = 0;
    detected_duty_cycle = 0;
  }
  if (Damage > Health) {
    active = 0;
    Serial.println("death");
  }
}

void coreTaskZero(void * pvParameters)
{
  for(;;) {
    BP32.update();
    if (myController && active && myController->isConnected()) {
      // drive motors
      int Xaxis_L = myController->axisX(); // turn
      int Yaxis_L = -myController->axisY(); // throttle
      int leftPWM = deadZone(Yaxis_L + Xaxis_L);
      int rightPWM = deadZone(Yaxis_L - Xaxis_L);
      moveMotor(rightPWM, FI1_CH, BI1_CH);
      moveMotor(leftPWM, FI2_CH, BI2_CH);
      // turret motor
      int TurretJoy = deadZone(myController->axisRX()); //turret turn
      moveMotor(TurretJoy, TRTR_CH, TRTL_CH);
      // turret servo
      int Yaxis_R = deadZone(-myController->axisRY()); //turret servo
      int angle = map(Yaxis_R, -512, 512, ServoMin, ServoMax); // need to adjust servo max and min value so it is horizontal when joystick isnt touched
      barrelServo.write(angle);
      // Laser
      shootLaser();
      healthUpdate();
    }
    else {
      killMotors();
    }
    vTaskDelay(pdMS_TO_TICKS(1)); // this is nessiary 
  }
}



// detection core 
void coreTaskOne(void * pvParameters)
{
  for(;;) {
    Serial.println("core one task running ");// does work with out a statement here
    if (active){
      //hitDetection();
      altDetection();
    }
  }
}

void altDetection() {
  int i = 0; // detection counter
  int j = 0; // total cycles since first detection
  int k = 0; // number of cycle between the first detection and latest detection
  bool condition = 0;
  int cycleThresh = 200; 

  while(1) {
    int read1 = analogRead(PT1);// each ananlog read ads 90 micros to the cycle time with just one taking 80 micro
    //int read2 = analogRead(PT2);// 
    //int read3 = analogRead(PT3);// 

    if (condition){
      j++;// iterate every loop
    }
    if (read1 >= detectionThresh) {//(read1 >= detectionThresh || read2 >= detectionThresh || read3 >= detectionThresh)
      i++; // iterates only when a hit is detected
      if (i == 1) {
        j = 1; // this ensures that once a hit is detected both i and j start from the same iteration
        condition = 1;
      }
      k = j; 
    }
    if (j - k > cycleThresh || k > Health) { // might need to add an or statement to this say that if the total number of damage ticks is greater then the remaining health stop counting but this might slow down the code
      update_damage_ticks = k;
      //Serial.print("damage: ");
      //Serial.println(update_damage_ticks);
      detected_duty_cycle = 1023*i/k; //this is to avoid floating point divsion which is very slow
      i = 0;
      j = 0;
      k = 0;
      condition = 0;
    }

  }
}

void hitDetection() {
  int i = 0;
  int j = 0;
  int avgI = 0;
  const int noHitDelay = 50; //how long after a pwm pulse has been detected before the we decide the tank is no longer being hit we could make this number small i think and it would work better


  int doPrint = 0;

  int currTime = 0, prevTime = 0;
  for(;;) {  // Read the phototransistor value and set the read time
    int read1 = analogRead(PT1);
    int read2 = analogRead(PT2);
    int read3 = analogRead(PT3);
    currTime = millis();

    // Increment i if the read value is above the threshold
    if (read1 || read2 || read3 >= detectionThresh)
    {
      i++;
    }
    
    // If a hit has been detected and it has been more than 50ms since the previous PWM calculation. might be able to make this time should but is it need?
    if (i > 0 && currTime - prevTime >= noHitDelay)
    {
      // Duty cycle equivalent -> averages number of hits per time period
      uint32_t dc = i / (currTime - prevTime - noHitDelay);

      printf("Ambient read: %d & thresh: %d & i: %d\t", read, detectionThresh, i);
      printf("DC equiv.: %d\n", dc);

      // Set number of hits back to 0 and update the previous PWM calculation time
      i = 0;
      prevTime = millis();
    }
  }
}
