#include <Arduino.h>
#include <esp_system.h>

#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif

#ifndef ESP_ARDUINO_VERSION_MAJOR
#define ESP_ARDUINO_VERSION_MAJOR 2
#endif

constexpr uint8_t PIN_START_BUTTON = 27;

constexpr uint8_t PIN_US_TRIG = 25;
constexpr uint8_t PIN_US_ECHO = 26;

constexpr uint8_t PIN_TOF_SDA = 33;
constexpr uint8_t PIN_TOF_SCL = 13;

constexpr uint8_t PIN_LEFT_PWM  = 18;
constexpr uint8_t PIN_LEFT_IN1  = 21;
constexpr uint8_t PIN_LEFT_IN2  = 19;
constexpr uint8_t PIN_RIGHT_PWM = 5;
constexpr uint8_t PIN_RIGHT_IN1 = 22;
constexpr uint8_t PIN_RIGHT_IN2 = 23;

constexpr bool INVERT_LEFT_MOTOR = false;
constexpr bool INVERT_RIGHT_MOTOR = false;

constexpr uint8_t PIN_EDGE_CENTER = 32;
constexpr uint8_t EDGE_WHITE_LEVEL = HIGH;

constexpr uint32_t PWM_FREQUENCY = 20000;
constexpr uint8_t PWM_RESOLUTION = 8;
constexpr uint8_t PWM_CHANNEL_LEFT = 0;
constexpr uint8_t PWM_CHANNEL_RIGHT = 1;

#ifndef USE_VL53L0X
#define USE_VL53L0X 0
#endif

#if USE_VL53L0X
#include <Wire.h>
#include <VL53L0X.h>
VL53L0X tof;
#endif

constexpr uint32_t START_DEBOUNCE_MS = 35;
constexpr uint32_t PHASE_1_MS = 5000;

constexpr uint32_t US_PROBE_MIN_INTERVAL_MS = 45;
constexpr uint32_t US_PROBE_MAX_INTERVAL_MS = 70;
constexpr uint32_t US_JUKE_MIN_INTERVAL_MS  = 26;
constexpr uint32_t US_JUKE_MAX_INTERVAL_MS  = 36;

constexpr uint16_t US_MIN_CM = 3;
constexpr uint16_t US_MAX_CM = 55;
constexpr uint32_t US_TIMEOUT_US = 3500;

constexpr uint32_t TOF_INTERVAL_MS = 50;
constexpr uint16_t TOF_MIN_MM = 20;
constexpr uint16_t TOF_MAX_MM = 550;

constexpr uint32_t JUKE_MIN_MS = 75;
constexpr uint32_t JUKE_DEFAULT_MS = 145;
constexpr uint32_t JUKE_MAX_MS = 240;
constexpr uint8_t LOST_CONFIRM_COUNT = 3;

constexpr uint32_t COUNTER_BRAKE_MS = 15;

constexpr uint32_t COMBAT_US_MIN_INTERVAL_MS = 30;
constexpr uint32_t COMBAT_US_MAX_INTERVAL_MS = 42;
constexpr uint32_t US_FRESH_MS = 80;
constexpr uint32_t TOF_FRESH_MS = 110;
constexpr uint8_t TARGET_ACQUIRE_COUNT = 2;
constexpr uint8_t TARGET_LOST_COUNT = 3;

constexpr uint32_t RELOCK_REVERSE_DIRECTION_MS = 420;
constexpr uint32_t RELOCK_TIMEOUT_MS = 900;
constexpr uint32_t STRIKE_TOTAL_TIMEOUT_MS = 650;
constexpr uint16_t CONTACT_DISTANCE_CM = 14;
constexpr uint16_t CONTACT_HOLD_DISTANCE_CM = 17;
constexpr uint32_t CONTACT_TEST_MS = 100;
constexpr uint32_t ATTACK_BURST_MS = 80;
constexpr uint32_t ANCHOR_ESCAPE_MS = 65;
constexpr uint32_t FLANK_ARC_MS = 220;

constexpr uint32_t BLIND_RUSH_MS = 70;
constexpr uint32_t SEARCH_DIRECTION_MS = 650;
constexpr uint32_t EDGE_REVERSE_MIN_MS = 65;
constexpr uint32_t EDGE_REVERSE_MAX_MS = 125;
constexpr uint32_t EDGE_CLEAR_CONFIRM_MS = 20;
constexpr uint32_t EDGE_TURN_MS = 160;

