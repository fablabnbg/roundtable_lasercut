// http://www.airspayce.com/mikem/arduino/AccelStepper/classAccelStepper.html
//
// lsusb | grep USBtiny
//  Bus 002 Device 014: ID 1781:0c9f Multiple Vendors USBtiny
// sudo chmod a+rw /dev/bus/usb/002/014
// Arduino Nano -> Hochladen mit Programmer
// 
// 180:14 main ratio, 512 steps per rotation. 4x half stepping.
// -> 26331.42857 steps per rotation... -> 73.142857 steps per degree
//
// FIXME: use 180:16 for 64 steps per degree.
//
// Halfstepping with 1.2A seems to be fine.

#include <AccelStepper.h>

#define ULN2003 0   // 1: 4pin ULN2003, 0: STEP/DIR driver

#define BUTTON_PIN 11
#define LED_PIN1   12
#define LED_PIN2   13

// Motor pin definitions
#define motorPin1 2 // IN1 on the ULN2003 driver 1   // Driver: STEP
#define motorPin2 3 // IN2 on the ULN2003 driver 1   // Driver: DIRECTION
#define motorPin3 4 // IN3 on the ULN2003 driver 1
#define motorPin4 5 // IN4 on the ULN2003 driver 1

// Initialize with pin sequence IN1-IN3-IN2-IN4 for using the AccelStepper with 28BYJ-48
#if ULN2003
AccelStepper stepper(AccelStepper::FULL4WIRE, motorPin1, motorPin3, motorPin2, motorPin4);
# define STEPS_BACKLASH 60    // TODO: to be added when reversing direction.
# define STEPS_PER_360 (180./14.*512*4)
# define MAX_SPEED 800      // default 600, too much 1000
# define ACCELERATION 1500
# define DEMO_DEFAULT 1
#else
AccelStepper stepper(AccelStepper::DRIVER, motorPin1, motorPin2);
# define SUBSTEPS 2
# define STEPS_BACKLASH (600*SUBSTEPS)    // TODO: to be added when reversing direction.
# define STEPS_PER_360 (89590*SUBSTEPS)
# define MAX_SPEED (1900*SUBSTEPS)     // 2000 is a horrible feep, 1980 is good
# define ACCELERATION (1000*SUBSTEPS)
# define DEMO_DEFAULT 0
#endif

// #define STEPS_PER_360 26331
long Positions[] = { (long)(0.125*STEPS_PER_360), 0, 1000, 0, (long)(-0.25*STEPS_PER_360), 0, (long)STEPS_PER_360, 0, 1000, 0 };
uint8_t actPos = 0;
unsigned char startFirst = true;
uint8_t demo = DEMO_DEFAULT;         // run the Positions in demoMode

uint8_t serInBufLen = 0; // nr of chars in buffer
uint8_t serInBuf[32];
uint8_t serOutBufLen = 0; // nr of chars in buffer
uint8_t serOutBufPos = 0; // nr of chars already written. Remaining is serOutBufLen-serOutBufPos.
uint8_t serOutBuf[32];

void setup()
{
  pinMode(LED_PIN1, OUTPUT);
  pinMode(LED_PIN2, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  stepper.setMaxSpeed(MAX_SPEED);
  stepper.setAcceleration(ACCELERATION);
  Serial.begin(9600);
  while (!Serial) 
    ; // wait for serial port to connect. Needed for native USB port
}

void serCmd(uint8_t *buf, uint8_t len)
{
  // simple echo back server.
  serOutBufPos = 0;
  serOutBufLen = 0;
  buf[len] = '\0';
  if (len) 
    {
      if (buf[0] == 'c')  // query current position
        serOutBufLen = snprintf((char *)serOutBuf, sizeof(serOutBuf), "currentPosition = %d\r\n", stepper.currentPosition());
      else if (buf[0] == 'd')  // demo mode
        {
          demo = 1;
          serOutBufLen = snprintf((char *)serOutBuf, sizeof(serOutBuf), "demo mode = %d\r\n", demo);
        }
      else if (buf[0] == 's')  // stop
        {
          demo = 0;
          stepper.stop();
          serOutBufLen = snprintf((char *)serOutBuf, sizeof(serOutBuf), "stopped.\r\n", demo);
        }
      else if (buf[0] == 'g')  // go in degrees
        {
          long deg;
          long steps;
          demo = 0;
          while (*buf != ' ' && len > 1) { buf++; len--; }
          while (*buf == ' ' && len > 1) { buf++; len--; }
          deg = (double)strtol((char *)buf, NULL, 10);
          steps = (long)(deg*STEPS_PER_360/360.);
          stepper.move(steps);
          serOutBufLen = snprintf((char *)serOutBuf, sizeof(serOutBuf), "move = %ld steps, %ld deg.\r\n", steps, deg);          
        }
      else if (buf[0] == 'm')  // move steps
        {
          long steps;
          demo = 0;
          while (*buf != ' ' && len > 1) { buf++; len--; }
          while (*buf == ' ' && len > 1) { buf++; len--; }
          steps = strtol((char *)buf, NULL, 10);
          stepper.move(steps);
          serOutBufLen = snprintf((char *)serOutBuf, sizeof(serOutBuf), "move = %d.\r\n", steps);
        }
      else
        serOutBufLen = snprintf((char *)serOutBuf, sizeof(serOutBuf), "%d: You sent %d bytes\r\n", buf[0], len);
      if (serOutBufLen > sizeof(serOutBuf)) 
        serOutBufLen = sizeof(serOutBuf);
    }
}

long button_counter = 0;

void loop()
{
  if (digitalRead(11))
    {
      digitalWrite(LED_PIN2, HIGH);
      digitalWrite(LED_PIN1, LOW);

      if (button_counter > 100000)
        {
          demo = 1 - demo;
        }
      else if (button_counter > 0)
        { 
          if (demo == 1)
            serCmd((uint8_t*)"stop", 4);
          else
            serCmd((uint8_t *)"g 30", 4); // go 15 deg forward.
          demo = 0;
        }
      button_counter = 0;
    }
  else
    {
      digitalWrite(LED_PIN2, LOW);
      digitalWrite(LED_PIN1, HIGH);
      button_counter++;
      if (button_counter > 100000)
        {
          digitalWrite(LED_PIN2, HIGH);
        }
    }
    
  if( demo && startFirst ) {
    actPos = 0;
    stepper.moveTo( Positions[actPos] );
    startFirst = false;
  }
  else {
    if( demo && stepper.distanceToGo() == 0 ) {
      actPos++;
      if( actPos < sizeof(Positions)/sizeof(long) ) {
        stepper.moveTo( Positions[actPos] );
      }
    }
  }
  stepper.run();

  // Either input or output a byte. Output has priority.
  if (serOutBufPos < serOutBufLen && Serial.availableForWrite() > 0)
    {
      Serial.write(serOutBuf[serOutBufPos++]);
      if (serOutBufPos >= serOutBufLen)
        {
          serOutBufLen = 0;
          serOutBufPos = 0;
        }
    }
  else if (Serial.available() > 0)
    {
      uint8_t inByte = Serial.read();
      if (inByte == '\r' || inByte == '\n')
        {
          // Complete command seen. Execute.
          serCmd(serInBuf, serInBufLen);
          serInBufLen=0;
        }
      else if (serInBufLen < sizeof(serInBuf)-1)
        {
          serInBuf[serInBufLen++] = inByte;
        }
    }
}

