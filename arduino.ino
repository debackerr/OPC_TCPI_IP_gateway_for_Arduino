#include <OPC.h>

OPCSerial aOPCSerial;

int ledRed = 13;
int ledGreen = 10;
int ledYellow = 9;


bool callback1(const char *itemID, const opcOperation opcOP, const bool value) {
  static bool ledValue = false;
  if (opcOP == opc_opwrite) {
    ledValue = value;
      if(ledValue == 1){
        digitalWrite(ledRed, HIGH);
      } else {
        digitalWrite(ledRed, LOW);
      }
  }  else {
      ledValue = digitalRead(ledRed);
      return ledValue;
  }      
}


bool callback2(const char *itemID, const opcOperation opcOP, const bool value) {
  static bool ledValue = false;
  if (opcOP == opc_opwrite) {
    ledValue = value;
      if(ledValue == 1){
        digitalWrite(ledGreen, HIGH);
      } else {
        digitalWrite(ledGreen, LOW);
      }
  }  else {
      ledValue = digitalRead(ledGreen);
      return ledValue;
  }      
}


bool callback3(const char *itemID, const opcOperation opcOP, const bool value) {
  static bool ledValue = false;
  if (opcOP == opc_opwrite) {
    ledValue = value;
      if(ledValue == 1){
        digitalWrite(ledYellow, HIGH);
      } else {
        digitalWrite(ledYellow, LOW);
      }
  }  else {
      ledValue = digitalRead(ledYellow);
      return ledValue;
  }      
}

void setup() {

  Serial.begin(9600);
  pinMode(ledRed, OUTPUT);
  pinMode(ledGreen, OUTPUT);
  pinMode(ledYellow, OUTPUT);

  aOPCSerial.setup();
  aOPCSerial.addItem("lr", opc_readwrite, opc_bool, callback1);
  aOPCSerial.addItem("lg", opc_readwrite, opc_bool, callback2);
  aOPCSerial.addItem("ly", opc_readwrite, opc_bool, callback3);

}

void loop() {
  aOPCSerial.processOPCCommands();
}