constexpr int PWM_SEARCH = 90;
constexpr int PWM_JUKE = 190;
constexpr int PWM_APPROACH = 175;
constexpr int PWM_CONTACT_TEST = 190;
constexpr int PWM_ATTACK_BURST = 205;
constexpr int PWM_BLIND_RUSH = 175;
constexpr int PWM_ESCAPE = 160;
constexpr int PWM_FLANK_SLOW = 75;
constexpr int PWM_FLANK_FAST = 185;
constexpr int PWM_EDGE_REVERSE = 175;
constexpr int PWM_EDGE_TURN = 145;

enum MatchState {
  WAIT_START,
  PHASE_1_PROBING,
  PHASE_2_JUKE,
  PHASE_2_BRAKE,
  PHASE_3_RELOCK,
  PHASE_4_STRIKE,
  PHASE_5_RING_CONTROL
};

enum Phase5SubState {
  P5_REGULAR_TRACKING,
  P5_CONTACT_TEST,
  P5_ATTACK_BURST,
  P5_ANCHOR_ESCAPE,
  P5_FLANK_ARC,
  P5_EDGE_REVERSING,
  P5_EDGE_TURNING
};

enum Phase4SubState {
  P4_APPROACH,
  P4_CONTACT_TEST,
  P4_ATTACK_BURST
};

enum Alignment {
  ALIGNMENT_UNKNOWN,
  ALIGNMENT_FRONT
};

struct OpponentTelemetry {
  uint16_t usAttempts = 0;
  uint16_t usValidSamples = 0;
  uint16_t usTimeoutSamples = 0;

  uint16_t tofAttempts = 0;
  uint16_t tofValidSamples = 0;

  uint16_t distanceCm = 9999;
  uint8_t confidencePercent = 0;
  bool detected = false;
  Alignment alignment = ALIGNMENT_UNKNOWN;
};

OpponentTelemetry enemy;
MatchState currentState = WAIT_START;

constexpr size_t MAX_DISTANCE_SAMPLES = 105;
uint16_t usSamples[MAX_DISTANCE_SAMPLES];
uint16_t tofSamples[MAX_DISTANCE_SAMPLES];
size_t usSampleCount = 0;
size_t tofSampleCount = 0;

uint32_t phaseStartMs = 0;
uint32_t lastUsScanMs = 0;
uint32_t nextUsIntervalMs = US_PROBE_MIN_INTERVAL_MS;
uint32_t lastTofScanMs = 0;
uint32_t brakeStartMs = 0;
uint32_t combatStateStartMs = 0;
uint32_t lastCombatUsMs = 0;
uint32_t nextCombatUsIntervalMs = COMBAT_US_MIN_INTERVAL_MS;
uint32_t lastCombatTofMs = 0;
uint32_t predictiveRushStartMs = 0;
uint32_t searchDirectionStartMs = 0;
uint32_t edgeEscapeStartMs = 0;
uint32_t edgeClearStartMs = 0;
uint32_t tacticalActionStartMs = 0;

int jukeDirection = 1;
int searchDirection = 1;
int edgeTurnDirection = 1;
int flankDirection = 1;
uint8_t consecutiveTargetMisses = 0;
uint8_t combatSeenStreak = 0;
uint8_t combatMissStreak = 0;
int combatUsCm = -1;
int combatTofMm = -1;
bool predictiveRushActive = false;
bool p5HadTarget = false;
Phase5SubState p5State = P5_REGULAR_TRACKING;
Phase4SubState p4State = P4_APPROACH;

void setMotor(int speed, uint8_t pwmPin, uint8_t in1, uint8_t in2);
void shortBrakeMotor(uint8_t pwmPin, uint8_t in1, uint8_t in2);
bool setupMotorPwm();
void writeMotorPwm(uint8_t pwmPin, uint8_t duty);
void motorsDrive(int leftSpeed, int rightSpeed);
void stopMotors();
void executeJuke(int direction);
void executeCounterBrake(int direction);

bool edgeDetected();
void enterEdgeEscape(uint32_t now);
bool startButtonPressed(uint32_t now);
int readUltrasonicCm();
int readToFMm();

void resetTelemetry();
void collectPhase1(uint32_t now);
void finalizePhase1();
bool targetLostConfirmed(uint32_t now);
uint16_t medianOf(uint16_t *values, size_t count);
void printTelemetry();
void resetCombatObservation(uint32_t now);
void updateCombatSensors(uint32_t now);
bool combatTargetSeen(uint32_t now);
uint16_t combatDistanceCm(uint32_t now);
void executeSearchTurn(int direction, int pwm);
void handlePhase3(uint32_t now);
void handlePhase4(uint32_t now);
void handlePhase5(uint32_t now);

