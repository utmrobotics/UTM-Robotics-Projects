#include <QTRSensors.h>

#define LED_BUILTIN 2

#define MOTOR_A_IN1 21 // blue
#define MOTOR_A_IN2 19 // white

#define MOTOR_B_IN1 18 // orange
#define MOTOR_B_IN2 27 // yellow

#define BASE_DUTY_CYCLE 255

#define IR1_PIN 36 // or VP
#define IR3_PIN 34
#define IR5_PIN 32

QTRSensors qtr;

void moveA(int dutyCycle) {
  if (dutyCycle >= 0) {
    analogWrite(MOTOR_A_IN1, dutyCycle);
    analogWrite(MOTOR_A_IN2, 0);
  }
  else {
    analogWrite(MOTOR_A_IN1, 0);
    analogWrite(MOTOR_A_IN2, abs(dutyCycle));
  }
}
void moveB(int dutyCycle) {
  if (dutyCycle >= 0) {
    analogWrite(MOTOR_B_IN1, dutyCycle);
    analogWrite(MOTOR_B_IN2, 0);
  }
  else {
    analogWrite(MOTOR_B_IN1, 0);
    analogWrite(MOTOR_B_IN2, abs(dutyCycle));
  }
}

String serialReadLine() {
  String data = "";
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      data.trim();
      break;
    }
    data += c;
  }
  return data;
}

void calibrateSensors() {
  Serial.println("Calibrating Sensors...");
  digitalWrite(LED_BUILTIN, HIGH);
  for (uint8_t i = 0; i < 250; i++) {
    qtr.calibrate();
    delay(20);
  }
  digitalWrite(LED_BUILTIN, LOW);
}

void setup() {
  pinMode(MOTOR_A_IN1, OUTPUT);
  pinMode(MOTOR_A_IN2, OUTPUT);
  pinMode(MOTOR_B_IN1, OUTPUT);
  pinMode(MOTOR_B_IN2, OUTPUT);

  pinMode(LED_BUILTIN, OUTPUT);

  qtr.setTypeAnalog();
  qtr.setSensorPins((const uint8_t[]){IR1_PIN, IR3_PIN, IR5_PIN}, 3);

  Serial.begin(115200);

  calibrateSensors();
}

void loop() {
  static int16_t lastError = 0;
  static int16_t lastKnownError = 0; // the last error where the robot wasn't off the line
  
  uint16_t sensorValues[3];
  // int16_t position = qtr.readLineBlack(sensorValues);
  int16_t position = qtr.readLineWhite(sensorValues);
  int16_t error = 1000 - position;

  static int16_t threshold = 200;
  static float Kp = 0.5;
  static float Kd = 5.0;
  if (sensorValues[0] > threshold && sensorValues[1] > threshold && sensorValues[2] > threshold) {
    lastKnownError = (!lastKnownError) ? lastError : lastKnownError;
    if (lastKnownError > 0) {
      moveA(255);
      moveB(-255);
    } else {
      moveA(-255);
      moveB(255);
    }
  } 
  else {
    int16_t motorSpeedChange = Kp * error + Kd * (error - lastError);
    lastError = error;

    int16_t mASpeed = BASE_DUTY_CYCLE + motorSpeedChange;
    mASpeed = constrain(mASpeed, -255, 255);
    int16_t mBSpeed = BASE_DUTY_CYCLE - motorSpeedChange;
    mBSpeed = constrain(mBSpeed, -255, 255);
    moveA(mASpeed);
    moveB(mBSpeed);

    lastKnownError = 0;
  }

  String input = serialReadLine();
  if (input == "modify") {
    moveA(0);
    moveB(0);
    Serial.println("Enter a Kp value.");
    input = "";
    while (!input.length()) {
      input = serialReadLine();
    }
    Kp = input.toFloat();
    Serial.printf("\nKp = %f\n", Kp);

    Serial.println("Enter a Kd value.");
    input = "";
    while (!input.length()) {
      input = serialReadLine();
    }
    Kd = input.toFloat();
    Serial.printf("\nKd = %f\n", Kd);

    Serial.println("Enter a threshold value.");
    input = "";
    while (!input.length()) {
      input = serialReadLine();
    }
    threshold = input.toInt();
    Serial.printf("\nthreshold = %d\n", threshold);
  }
  else if (input == "calibrate") {
    moveA(0);
    moveB(0);
    calibrateSensors();
  }

  // Serial.printf("%d %d %d %d\n", sensorValues[0], sensorValues[1], sensorValues[2], position);
}
