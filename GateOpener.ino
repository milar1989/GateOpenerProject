#include <Arduino.h>
#include <RCSwitch.h>
#include <timer.h>

#define LED 2
void OpenGateA();
void OpenGateB();
void CloseGateA();
void CloseGateB();
void CloseGates();
void StopGateA();
void StopGateA_AfterOpening();
void StopGateA_AfterClosing();
void StopGateB();
void StopGateB_AfterOpening();
void StopGateB_AfterClosing();
void BlinkLamp();
void LookingForMovement();
void BlinkMovementDetectorLight();
void BlinkMovementDetectorLightOff();
bool GetButton(int buttonsArray[], int buttonsSize, int receivedValue);
bool RunGate = false;
bool LedIsOn = false;
bool MovementLedIsOn = false;
bool MovementFound = false;

RCSwitch mySwitch = RCSwitch();
int aButtons[] = {11141684, 0};
int bButtons[] = {11141688, 0};
int cButtons[] = {11141681, 0};
int dButtons[] = {11141682, 0};

int light = 2;
int aEngineA = 16;
int bEngineA = 4;
int aEngineB = 26;
int bEngineB = 25;
int movementDetector = 18;
int movementDetectorLight = 22;
int limitSwitchAOpen = 19;
int limitSwitchAClose = 21;
int limitSwitchBOpen = 34;
int limitSwitchBClose = 35;

int timeOutForGates = 3000;
int value = 0;
Timer t1;
Timer lightTimer;
Timer movementDetectorLightTimer;
Timer restartArduino;

enum statusEnum { GateOpened, GateOpening, GateClosed, GateClosing, GateStopped, MovementDetected };
statusEnum previeusStatus = GateClosed;
statusEnum actualStatus = GateClosed;

void setup() {
  mySwitch.enableReceive(0);
  Serial.begin(9600);
  pinMode(light, OUTPUT);
  pinMode(aEngineA, OUTPUT);
  pinMode(bEngineA, OUTPUT);
  pinMode(aEngineB, OUTPUT);
  pinMode(bEngineB, OUTPUT);
  pinMode(movementDetectorLight, OUTPUT);
  pinMode(limitSwitchAOpen, INPUT);
  pinMode(limitSwitchAClose, INPUT);
  pinMode(limitSwitchBOpen, INPUT);
  pinMode(limitSwitchBClose, INPUT);
  pinMode(movementDetector, INPUT);
  restartArduino.start();
  lightTimer.setInterval(500);
  lightTimer.setCallback(BlinkLamp);
  movementDetectorLightTimer.setInterval(100);
  movementDetectorLightTimer.setCallback(BlinkMovementDetectorLight);
  RunGate = false;
}

void loop() {
  lightTimer.update();
  restartArduino.update();
  if (restartArduino.getElapsedTime() > 60000)
  {
    Serial.println("Restarting...");
    ESP.restart();
  }
  
  movementDetectorLightTimer.update();
  if (mySwitch.available())
  {
    value = mySwitch.getReceivedValue();
    if (GetButton(aButtons, sizeof(aButtons)/sizeof(aButtons[0]), value)){
        Serial.println("Dostałem sygnał z guzika!");
        t1.setTimeout(timeOutForGates);
        t1.start();
        RunGate = true;
        mySwitch.resetAvailable();
        lightTimer.start();
    }
  }

  if (GetButton(cButtons, sizeof(cButtons)/sizeof(cButtons[0]), value)){
    StopGates();
    actualStatus = GateStopped;
    mySwitch.resetAvailable();
    RunGate = true;
  }

  if (RunGate){
    LookingForMovement();
    
    switch (actualStatus) {
      case GateClosed:
          Serial.println("Wszedłem w GateClosed.");
          MovementFound = false;
          previeusStatus = GateClosed;
          OpenGateA();
          t1.update();
          lightTimer.update();
          t1.setCallback(OpenGateB);
      break;

      case GateOpening:
          Serial.println("Wszedłem w GateOpening.");
          MovementFound = false;
          if (digitalRead(limitSwitchBOpen) == HIGH){
            Serial.println("limitSwitchAOpen - HIGH");
            StopGateA_AfterOpening();
          }
          
          if (digitalRead(limitSwitchAOpen) == HIGH){
            Serial.println("limitSwitchBOpen - HIGH");
            StopGateB_AfterOpening();
          }
      break;

      case GateOpened:
          Serial.println("Wszedłem w GateOpened.");
          MovementFound = false;
          CloseGateB();
          t1.update();
          lightTimer.update();
          t1.setCallback(CloseGateA);
          Serial.println(actualStatus);
      break;

      case GateClosing:
          Serial.println("Wszedłem w GateClosing.");
          MovementFound = false;
          if (digitalRead(limitSwitchBClose) == HIGH){
            Serial.println("limitSwitchBClose - HIGH");
            StopGateA_AfterClosing();
          }
          
          if (digitalRead(limitSwitchAClose) == HIGH){
            Serial.println("limitSwitchAClose - HIGH");
            StopGateB_AfterClosing();
          }
          
          Serial.println(actualStatus);
      break;

      case GateStopped:
          Serial.println("Wszedłem w GateStopped.");
          Serial.println(actualStatus);
          actualStatus = previeusStatus;
          t1.stop();
          RunGate = false;
          MovementFound = false;
      break;

      case MovementDetected:
          Serial.println("Wszedlem w MovementDetected");
          movementDetectorLightTimer.start();
          Serial.println(actualStatus);
          Serial.println("Previeus status:");
          Serial.println(previeusStatus);
          MovementFound = true;
          if (previeusStatus == GateClosing){
            StopGateA_AfterClosing();
            StopGateB_AfterClosing();
          }

          if (previeusStatus == GateOpening)
          {
            StopGateA_AfterOpening();
            StopGateB_AfterOpening();
          }

          t1.stop();
      break;
    }
  }
}