void setup() {
  Serial.begin(115200);

  pinMode(PIN_START_BUTTON, INPUT_PULLUP);
  pinMode(PIN_US_TRIG, OUTPUT);
  pinMode(PIN_US_ECHO, INPUT);
  digitalWrite(PIN_US_TRIG, LOW);

  pinMode(PIN_LEFT_IN1, OUTPUT);
  pinMode(PIN_LEFT_IN2, OUTPUT);
  pinMode(PIN_RIGHT_IN1, OUTPUT);
  pinMode(PIN_RIGHT_IN2, OUTPUT);

  pinMode(PIN_EDGE_CENTER, INPUT);

  if (!setupMotorPwm()) {
    Serial.println("[LOI] Khong khoi tao duoc LEDC PWM.");
  }

  randomSeed(esp_random());

#if USE_VL53L0X
  Wire.begin(PIN_TOF_SDA, PIN_TOF_SCL);
  tof.setTimeout(30);
  if (!tof.init()) {
    Serial.println("[LOI] Khong tim thay VL53L0X.");
  } else {
    tof.setMeasurementTimingBudget(50000);
    tof.startContinuous(TOF_INTERVAL_MS);
  }
#endif

  stopMotors();
  Serial.println("[SAN SANG] Cho trong tai ho, sau do bam nut START.");
}

void loop() {
  const uint32_t now = millis();

  switch (currentState) {
    case WAIT_START:
      stopMotors();
      if (startButtonPressed(now)) {
        resetTelemetry();
        phaseStartMs = now;
        lastUsScanMs = now;
        lastTofScanMs = now;
        nextUsIntervalMs = random(US_PROBE_MIN_INTERVAL_MS,
                                  US_PROBE_MAX_INTERVAL_MS + 1);
        currentState = PHASE_1_PROBING;
        Serial.println("[PHASE 1] Bat dau dem 5 giay va tham do.");
      }
      break;

    case PHASE_1_PROBING:
      stopMotors();
      collectPhase1(now);

      if (now - phaseStartMs >= PHASE_1_MS) {
        finalizePhase1();
        jukeDirection = (esp_random() & 1U) ? 1 : 2;
        printTelemetry();

        phaseStartMs = now;
        lastUsScanMs = now;
        nextUsIntervalMs = random(US_JUKE_MIN_INTERVAL_MS,
                                  US_JUKE_MAX_INTERVAL_MS + 1);
        consecutiveTargetMisses = 0;
        currentState = PHASE_2_JUKE;
        Serial.println("[PHASE 2] Bat dau pivot co phan hoi cam bien.");
      }
      break;

    case PHASE_2_JUKE: {
      const uint32_t elapsed = now - phaseStartMs;

      if (edgeDetected()) {
        enterEdgeEscape(now);
        Serial.println("[AN TOAN] Phat hien vach trang trong luc ne.");
        break;
      }

      executeJuke(jukeDirection);

      const bool lost = targetLostConfirmed(now);
      const bool sensorStop = enemy.detected && elapsed >= JUKE_MIN_MS && lost;
      const bool timedStop = (!enemy.detected && elapsed >= JUKE_DEFAULT_MS) ||
                             elapsed >= JUKE_MAX_MS;

      if (sensorStop || timedStop) {
        brakeStartMs = now;
        currentState = PHASE_2_BRAKE;
      }
      break;
    }

    case PHASE_2_BRAKE:
      if (edgeDetected()) {
        enterEdgeEscape(now);
      } else if (now - brakeStartMs < COUNTER_BRAKE_MS) {
        executeCounterBrake(jukeDirection);
      } else {
        stopMotors();
        resetCombatObservation(now);
        searchDirection = (jukeDirection == 1) ? 2 : 1;
        currentState = PHASE_3_RELOCK;
        Serial.println("[PHASE 3] Ket thuc ne, bat dau khoa lai muc tieu.");
      }
      break;

    case PHASE_3_RELOCK:
      handlePhase3(now);
      break;

    case PHASE_4_STRIKE:
      handlePhase4(now);
      break;

    case PHASE_5_RING_CONTROL:
      handlePhase5(now);
      break;
  }
}

void resetTelemetry() {
  enemy = OpponentTelemetry{};
  usSampleCount = 0;
  tofSampleCount = 0;
}

