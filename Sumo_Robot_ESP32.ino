#include <Arduino.h>
#include <Wire.h>
#include <VL53L0X.h>

const uint8_t PIN_BTN_START = 4;

const uint8_t PIN_AIN1 = 25; const uint8_t PIN_AIN2 = 26; const uint8_t PIN_PWMA = 27;
const uint8_t PIN_BIN1 = 14; const uint8_t PIN_BIN2 = 12; const uint8_t PIN_PWMB = 13;
const uint8_t PIN_STBY = 33;

const uint8_t PIN_LINE = 32;      
const uint8_t PIN_US_TRIG = 5;    
const uint8_t PIN_US_ECHO = 18;
const uint8_t I2C_SDA = 21;      
const uint8_t I2C_SCL = 22;

const int TOF_KEEP_MM = 600;
const int US_KEEP_CM = 60;

volatile bool emergencyLineTriggered = false;
volatile unsigned long lastLineTrigger = 0;

void IRAM_ATTR onLineDetected() {
  if (millis() - lastLineTrigger > 10) {
    emergencyLineTriggered = true;
    lastLineTrigger = millis();
  }
}

enum TargetState { 
  TARGET_NONE,   
  TARGET_EDGE,   
  TARGET_LOCKED  
};

class SumoRobot {
private:
  enum State { 
    IDLE_WAIT_BUTTON, 
    COUNTDOWN_5S,
    MATADOR_DODGE,  
    SEARCH_SCAN,    
    TRACKING,       
    ATTACK,         
    LINE_REVERSE, 
    LINE_TURN, 
    FAULT 
  };
  
  State state;
  unsigned long stateStartedAt;
  
  VL53L0X tof;
  bool tofReady;
  bool pwmReady;
  bool searchTurnRight;
  unsigned long lastTargetSeenAt;

  void drive(int leftSpeed, int rightSpeed) {
    if (!pwmReady) return;
    digitalWrite(PIN_STBY, HIGH); 
    
    digitalWrite(PIN_AIN1, leftSpeed >= 0 ? HIGH : LOW);
    digitalWrite(PIN_AIN2, leftSpeed >= 0 ? LOW : HIGH);
    ledcWrite(PIN_PWMA, abs(leftSpeed));

    digitalWrite(PIN_BIN1, rightSpeed >= 0 ? HIGH : LOW);
    digitalWrite(PIN_BIN2, rightSpeed >= 0 ? LOW : HIGH);
    ledcWrite(PIN_PWMB, abs(rightSpeed));
  }

  void stopMotors() {
    digitalWrite(PIN_STBY, LOW);
    ledcWrite(PIN_PWMA, 0); ledcWrite(PIN_PWMB, 0);
  }

  TargetState scanTarget() {
    bool tofLocked = false;
    bool usLocked = false;

    if (tofReady) {
      uint16_t distance = tof.readRangeContinuousMillimeters();
      if (!tof.timeoutOccurred() && distance < TOF_KEEP_MM) tofLocked = true;
    } 

    digitalWrite(PIN_US_TRIG, LOW); delayMicroseconds(2);
    digitalWrite(PIN_US_TRIG, HIGH); delayMicroseconds(10);
    digitalWrite(PIN_US_TRIG, LOW);
    long duration = pulseIn(PIN_US_ECHO, HIGH, 5000); 
    if (duration > 0 && (duration * 0.034 / 2) < US_KEEP_CM) usLocked = true;

    if (tofLocked) return TARGET_LOCKED; 
    if (usLocked) return TARGET_EDGE;    
    return TARGET_NONE;                  
  }

