int cdsLeftUp = A0;
int cdsRightUp = A1;
int cdsLeftDown = A2;
int cdsRightDown = A3;

void setup() {
  Serial.begin(9600);

}

void loop() {
  int LeftUp = analogRead(cdsLeftUp);
  int LeftDown = analogRead(cdsLeftDown);
  int RightUp = analogRead(cdsRightUp);
  int RightDown = analogRead(cdsRightDown);

  Serial.print(cdsLeftUp);
  Serial.print(" ");
  Serial.print(cdsLeftDown);
  Serial.print(" ");
  Serial.print(cdsRightUp);
  Serial.print(" ");
  Serial.println(cdsRightDown);
  delay(100);
}