void collectPhase1(uint32_t now) {
  if (now - lastUsScanMs >= nextUsIntervalMs) {
    lastUsScanMs = now;
    nextUsIntervalMs = random(US_PROBE_MIN_INTERVAL_MS,
                              US_PROBE_MAX_INTERVAL_MS + 1);

    enemy.usAttempts++;
    const int cm = readUltrasonicCm();
    if (cm >= US_MIN_CM && cm <= US_MAX_CM) {
      enemy.usValidSamples++;
      if (usSampleCount < MAX_DISTANCE_SAMPLES) {
        usSamples[usSampleCount++] = static_cast<uint16_t>(cm);
      }
    } else {
      enemy.usTimeoutSamples++;
    }
  }

#if USE_VL53L0X
  if (now - lastTofScanMs >= TOF_INTERVAL_MS) {
    lastTofScanMs = now;
    enemy.tofAttempts++;
    const int mm = readToFMm();
    if (mm >= TOF_MIN_MM && mm <= TOF_MAX_MM) {
      enemy.tofValidSamples++;
      if (tofSampleCount < MAX_DISTANCE_SAMPLES) {
        tofSamples[tofSampleCount++] = static_cast<uint16_t>(mm);
      }
    }
  }
#endif
}

void finalizePhase1() {
  const float usRate = enemy.usAttempts
                           ? static_cast<float>(enemy.usValidSamples) / enemy.usAttempts
                           : 0.0f;
  const float tofRate = enemy.tofAttempts
                            ? static_cast<float>(enemy.tofValidSamples) / enemy.tofAttempts
                            : 0.0f;

  if (tofSampleCount >= 5) {
    enemy.distanceCm = medianOf(tofSamples, tofSampleCount) / 10;
  } else if (usSampleCount >= 5) {
    enemy.distanceCm = medianOf(usSamples, usSampleCount);
  }

  const float fusedConfidence = USE_VL53L0X
                                    ? (0.55f * tofRate + 0.45f * usRate)
                                    : usRate;
  enemy.confidencePercent = static_cast<uint8_t>(
      constrain(static_cast<int>(fusedConfidence * 100.0f + 0.5f), 0, 100));

  enemy.detected = (tofRate >= 0.55f) || (usRate >= 0.45f);
  enemy.alignment = enemy.detected ? ALIGNMENT_FRONT : ALIGNMENT_UNKNOWN;
}

uint16_t medianOf(uint16_t *values, size_t count) {
  for (size_t i = 1; i < count; ++i) {
    const uint16_t key = values[i];
    size_t j = i;
    while (j > 0 && values[j - 1] > key) {
      values[j] = values[j - 1];
      --j;
    }
    values[j] = key;
  }

  if (count == 0) return 9999;
  if (count & 1U) return values[count / 2];
  return static_cast<uint16_t>(
      (static_cast<uint32_t>(values[count / 2 - 1]) + values[count / 2]) / 2U);
}

bool targetLostConfirmed(uint32_t now) {
  if (now - lastUsScanMs < nextUsIntervalMs) {
    return consecutiveTargetMisses >= LOST_CONFIRM_COUNT;
  }

  lastUsScanMs = now;
  nextUsIntervalMs = random(US_JUKE_MIN_INTERVAL_MS,
                            US_JUKE_MAX_INTERVAL_MS + 1);

  const int cm = readUltrasonicCm();
  if (cm >= US_MIN_CM && cm <= US_MAX_CM) {
    consecutiveTargetMisses = 0;
  } else if (consecutiveTargetMisses < 255) {
    consecutiveTargetMisses++;
  }

  return consecutiveTargetMisses >= LOST_CONFIRM_COUNT;
}

int readUltrasonicCm() {
  digitalWrite(PIN_US_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_US_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_US_TRIG, LOW);

  const uint32_t duration = pulseIn(PIN_US_ECHO, HIGH, US_TIMEOUT_US);
  if (duration == 0) return -1;
  return static_cast<int>((duration * 343UL) / 20000UL);
}

int readToFMm() {
#if USE_VL53L0X
  const uint16_t mm = tof.readRangeContinuousMillimeters();
  if (tof.timeoutOccurred()) return -1;
  return mm;
#else
  return -1;
#endif
}

void resetCombatObservation(uint32_t now) {
  combatUsCm = -1;
  combatTofMm = -1;
  combatSeenStreak = 0;
  combatMissStreak = 0;
  lastCombatUsMs = now;
  lastCombatTofMs = now;
  nextCombatUsIntervalMs = random(COMBAT_US_MIN_INTERVAL_MS,
                                   COMBAT_US_MAX_INTERVAL_MS + 1);
  combatStateStartMs = now;
  predictiveRushActive = false;
  p5HadTarget = false;
}

