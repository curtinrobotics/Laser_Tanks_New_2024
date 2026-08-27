#include <Arduino.h>

void coreTaskZero(void * pvParameters);
void coreTaskOne(void * pvParameters);

uint8_t LASER1 = 18;
uint8_t LASER2 = 17;
uint8_t PHOTO = 15; //Phototransistor
uint8_t THRESHPIN = 35;

// thresh could be 560
int detectionThresh = 300;

void setup()
{
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
    analogWrite(LASER1,100);
    analogWrite(LASER2,200);
    
    // for (int i = 0; i < 1000; i++)
    // {
    //   digitalWrite(18, HIGH);
    //   delayMicroseconds(i);
    //   digitalWrite(18, LOW);
    //   delayMicroseconds(1000 - i);
    // }

    detectionThresh = analogRead(THRESHPIN);
    
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void coreTaskOne(void * pvParameters)
{
  int i = 0;
  int j = 0;
  int avgI = 0;

  // int prevRead = 0;
  // int currRead = 0;

  // int thresh = 15;
  // int thresh = 2000;
  // int thresh = 300;
  // unsigned long startTime = 0, endTime = 0;
  unsigned long lastPrint = 0;

  int doPrint = 0;

  int currTime = 0, prevTime = 0;

  uint16_t maxRead = 0;

  while(1)
  {
    // printf("Detection Threshold is: %d\n", detectionThresh);

    // int res1 = analogRead(15);
    // int res2 = analogRead(2);
    // int res3 = analogRead(4);

    //printf("ADC 15: %d, 2: %d, 4: %d\n", res1, res2, res3);

    // if (res1 > 15 || res2 > 15 || res3 > 15)
    // {
    //   i++;
    // }

    // if (res1 > 15)
    // {
    //   i++;
    // }

    // if (i >= 10000)
    // {
    //   printf("Counter reached 10000!\n");
    //   i = 0;
    // }



    // currRead = analogRead(15);

    // if (currRead >= thresh && prevRead < thresh)
    // {
    //   startTime = micros();
    // }

    // if (prevRead >= thresh && currRead < thresh)
    // {
    //   endTime = micros();

    //   if (doPrint)
    //     printf("Duration is: %dus\n", endTime - startTime);

    //   doPrint = 0;
    // }

    // if (micros() - startTime > 100000)
    // {
    //   doPrint = 1;
    // }

    // prevRead = currRead;

    int read = analogRead(PHOTO);
    currTime = millis();

    // printf("Read value is: %d\n", read);

    if (read > maxRead)
    {
      maxRead = read;
    }

    if (read >= detectionThresh)
    {
      i++;
    }
    // else
    // {
    //   // j++;

    //   if (i != 0)
    //   {
    //     avgI += i;
    //     j++;
        
    //     if (millis() - lastPrint >= 100)
    //     {
    //       // printf("Ambient: %d\t", read);
    //       printf("Number of iter: %d\n", avgI / j);
    //       lastPrint = millis();
          
    //       avgI = 0;
    //       j = 0;
    //     }
    //     // doPrint = 0;
    //   }

    //   // if (j >= 1000)
    //   // {
    //   //   doPrint = 1;
    //   //   j = 0;
    //   // }

    //   i = 0;
    // }

    if (i > 0 && currTime - prevTime >= 100)
    {
      // Duty cycle equivalent
      uint32_t dc = i / (currTime - prevTime);

      printf("Ambient read: %d & thresh: %d & i: %d & maxRead: %d\t", read, detectionThresh, i, maxRead);
      printf("DC equiv.: %d\n", dc);

      i = 0;
      prevTime = millis();
      maxRead = 0;
    }
  }
}