void LookingForMovement(){
  digitalWrite(movementDetectorLight, LOW);
  int value = digitalRead(movementDetector);
  if (value == HIGH){
    Serial.println("Wszedłem tu i zmieniam status na MovementDetected");
    digitalWrite(movementDetectorLight, HIGH);
    previeusStatus = actualStatus;
    actualStatus = MovementDetected;
  }
}

void BlinkMovementDetectorLight(){
  if (MovementFound == false){
    digitalWrite(movementDetectorLight, LOW);
  }
  else{
    if (MovementLedIsOn){
      digitalWrite(movementDetectorLight, HIGH);
    }
    else{
      digitalWrite(movementDetectorLight, LOW);
    }
  
    MovementLedIsOn = !MovementLedIsOn;
  }
}

void BlinkLamp(){
  if (RunGate == false || actualStatus == GateStopped){
    LedIsOn = false;
    lightTimer.stop();
    digitalWrite(light, LOW);
  }
  else{
    if (LedIsOn){
      digitalWrite(light, HIGH);
    }
    else{
      digitalWrite(light, LOW);
    }

      LedIsOn = !LedIsOn;
  }
}

void OpenGateA(){
  digitalWrite(aEngineA, HIGH);
  digitalWrite(bEngineA, LOW);
}

void OpenGateB(){
  digitalWrite(aEngineB, HIGH);
  digitalWrite(bEngineB, LOW);
  actualStatus = GateOpening;
}

void CloseGateA(){
  digitalWrite(aEngineA, LOW);
  digitalWrite(bEngineA, HIGH);
  actualStatus = GateClosing;
}

void CloseGateB(){
  digitalWrite(aEngineB, LOW);
  digitalWrite(bEngineB, HIGH);
}

void StopGateA_AfterOpening(){
  digitalWrite(aEngineA, LOW);
  digitalWrite(bEngineA, LOW);
}

void StopGateA_AfterClosing(){
  digitalWrite(aEngineA, LOW);
  digitalWrite(bEngineA, LOW);
  previeusStatus = GateOpened;
  actualStatus = GateClosed;
  digitalWrite(light, LOW);
  RunGate = false;
}

void StopGateA(){
  digitalWrite(aEngineA, LOW);
  digitalWrite(bEngineA, LOW);
}

void StopGateB_AfterOpening(){
  digitalWrite(aEngineB, LOW);
  digitalWrite(bEngineB, LOW);
  previeusStatus = GateClosed;
  actualStatus = GateOpened;
  digitalWrite(light, LOW);
  RunGate = false;
}

void StopGateB(){
  digitalWrite(aEngineB, LOW);
  digitalWrite(bEngineB, LOW);
}

void StopGateB_AfterClosing(){
  digitalWrite(aEngineB, LOW);
  digitalWrite(bEngineB, LOW); 
}

void StopGates(){
  StopGateA();
  StopGateB();
  digitalWrite(light, LOW);
  RunGate = false;
}

bool GetButton(int buttonsArray[], int buttonsSize, int receivedValue){
   for (int i = 0; i < buttonsSize; i++){
      if (receivedValue == buttonsArray[i]){
        return true;
      }
  }

    return false;
}