bool combatTargetSeen(uint32_t now) {
  const bool usSeen = combatUsCm >= US_MIN_CM &&
                      combatUsCm <= US_MAX_CM &&
                      now - lastCombatUsMs <= US_FRESH_MS;

#if USE_VL53L0X
  const bool tofSeen = combatTofMm >= TOF_MIN_MM &&
                       combatTofMm <= TOF_MAX_MM &&
                       now - lastCombatTofMs <= TOF_FRESH_MS;
#else
  const bool tofSeen = false;
#endif

  return usSeen || tofSeen;
}

void updateCombatSensors(uint32_t now) {
  bool hasNewSample = false;

  if (now - lastCombatUsMs >= nextCombatUsIntervalMs) {
    combatUsCm = readUltrasonicCm();
    lastCombatUsMs = now;
    nextCombatUsIntervalMs = random(COMBAT_US_MIN_INTERVAL_MS,
                                     COMBAT_US_MAX_INTERVAL_MS + 1);
    hasNewSample = true;
  }

#if USE_VL53L0X
  if (now - lastCombatTofMs >= TOF_INTERVAL_MS) {
    combatTofMm = readToFMm();
    lastCombatTofMs = now;
    hasNewSample = true;
  }
#endif

  if (!hasNewSample) return;

  const uint32_t afterRead = millis();
  if (combatTargetSeen(afterRead)) {
    if (combatSeenStreak < 255) combatSeenStreak++;
    combatMissStreak = 0;
  } else {
    combatSeenStreak = 0;
    if (combatMissStreak < 255) combatMissStreak++;
  }
}

uint16_t combatDistanceCm(uint32_t now) {
#if USE_VL53L0X
  if (combatTofMm >= TOF_MIN_MM && combatTofMm <= TOF_MAX_MM &&
      now - lastCombatTofMs <= TOF_FRESH_MS) {
    return static_cast<uint16_t>(combatTofMm / 10);
  }
#endif

  if (combatUsCm >= US_MIN_CM && combatUsCm <= US_MAX_CM &&
      now - lastCombatUsMs <= US_FRESH_MS) {
    return static_cast<uint16_t>(combatUsCm);
  }
  return 9999;
}

void executeSearchTurn(int direction, int pwm) {
  pwm = constrain(pwm, 0, 255);
  if (direction == 1) {
    motorsDrive(-pwm, pwm);
  } else {
    motorsDrive(pwm, -pwm);
  }
}

void enterEdgeEscape(uint32_t now) {
  stopMotors();
  currentState = PHASE_5_RING_CONTROL;
  p5State = P5_EDGE_REVERSING;
  edgeEscapeStartMs = now;
  edgeClearStartMs = 0;
  edgeTurnDirection = (edgeTurnDirection == 1) ? 2 : 1;
  predictiveRushActive = false;
  p5HadTarget = false;
  Serial.println("[BIEN] Bat dau lui ngan de dua cam bien khoi vach trang.");
}

void handlePhase3(uint32_t now) {
  if (edgeDetected()) {
    enterEdgeEscape(now);
    return;
  }

  updateCombatSensors(now);
  const uint32_t elapsed = now - combatStateStartMs;

  if (combatSeenStreak >= TARGET_ACQUIRE_COUNT) {
    stopMotors();
    combatStateStartMs = millis();
    tacticalActionStartMs = combatStateStartMs;
    combatMissStreak = 0;
    p4State = P4_APPROACH;
    currentState = PHASE_4_STRIKE;
    Serial.println("[PHASE 3 -> 4] Da xac nhan muc tieu qua nhieu mau.");
    return;
  }

  if (elapsed >= RELOCK_TIMEOUT_MS) {
    stopMotors();
    p5State = P5_REGULAR_TRACKING;
    searchDirectionStartMs = now;
    predictiveRushActive = false;
    p5HadTarget = false;
    currentState = PHASE_5_RING_CONTROL;
    Serial.println("[PHASE 3 -> 5] Het timeout khoa muc tieu, chuyen sang tim kiem.");
    return;
  }

  const int direction = elapsed < RELOCK_REVERSE_DIRECTION_MS
                            ? searchDirection
                            : (3 - searchDirection);
  executeSearchTurn(direction, PWM_SEARCH);
}

