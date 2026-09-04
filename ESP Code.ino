#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"

#define WIFI_SSID "WIFI SSID"
#define WIFI_PASSWORD "WIFI PASSWORD"

#define API_KEY "Firebase API Key"
#define DATABASE_URL "Database URL"

FirebaseData fbdoStream;
FirebaseAuth auth;
FirebaseConfig config;

volatile bool signupOK = false;

#define FR_IN1 26
#define FR_IN2 27
#define FR_EN  14

#define BR_IN1 32
#define BR_IN2 33
#define BR_EN  21

#define FL_IN1 19
#define FL_IN2 18
#define FL_EN  22

#define BL_IN1 4
#define BL_IN2 13
#define BL_EN  23

const int testSpeed = 80;
const int turnSpeed = 120;

String currentCommand = "S";
unsigned long lastCommandMillis = 0;
const unsigned long COMMAND_TIMEOUT_MS = 4000;

// ==================== MOVE FORWARD ====================

void moveForward() {
  analogWrite(FR_EN, testSpeed);
  analogWrite(BR_EN, testSpeed);
  analogWrite(FL_EN, testSpeed);
  analogWrite(BL_EN, testSpeed);

  digitalWrite(FR_IN1, LOW);
  digitalWrite(FR_IN2, HIGH);

  digitalWrite(BR_IN1, LOW);
  digitalWrite(BR_IN2, HIGH);

  digitalWrite(FL_IN1, LOW);
  digitalWrite(FL_IN2, HIGH);

  digitalWrite(BL_IN1, LOW);
  digitalWrite(BL_IN2, HIGH);
}

// ==================== MOVE BACKWARD ====================

void moveBackward() {
  analogWrite(FR_EN, testSpeed);
  analogWrite(BR_EN, testSpeed);
  analogWrite(FL_EN, testSpeed);
  analogWrite(BL_EN, testSpeed);

  digitalWrite(FR_IN1, HIGH);
  digitalWrite(FR_IN2, LOW);

  digitalWrite(BR_IN1, HIGH);
  digitalWrite(BR_IN2, LOW);

  digitalWrite(FL_IN1, HIGH);
  digitalWrite(FL_IN2, LOW);

  digitalWrite(BL_IN1, HIGH);
  digitalWrite(BL_IN2, LOW);
}

// ==================== MOVE LEFT ====================

void moveLeft() {
  analogWrite(FR_EN, turnSpeed);
  analogWrite(BR_EN, turnSpeed);
  analogWrite(FL_EN, turnSpeed);
  analogWrite(BL_EN, turnSpeed);

  digitalWrite(FR_IN1, LOW);
  digitalWrite(FR_IN2, HIGH);

  digitalWrite(BR_IN1, LOW);
  digitalWrite(BR_IN2, HIGH);

  digitalWrite(FL_IN1, HIGH);
  digitalWrite(FL_IN2, LOW);

  digitalWrite(BL_IN1, HIGH);
  digitalWrite(BL_IN2, LOW);
}

// ==================== MOVE RIGHT ====================

void moveRight() {
  analogWrite(FR_EN, turnSpeed);
  analogWrite(BR_EN, turnSpeed);
  analogWrite(FL_EN, turnSpeed);
  analogWrite(BL_EN, turnSpeed);

  digitalWrite(FL_IN1, LOW);
  digitalWrite(FL_IN2, HIGH);

  digitalWrite(BL_IN1, LOW);
  digitalWrite(BL_IN2, HIGH);

  digitalWrite(FR_IN1, HIGH);
  digitalWrite(FR_IN2, LOW);

  digitalWrite(BR_IN1, HIGH);
  digitalWrite(BR_IN2, LOW);
}

// ==================== STOP CAR ====================

void stopCar() {
  analogWrite(FR_EN, 0);
  analogWrite(BR_EN, 0);
  analogWrite(FL_EN, 0);
  analogWrite(BL_EN, 0);

  digitalWrite(FR_IN1, LOW);
  digitalWrite(FR_IN2, LOW);

  digitalWrite(BR_IN1, LOW);
  digitalWrite(BR_IN2, LOW);

  digitalWrite(FL_IN1, LOW);
  digitalWrite(FL_IN2, LOW);

  digitalWrite(BL_IN1, LOW);
  digitalWrite(BL_IN2, LOW);
}

// ==================== EXECUTE COMMAND ====================

void executeCommand(String cmd) {
  if (cmd == "F") {
    moveForward();
  } else if (cmd == "B") {
    moveBackward();
  } else if (cmd == "L") {
    moveLeft();
  } else if (cmd == "R") {
    moveRight();
  } else {
    stopCar();
  }
}

// ==================== FIREBASE STREAM CALLBACK ====================

void streamCallback(FirebaseStream data) {
  if (data.dataType() == "string") {
    String newCommand = data.stringData();
    newCommand.trim();
    newCommand.toUpperCase();

    if (newCommand.length() > 0) {
      currentCommand = newCommand;
      lastCommandMillis = millis();
      Serial.print("Received command: ");
      Serial.println(currentCommand);
      executeCommand(currentCommand);
    }
  }
}

void streamTimeoutCallback(bool timeout) {
  if (timeout) {
    Serial.println("Stream timeout, resuming...");
  }
}

// ==================== WIFI CONNECT ====================

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
}

// ==================== FIREBASE CONNECT ====================

void connectFirebase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase sign-up successful (anonymous)");
    signupOK = true;
  } else {
    Serial.printf("Firebase sign-up failed: %s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Listen to /command path in Realtime Database
  if (!Firebase.RTDB.beginStream(&fbdoStream, "/command")) {
    Serial.printf("Stream begin failed: %s\n", fbdoStream.errorReason().c_str());
  }
  Firebase.RTDB.setStreamCallback(&fbdoStream, streamCallback, streamTimeoutCallback);
}

void setup() {
  Serial.begin(115200);

  pinMode(FR_IN1, OUTPUT);
  pinMode(FR_IN2, OUTPUT);
  pinMode(FR_EN, OUTPUT);

  pinMode(BR_IN1, OUTPUT);
  pinMode(BR_IN2, OUTPUT);
  pinMode(BR_EN, OUTPUT);

  pinMode(FL_IN1, OUTPUT);
  pinMode(FL_IN2, OUTPUT);
  pinMode(FL_EN, OUTPUT);

  pinMode(BL_IN1, OUTPUT);
  pinMode(BL_IN2, OUTPUT);
  pinMode(BL_EN, OUTPUT);

  stopCar();

  connectWiFi();
  connectFirebase();
}

void loop() {
  // Safety timeout: if no new command received in COMMAND_TIMEOUT_MS, stop the car
  if (currentCommand != "S" && millis() - lastCommandMillis > COMMAND_TIMEOUT_MS) {
    Serial.println("Command timeout, stopping car");
    currentCommand = "S";
    stopCar();
  }
}