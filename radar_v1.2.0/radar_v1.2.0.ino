#include <Arduino.h>
#include <Servo.h>

Servo myservo;

int i = 0;
int a = 0;
float k = 0;
float x = 0;
float y = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  myservo.attach(2);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (i == 0) {
    for (i = 0; i < 180; i++) {
      myservo.write(i);
      a = analogRead(A0);
      k = 6787 / (a - 3.0) - 4;
      x = k * cos(i * PI / 180.0);
      y = k * sin(i * PI / 180.0);
      Serial.print(x);
      Serial.print(",");
      Serial.println(y);
      delay(20);
    }
  }
  if (i == 180) {
    for (i = 180; i > 0; i--) {
      myservo.write(i);
      a = analogRead(A0);
      k = 6787 / (a - 3.0) - 4;
      x = k * cos(i * PI / 180.0);
      y = k * sin(i * PI / 180.0);
      Serial.print(x);
      Serial.print(",");
      Serial.println(y);
      delay(20);
    }
  }
}