void handlePhase4(uint32_t now) {
  if (edgeDetected()) {
    enterEdgeEscape(now);
    return;
  }

  updateCombatSensors(now);
  const bool seen = combatTargetSeen(now) && combatSeenStreak > 0;

  if (combatMissStreak >= TARGET_LOST_COUNT) {
    stopMotors();
    p5State = P5_REGULAR_TRACKING;
    searchDirectionStartMs = now;
    predictiveRushActive = false;
    p5HadTarget = false;
    currentState = PHASE_5_RING_CONTROL;
    Serial.println("[PHASE 4 -> 5] Mat muc tieu, huy chu trinh tan cong.");
    return;
  }

  if (now - combatStateStartMs >= STRIKE_TOTAL_TIMEOUT_MS) {
    stopMotors();
    p5State = P5_REGULAR_TRACKING;
    searchDirectionStartMs = now;
    predictiveRushActive = false;
    p5HadTarget = seen;
    currentState = PHASE_5_RING_CONTROL;
    Serial.println("[PHASE 4 -> 5] Het timeout tiep can, chuyen sang dieu khien san.");
    return;
  }

  switch (p4State) {
    case P4_APPROACH: {
      if (!seen) {
        executeSearchTurn(searchDirection, PWM_SEARCH);
        break;
      }

      const uint16_t distance = combatDistanceCm(now);
      if (distance <= CONTACT_DISTANCE_CM) {
        p4State = P4_CONTACT_TEST;
        tacticalActionStartMs = now;
        motorsDrive(PWM_CONTACT_TEST, PWM_CONTACT_TEST);
      } else {
        motorsDrive(PWM_APPROACH, PWM_APPROACH);
      }
      break;
    }

    case P4_CONTACT_TEST:
      motorsDrive(PWM_CONTACT_TEST, PWM_CONTACT_TEST);
      if (now - tacticalActionStartMs >= CONTACT_TEST_MS) {
        if (seen && combatDistanceCm(now) <= CONTACT_HOLD_DISTANCE_CM) {
          p4State = P4_ATTACK_BURST;
          tacticalActionStartMs = now;
        } else {
          p4State = P4_APPROACH;
        }
      }
      break;

    case P4_ATTACK_BURST:
      motorsDrive(PWM_ATTACK_BURST, PWM_ATTACK_BURST);
      if (now - tacticalActionStartMs >= ATTACK_BURST_MS) {
        if (seen && combatDistanceCm(now) <= CONTACT_HOLD_DISTANCE_CM) {
          stopMotors();
          flankDirection = (flankDirection == 1) ? 2 : 1;
          p5State = P5_ANCHOR_ESCAPE;
          tacticalActionStartMs = now;
          currentState = PHASE_5_RING_CONTROL;
          Serial.println("[ANTI-ANCHOR] Muc tieu van sat sau xung day, rut ra danh suon.");
        } else {
          p5State = P5_REGULAR_TRACKING;
          p5HadTarget = seen;
          predictiveRushActive = false;
          searchDirectionStartMs = now;
          currentState = PHASE_5_RING_CONTROL;
        }
      }
      break;
  }
}

