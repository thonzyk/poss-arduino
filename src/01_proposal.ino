#include <Arduino.h>
#include "MeAuriga.h"
#include "MeRGBLineFollower.h"

// Levý motor
const int pwmMotorPravy = 11;
const int inMotorPravy1 = 49;
const int inMotorPravy2 = 48;

// Pravý motor
const int pwmMotorLevy = 10;
const int inMotorLevy1 = 47;
const int inMotorLevy2 = 46;

int rychlostJizdy = 200;
int minRychlost = 100;
int maxRychlost = 255;

// Ultrazvukovy snimac
// pouziti: vzdalenost = sonar.distanceCm()
MeUltrasonicSensor sonar(PORT_10);

// Snimac cary
// pouziti: linState = RGBLineFollower.getPositionState();
//          lineOffset = RGBLineFollower.getPositionOffset();
MeRGBLineFollower RGBLineFollower(PORT_9);

// Servo
const byte servoPin = 68;
const byte servoMin = 13;
const byte servoMax = 137;
Servo servo;

// narazniky
const byte pravyNaraznik = 67;
const byte levyNaraznik = 62;
volatile boolean narazVpravo = false;
volatile boolean narazVlevo = false;

// promenne pro enkodery
// enkoder pro pravy motor
const byte pravyEnkoderA = 19; // INT2
const byte pravyEnkoderB = 42; // no interrupts :(

// enkoder pro levy motor
const byte levyEnkoderA = 18; // INT3
const byte levyEnkoderB = 43; // no interrupts :(

const int eckoderPulseNumber = 9;
const float motorGearRatio = 39.267;

volatile long pulseCountVlevo = 0;
volatile long pulseCountVpravo = 0;

// RGB LED ring
const byte numberOfLEDs = 12;
const byte rgbLEDringPin = 44;
#define RINGALLLEDS 0
MeRGBLed ledRing(0, numberOfLEDs);

#define amber 255, 194, 000
#define orange 255, 165, 000
#define vermillion 227, 066, 052
#define red 255, 000, 000
#define magenta 255, 000, 255
#define purple 128, 000, 128
#define indigo 075, 000, 130
#define blue 000, 000, 255
#define aquamarine 127, 255, 212
#define green 000, 255, 000
#define chartreuse 127, 255, 000
#define yellow 255, 255, 000
#define white 000, 000, 000
#define black 255, 255, 255

// bzučák
const byte buzzerPin = 45;
MeBuzzer buzzer;

// Gyro
MeGyro gyro(1, 0x69);

// States
volatile byte STATE = 0;
const byte WAIT = 0;
const byte START_MAPPING = 1;
const byte START_SPEEDRUN = 2;
const byte WALK_THE_LINE = 3;
const byte STOP = 4;
const byte MAPPING_DECISION = 5;
const byte SPEEDRUN_DECISION = 6;
const byte TURN = 7;
const byte FINISH_MAPPING = 8;
const byte FINISH_SPEEDRUN = 9;

// Modes
volatile byte MODE = 0;
const byte NULL_MODE = 0;
const byte MAPPING = 1;
const byte SPEEDRUN = 2;

// Errors
const byte UNSUPPORTED_MODE = 1;

void setup()
{
  // nastav piny narazniku
  pinMode(pravyNaraznik, INPUT_PULLUP);
  pinMode(levyNaraznik, INPUT_PULLUP);

  // nastavení ovladacích pinů motorů jako výstupní
  pinMode(pwmMotorPravy, OUTPUT);
  pinMode(inMotorPravy1, OUTPUT);
  pinMode(inMotorPravy2, OUTPUT);

  pinMode(pwmMotorLevy, OUTPUT);
  pinMode(inMotorLevy1, OUTPUT);
  pinMode(inMotorLevy2, OUTPUT);

  // Nastavení frekvencep pwm na 8KHz pro řízení DC motorů
  TCCR1A = _BV(WGM10);
  TCCR1B = _BV(CS11) | _BV(WGM12);

  TCCR2A = _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(CS21);

  // inicializace enkoderu
  pinMode(pravyEnkoderA, INPUT_PULLUP);
  pinMode(pravyEnkoderB, INPUT_PULLUP);
  pinMode(levyEnkoderA, INPUT_PULLUP);
  pinMode(levyEnkoderB, INPUT_PULLUP);

  // inicializace obsluhy preruseni od kanalů A enkoderů
  attachInterrupt(digitalPinToInterrupt(pravyEnkoderA), &pravyEncoderAInt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(levyEnkoderA), &levyEncoderAInt, CHANGE);

  // pripoj a omez servo
  servo.attach(servoPin); //,servoMin,servoMax);
  servo.write(90);

  // inicializace RGB LED ringu
  // pro ovládání slouží metoda
  // bool MeRGBLed::setColor(uint8_t index, uint8_t red, uint8_t green, uint8_t blue)
  ledRing.setpin(rgbLEDringPin);
  ledRing.setColor(RINGALLLEDS, 0, 0, 0);
  ledRing.show();

  // nastavení bzučáku
  buzzer.setpin(buzzerPin);
  buzzer.noTone();

  // inicializace gyra
  // gyro.begin();

  // inicializace sledovani cary
  RGBLineFollower.begin();
  RGBLineFollower.setKp(1);

  // inicializace sériového kanálu
  Serial.begin(9600);

  while (digitalRead(levyNaraznik))
  {
    // nepokracuj dokud neni stiknut levy naraznik
  }
}

