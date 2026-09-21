void setup() {
  // put your setup code here, to run once:
Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
int a = analogRead(A2); 
a = 6787 / (a - 3) - 4;
Serial.print("（赤外線距離センサ）距離：");
Serial.print(a);
Serial.println("cm");
delay(100);

}