  void enterState(State newState) {
    if (state == newState) return;
    state = newState;
    stateStartedAt = millis();

    switch (state) {
      case IDLE_WAIT_BUTTON: stopMotors(); break;
      case COUNTDOWN_5S: stopMotors(); break;
      case MATADOR_DODGE: 
        drive(255, 80); 
        break;
      case SEARCH_SCAN: emergencyLineTriggered = false; break;
      case TRACKING: break;
      case ATTACK: break;
      case LINE_REVERSE: searchTurnRight = !searchTurnRight; drive(-255, -255); break;
      case LINE_TURN: 
        if (searchTurnRight) drive(200, -200); else drive(-200, 200); 
        break;
      case FAULT: stopMotors(); break;
    }
  }

public:
  SumoRobot() : state(FAULT), tofReady(false), pwmReady(false), searchTurnRight(true) {}

  void begin() {
    pinMode(PIN_BTN_START, INPUT_PULLUP);
    pinMode(PIN_AIN1, OUTPUT); pinMode(PIN_AIN2, OUTPUT);
    pinMode(PIN_BIN1, OUTPUT); pinMode(PIN_BIN2, OUTPUT); pinMode(PIN_STBY, OUTPUT);
    
    pwmReady = ledcAttach(PIN_PWMA, 20000, 8) && ledcAttach(PIN_PWMB, 20000, 8);
    stopMotors();

    pinMode(PIN_US_TRIG, OUTPUT); pinMode(PIN_US_ECHO, INPUT); digitalWrite(PIN_US_TRIG, LOW);
    pinMode(PIN_LINE, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_LINE), onLineDetected, FALLING); 

    Wire.begin(I2C_SDA, I2C_SCL);
    tof.setTimeout(8);
    tofReady = tof.init();
    if (tofReady) tof.startContinuous(25);

    if (!pwmReady) enterState(FAULT);
    else enterState(IDLE_WAIT_BUTTON);
  }

  void update() {
    if (state == FAULT) return;
    unsigned long elapsed = millis() - stateStartedAt;

    if (emergencyLineTriggered && state != LINE_REVERSE && state != LINE_TURN && 
        state != IDLE_WAIT_BUTTON && state != COUNTDOWN_5S && state != MATADOR_DODGE) {
      enterState(LINE_REVERSE);
      return; 
    }

    TargetState currentTarget = scanTarget();

    switch (state) {
      case IDLE_WAIT_BUTTON:
        if (digitalRead(PIN_BTN_START) == LOW) enterState(COUNTDOWN_5S);
        break;

      case COUNTDOWN_5S:
        if (elapsed >= 5000) enterState(MATADOR_DODGE); 
        break;

      case MATADOR_DODGE:
        if (elapsed >= 350) enterState(SEARCH_SCAN);
        break;

      case SEARCH_SCAN:
        if (elapsed % 200 < 150) {
          if (searchTurnRight) drive(150, -150); else drive(-150, 150);
        } else {
          stopMotors(); 
        }

        if (currentTarget == TARGET_LOCKED) enterState(ATTACK);
        else if (currentTarget == TARGET_EDGE) enterState(TRACKING);
        break;

      case TRACKING:
        if (searchTurnRight) drive(100, -100); else drive(-100, 100);
        
        if (currentTarget == TARGET_LOCKED) enterState(ATTACK);
        else if (currentTarget == TARGET_NONE) enterState(SEARCH_SCAN);
        break;

      case ATTACK:
        if (elapsed < 100) {
          int pumpPWM = 150 + (elapsed * (255 - 150) / 100);
          drive(pumpPWM, pumpPWM);
        } else {
          drive(255, 255); 
        }

        if (currentTarget == TARGET_NONE && elapsed > 200) enterState(SEARCH_SCAN);
        break;

      case LINE_REVERSE:
        if ((elapsed >= 250 && digitalRead(PIN_LINE) == HIGH) || elapsed >= 500) enterState(LINE_TURN);
        break;

      case LINE_TURN:
        if (digitalRead(PIN_LINE) == LOW) enterState(LINE_REVERSE); 
        else if (elapsed >= 300) enterState(SEARCH_SCAN);
        break;
        
      default: break;
    }
  }
};

SumoRobot robot;

void setup() {
  Serial.begin(115200);
  robot.begin();
}

void loop() {
  robot.update();
}