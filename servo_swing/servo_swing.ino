#include <Arduino.h>
#include <Servo.h>

Servo myservo;

int i = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  myservo.attach(2);
}

void loop() {
  // put your main code here, to run repeatedly:
  if(i == 0){
    for(i = 0; i < 180; i ++){
      myservo.write(i);
      delay(10);
    }
  }
  if(i > 180){
    for(i = 180; i > 0; i--){
      myservo.write(i);
      delay(10);
    }
  }
}
