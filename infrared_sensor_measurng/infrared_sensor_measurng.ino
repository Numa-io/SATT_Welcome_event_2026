void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  float k = 0;
  int a = analogRead(A0);
  k = 6787 / (a - 3.0) - 4;
  Serial.print("A0=");
  Serial.println(k);

  delay(100);
}
