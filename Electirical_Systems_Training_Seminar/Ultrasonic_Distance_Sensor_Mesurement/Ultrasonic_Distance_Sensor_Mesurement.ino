const int trigPin = 2;
const int echoPin = 3;

double duration = 0;
double distance = 0;

void setup() {
  Serial.begin( 9600 );
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
}
void loop() {
  digitalWrite(trigPin, LOW);
  digitalWrite(echoPin, LOW);
  delayMicroseconds(1);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);   
  duration = duration * 0.000001 * 34000 / 2;
  Serial.print("(超音波距離センサ)距離：");
  Serial.print((int)duration);
  Serial.println(" cm");
  delay(100);
}