/**
 * @brief
 */
void loop()
{
  while (true)
  {
    stateTransition();
  }
}

// osetreni preruseni od kanalu A enkoderu na pravem motoru
void pravyEncoderAInt()
{
}

// osetreni preruseni od kanalu A enkoderu na levem motoru
void levyEncoderAInt()
{
}

void levyMotorVpred(int rychlost)
{
  digitalWrite(inMotorLevy1, HIGH);
  digitalWrite(inMotorLevy2, LOW);
  analogWrite(pwmMotorLevy, rychlost);
}

void levyMotorVzad(int rychlost)
{
  digitalWrite(inMotorLevy1, LOW);
  digitalWrite(inMotorLevy2, HIGH);
  analogWrite(pwmMotorLevy, rychlost);
}

void levyMotorStop()
{
  analogWrite(pwmMotorLevy, 0);
}

void pravyMotorVpred(int rychlost)
{
  digitalWrite(inMotorPravy1, LOW);
  digitalWrite(inMotorPravy2, HIGH);
  analogWrite(pwmMotorPravy, rychlost);
}

void pravyMotorVzad(int rychlost)
{
  digitalWrite(inMotorPravy1, HIGH);
  digitalWrite(inMotorPravy2, LOW);
  analogWrite(pwmMotorPravy, rychlost);
}

void pravyMotorStop()
{
  analogWrite(pwmMotorPravy, 0);
}

/**
 * @brief
 */
void stateTransition()
{
  if (STATE == WAIT)
  {
    wait();
  }
  else if (STATE == START_MAPPING)
  {
    startMapping();
  }
  else if (STATE == START_SPEEDRUN)
  {
    startSpeedrun();
  }
  else if (STATE == WALK_THE_LINE)
  {
    walkTheLine();
  }
  else if (STATE == STOP)
  {
    stop();
  }
  else if (STATE == MAPPING_DECISION)
  {
    mappingDecision();
  }
  else if (STATE == SPEEDRUN_DECISION)
  {
    speedrunDecision();
  }
  else if (STATE == TURN)
  {
    turn();
  }
  else if (STATE == FINISH_MAPPING)
  {
    finishMapping();
  }
  else if (STATE == FINISH_SPEEDRUN)
  {
    finishSpeedrun();
  }
}

/**
 * @brief
 */
void wait()
{
  while (STATE == WAIT)
  {
    if (digitalRead(levyNaraznik))
    {
      STATE = START_MAPPING;
    }
    else if (digitalRead(pravyNaraznik))
    {
      STATE = START_SPEEDRUN;
    }
  }
}

/**
 * @brief
 */
void startMapping()
{
  MODE = MAPPING;
  STATE = WALK_THE_LINE;
}

/**
 * @brief
 */
void startSpeedrun()
{
  MODE = SPEEDRUN;
  STATE = WALK_THE_LINE;
}

/**
 * @brief
 */
void walkTheLine()
{
  while (isOnTheLine())
  {
    controlTheLine();
  }
  STATE = STOP;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
bool isOnTheLine()
{
  // TODO: implement!!!
  return true;
}

/**
 * @brief
 *
 */
void controlTheLine()
{
  // TODO: implement
}

/**
 * @brief
 */
void stop()
{
  if (MODE == MAPPING)
  {
    STATE = MAPPING_DECISION;
  }
  else if (MODE == SPEEDRUN)
  {
    STATE = SPEEDRUN_DECISION;
  }
  else
  {
    fallToError(UNSUPPORTED_MODE);
  }
}

/**
 * @brief
 */
void mappingDecision()
{
  // TODO: implement

  if (isFinishedMapping())
  {
    STATE = FINISH_MAPPING;
  }
  else
  {
    STATE = TURN;
  }
}

/**
 * @brief
 */
void speedrunDecision()
{
  // TODO: implement

  if (isFinishedSpeedrun())
  {
    STATE = FINISH_SPEEDRUN;
  }
  else
  {
    STATE = TURN;
  }
}

/**
 * @brief
 */
void turn()
{
  // TODO: implement
  STATE = STOP;
}

/**
 * @brief
 */
void finishMapping()
{
  // TODO: implement

  STATE = WAIT;
}

/**
 * @brief
 */
void finishSpeedrun()
{
  // TODO: implement

  STATE = WAIT;
}

void fallToError(int errorCode)
{

  while (true)
  {
    if (errorCode == UNSUPPORTED_MODE)
    {
      // TODO: implement
    }
    // TODO: implement
    delay(300);
  }
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
bool isFinishedMapping()
{
  // TODO: implement
  return true;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
bool isFinishedSpeedrun()
{
  // TODO: implement
  return true;
}
