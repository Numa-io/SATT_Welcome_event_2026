//1年生向け電装講習会用
//サーボモーター 可変抵抗制御プログラム
#include <Servo.h>

#define SERVO_SIG_PIN 9
#define VR_PIN A0

Servo myservo;

int angle;
int VR_value;

void READ_VR(void);
void Set_Angle(void);

void setup() {
  myservo.attach(SERVO_SIG_PIN);
  Serial.begin(9600);
}

void loop() {
  READ_VR();
  Set_Angle();
  myservo.write(angle);
  /*検証用
  Serial.print("angle=");
  Serial.println(angle);
  Serial.print("VR_value=");
  Serial.println(VR_value);
  */
  delay(10);
}

void READ_VR(void){
  VR_value = analogRead(VR_PIN);
}

void Set_Angle(void){
  angle = 180-(180.0* VR_value / 1023.0);
}

//計算途中もintで処理されるから桁あふれに注意