void handlePhase5(uint32_t now) {
  switch (p5State) {
    case P5_REGULAR_TRACKING: {
      if (edgeDetected()) {
        enterEdgeEscape(now);
        return;
      }

      updateCombatSensors(now);
      const bool seen = combatTargetSeen(now) && combatSeenStreak > 0;

      if (seen) {
        predictiveRushActive = false;
        p5HadTarget = true;
        const uint16_t distance = combatDistanceCm(now);
        if (distance <= CONTACT_DISTANCE_CM) {
          p5State = P5_CONTACT_TEST;
          tacticalActionStartMs = now;
          motorsDrive(PWM_CONTACT_TEST, PWM_CONTACT_TEST);
        } else {
          motorsDrive(PWM_APPROACH, PWM_APPROACH);
        }
        return;
      }

      if (p5HadTarget && !predictiveRushActive) {
        predictiveRushActive = true;
        predictiveRushStartMs = now;
      }

      if (predictiveRushActive && now - predictiveRushStartMs < BLIND_RUSH_MS) {
        motorsDrive(PWM_BLIND_RUSH, PWM_BLIND_RUSH);
        return;
      }

      if (now - searchDirectionStartMs >= SEARCH_DIRECTION_MS) {
        searchDirection = 3 - searchDirection;
        searchDirectionStartMs = now;
      }
      executeSearchTurn(searchDirection, PWM_SEARCH);
      break;
    }

    case P5_CONTACT_TEST: {
      if (edgeDetected()) {
        enterEdgeEscape(now);
        return;
      }

      updateCombatSensors(now);
      const bool seen = combatTargetSeen(now) && combatSeenStreak > 0;
      motorsDrive(PWM_CONTACT_TEST, PWM_CONTACT_TEST);

      if (!seen && combatMissStreak >= TARGET_LOST_COUNT) {
        stopMotors();
        p5State = P5_REGULAR_TRACKING;
        p5HadTarget = false;
        predictiveRushActive = false;
        searchDirectionStartMs = now;
      } else if (now - tacticalActionStartMs >= CONTACT_TEST_MS) {
        if (seen && combatDistanceCm(now) <= CONTACT_HOLD_DISTANCE_CM) {
          p5State = P5_ATTACK_BURST;
          tacticalActionStartMs = now;
        } else {
          p5State = P5_REGULAR_TRACKING;
        }
      }
      break;
    }

    case P5_ATTACK_BURST: {
      if (edgeDetected()) {
        enterEdgeEscape(now);
        return;
      }

      updateCombatSensors(now);
      const bool seen = combatTargetSeen(now) && combatSeenStreak > 0;
      motorsDrive(PWM_ATTACK_BURST, PWM_ATTACK_BURST);

      if (now - tacticalActionStartMs >= ATTACK_BURST_MS) {
        if (seen && combatDistanceCm(now) <= CONTACT_HOLD_DISTANCE_CM) {
          stopMotors();
          flankDirection = (flankDirection == 1) ? 2 : 1;
          p5State = P5_ANCHOR_ESCAPE;
          tacticalActionStartMs = now;
          Serial.println("[ANTI-ANCHOR] Khong ghe motor TT; rut ngan va doi goc.");
        } else {
          p5State = P5_REGULAR_TRACKING;
          p5HadTarget = seen;
          predictiveRushActive = false;
          searchDirectionStartMs = now;
        }
      }
      break;
    }

    case P5_ANCHOR_ESCAPE:
      motorsDrive(-PWM_ESCAPE, -PWM_ESCAPE);
      if (now - tacticalActionStartMs >= ANCHOR_ESCAPE_MS) {
        stopMotors();
        p5State = P5_FLANK_ARC;
        tacticalActionStartMs = now;
      }
      break;

    case P5_FLANK_ARC:
      if (edgeDetected()) {
        enterEdgeEscape(now);
        return;
      }

      if (flankDirection == 1) {
        motorsDrive(PWM_FLANK_SLOW, PWM_FLANK_FAST);
      } else {
        motorsDrive(PWM_FLANK_FAST, PWM_FLANK_SLOW);
      }

      if (now - tacticalActionStartMs >= FLANK_ARC_MS) {
        stopMotors();
        resetCombatObservation(now);
        searchDirection = flankDirection;
        currentState = PHASE_3_RELOCK;
        Serial.println("[ANTI-ANCHOR] Ket thuc cung danh suon, khoa lai muc tieu.");
      }
      break;

    case P5_EDGE_REVERSING: {
      const bool onLine = edgeDetected();
      const uint32_t elapsed = now - edgeEscapeStartMs;

      motorsDrive(-PWM_EDGE_REVERSE, -PWM_EDGE_REVERSE);

      if (onLine) {
        edgeClearStartMs = 0;
      } else if (edgeClearStartMs == 0) {
        edgeClearStartMs = now;
      }

      const bool clearConfirmed = edgeClearStartMs != 0 &&
                                  now - edgeClearStartMs >= EDGE_CLEAR_CONFIRM_MS;
      if ((elapsed >= EDGE_REVERSE_MIN_MS && clearConfirmed) ||
          elapsed >= EDGE_REVERSE_MAX_MS) {
        stopMotors();
        p5State = P5_EDGE_TURNING;
        edgeEscapeStartMs = now;
      }
      break;
    }

    case P5_EDGE_TURNING:
      if (edgeDetected()) {
        p5State = P5_EDGE_REVERSING;
        edgeEscapeStartMs = now;
        edgeClearStartMs = 0;
        motorsDrive(-PWM_EDGE_REVERSE, -PWM_EDGE_REVERSE);
        break;
      }

      executeSearchTurn(edgeTurnDirection, PWM_EDGE_TURN);
      if (now - edgeEscapeStartMs >= EDGE_TURN_MS) {
        stopMotors();
        resetCombatObservation(now);
        searchDirection = edgeTurnDirection;
        searchDirectionStartMs = now;
        p5State = P5_REGULAR_TRACKING;
        Serial.println("[BIEN] Da lui va xoay khoi vach, quay lai tim dich.");
      }
      break;
  }
}

void setMotor(int speed, uint8_t pwmPin, uint8_t in1, uint8_t in2) {
  speed = constrain(speed, -255, 255);

  if (speed > 0) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    writeMotorPwm(pwmPin, static_cast<uint8_t>(speed));
  } else if (speed < 0) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    writeMotorPwm(pwmPin, static_cast<uint8_t>(-speed));
  } else {
    writeMotorPwm(pwmPin, 0);
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
  }
}

void shortBrakeMotor(uint8_t pwmPin, uint8_t in1, uint8_t in2) {
  digitalWrite(in1, HIGH);
  digitalWrite(in2, HIGH);
  writeMotorPwm(pwmPin, 255);
}

bool setupMotorPwm() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  return ledcAttach(PIN_LEFT_PWM, PWM_FREQUENCY, PWM_RESOLUTION) &&
         ledcAttach(PIN_RIGHT_PWM, PWM_FREQUENCY, PWM_RESOLUTION);
#else
  ledcSetup(PWM_CHANNEL_LEFT, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcSetup(PWM_CHANNEL_RIGHT, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(PIN_LEFT_PWM, PWM_CHANNEL_LEFT);
  ledcAttachPin(PIN_RIGHT_PWM, PWM_CHANNEL_RIGHT);
  return true;
#endif
}

void writeMotorPwm(uint8_t pwmPin, uint8_t duty) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(pwmPin, duty);
#else
  ledcWrite(pwmPin == PIN_LEFT_PWM ? PWM_CHANNEL_LEFT : PWM_CHANNEL_RIGHT, duty);
#endif
}

void motorsDrive(int leftSpeed, int rightSpeed) {
  if (INVERT_LEFT_MOTOR) leftSpeed = -leftSpeed;
  if (INVERT_RIGHT_MOTOR) rightSpeed = -rightSpeed;
  setMotor(leftSpeed, PIN_LEFT_PWM, PIN_LEFT_IN1, PIN_LEFT_IN2);
  setMotor(rightSpeed, PIN_RIGHT_PWM, PIN_RIGHT_IN1, PIN_RIGHT_IN2);
}

void stopMotors() {
  motorsDrive(0, 0);
}

void executeJuke(int direction) {
  if (direction == 1) {
    motorsDrive(0, PWM_JUKE);
  } else {
    motorsDrive(PWM_JUKE, 0);
  }
}

void executeCounterBrake(int direction) {
  if (direction == 1) {
    setMotor(0, PIN_LEFT_PWM, PIN_LEFT_IN1, PIN_LEFT_IN2);
    shortBrakeMotor(PIN_RIGHT_PWM, PIN_RIGHT_IN1, PIN_RIGHT_IN2);
  } else {
    shortBrakeMotor(PIN_LEFT_PWM, PIN_LEFT_IN1, PIN_LEFT_IN2);
    setMotor(0, PIN_RIGHT_PWM, PIN_RIGHT_IN1, PIN_RIGHT_IN2);
  }
}

bool edgeDetected() {
  return digitalRead(PIN_EDGE_CENTER) == EDGE_WHITE_LEVEL;
}

bool startButtonPressed(uint32_t now) {
  static bool lastReading = HIGH;
  static bool stableState = HIGH;
  static uint32_t changedAt = 0;

  const bool reading = digitalRead(PIN_START_BUTTON);
  if (reading != lastReading) {
    lastReading = reading;
    changedAt = now;
  }

  if (now - changedAt >= START_DEBOUNCE_MS) {
    stableState = reading;
  }

  return stableState == LOW;
}

void printTelemetry() {
  Serial.println("\n--- PHASE 1 TELEMETRY ---");
  Serial.printf("US: %u/%u valid, timeout=%u\n",
                enemy.usValidSamples, enemy.usAttempts, enemy.usTimeoutSamples);
  Serial.printf("ToF: %u/%u valid\n",
                enemy.tofValidSamples, enemy.tofAttempts);
  Serial.printf("Detected=%s, confidence=%u%%, median=%u cm\n",
                enemy.detected ? "YES" : "NO",
                enemy.confidencePercent,
                enemy.distanceCm);
  Serial.printf("Juke direction=%s\n\n",
                jukeDirection == 1 ? "LEFT" : "RIGHT");
}