/*
   SollarTracker_V1.3
   2021-04-09 ~ 2021-04-17
   태양광 추적기 프로젝트
*/

#include <Servo.h>
#include <OneWire.h> // "툴-라이브러리 관리"에서 onewire 검색 후 DS18B20_RT, onewire(밑으로 스크롤) 라이브러리 설치
#include <ThreeWire.h>
#include <RtcDS1302.h> // "툴-라이브러리 관리"에서 DS1302 검색 후 rtc by makuna 라이브러리 설치

int cdsLeftUp = A2;
int cdsRightUp = A3;
int cdsLeftDown = A0;
int cdsRightDown = A1;

ThreeWire myWire(3, 4, 2); // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);

Servo Yaw;
Servo Pitch;
int ServoYawDefualt = 90;
int ServoPitchDefualt = 90;

int YawPin = 5;
int PitchPin = 6;

int ServoDelay = 35;  // 서보모터 반응속도
int ServoSensitive = 200; // 0 ~ 255

int TempPin = 7;
OneWire TempSensor(TempPin);

int Linear_1 = 8;
int Linear_2 = 9;
int Pump_1 = 10;
int Pump_2 = 11;

int pumpCount = 0;
int cleanerCount = 0;

void setup() {
  Serial.begin(9600);
  Rtc.Begin();
  Yaw.attach(YawPin);
  Pitch.attach(PitchPin);
  pinMode(TempPin, INPUT);
  pinMode(Linear_1, OUTPUT);
  pinMode(Linear_2, OUTPUT);
  pinMode(Pump_1, OUTPUT);
  pinMode(Pump_2, OUTPUT);

}

void loop() {
  int LeftUp = analogRead(cdsLeftUp);
  int LeftDown = analogRead(cdsLeftDown);
  int RightUp = analogRead(cdsRightUp);
  int RightDown = analogRead(cdsRightDown);

  int AverageUp = (LeftUp + RightUp) / 2;
  int AverageDown = (LeftDown + RightDown) / 2;
  int AverageLeft = (LeftUp + LeftDown) / 2;
  int AverageRight = (RightUp + RightDown) / 2;

  int UDDifference = AverageUp - AverageDown;
  int LRDifference = AverageLeft - AverageRight;

  // 온도 센서
  float Temp = getTemp();


  Serial.println(Temp);
  Serial.println(" ");
  Serial.println(LeftUp);
  Serial.println(RightUp);
  Serial.println(LeftDown);
  Serial.println(RightDown);
  Serial.println(" ");

  // RTC
  RtcDateTime now = Rtc.GetDateTime();

  // 밑에 펌프관련 코드들을 한시간에 한번만 동작하게 해주는 코드
  if
  (
    now.Minute() == 1 || now.Minute() == 6 ||
    now.Minute() == 11 || now.Minute() == 16 ||
    now.Minute() == 21 || now.Minute() == 26 ||
    now.Minute() == 31 || now.Minute() == 36 ||
    now.Minute() == 41 || now.Minute() == 46 ||
    now.Minute() == 51 || now.Minute() == 56
  )
  {
    pumpCount = 0;
    cleanerCount = 0;
  }

  // 온도 40도 이상 시, 물뿌림;
  if (Temp > 40) {
    digitalWrite(Pump_1, HIGH);
    digitalWrite(Pump_2, LOW);
    delay(6000);
    digitalWrite(Pump_1, HIGH);
    digitalWrite(Pump_2, HIGH);

    pumpCount++;
  }

  // 5분에 한번씩 청소
  if
  (
    now.Minute() == 0 || now.Minute() == 5 ||
    now.Minute() == 10 || now.Minute() == 15 ||
    now.Minute() == 20 || now.Minute() == 25 ||
    now.Minute() == 30 || now.Minute() == 35 ||
    now.Minute() == 40 || now.Minute() == 45 ||
    now.Minute() == 50 || now.Minute() == 55
  )
  {
    if (cleanerCount == 0) 
    {
      digitalWrite(Pump_1, HIGH);
      digitalWrite(Pump_2, LOW);
      delay(10000);
      digitalWrite(Pump_1, HIGH);
      digitalWrite(Pump_2, HIGH);
      delay(1000);
      digitalWrite(Linear_1, HIGH);
      digitalWrite(Linear_2, LOW);
      delay(10000);
      digitalWrite(Linear_1, LOW);
      digitalWrite(Linear_2, HIGH);
      delay(10000);
      digitalWrite(Linear_1, HIGH);
      digitalWrite(Linear_2, HIGH);

      cleanerCount++;
    }
  }

  // 태양광 추적
  if (-1 * ServoSensitive > UDDifference || UDDifference > ServoSensitive) {
    if (AverageUp < AverageDown) {
      ServoPitchDefualt = ++ServoPitchDefualt;
      if (ServoPitchDefualt > 180) {
        ServoPitchDefualt = 180;
      }
    }
    else if (AverageUp > AverageDown) {
      ServoPitchDefualt = --ServoPitchDefualt;
      if (ServoPitchDefualt < 0) {
        ServoPitchDefualt = 0;
      }
    }
    Pitch.write(ServoPitchDefualt);
  }

  if (-1 * ServoSensitive > LRDifference || LRDifference > ServoSensitive) {
    if (AverageLeft > AverageRight) {
      ServoYawDefualt = --ServoYawDefualt;
      if (ServoYawDefualt < 0) {
        ServoYawDefualt = 0;
      }
    }
    else if (AverageLeft < AverageRight) {
      ServoYawDefualt = ++ServoYawDefualt;
      if (ServoYawDefualt > 180) {
        ServoYawDefualt = 180;
      }
    }
    else if (AverageLeft == AverageRight) {

    }
    Yaw.write(ServoYawDefualt);
  }
  delay(ServoDelay);
}

float getTemp() {

  byte data[12];
  byte addr[8];

  if ( !TempSensor.search(addr)) {
    //no more sensors on chain, reset search
    TempSensor.reset_search();
    return -1000;
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
    Serial.println("CRC is not valid!");
    return -1000;
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
    Serial.print("Device is not recognized");
    return -1000;
  }

  TempSensor.reset();
  TempSensor.select(addr);
  TempSensor.write(0x44, 1); // start conversion, with parasite power on at the end

  byte present = TempSensor.reset();
  TempSensor.select(addr);
  TempSensor.write(0xBE); // Read Scratchpad


  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = TempSensor.read();
  }

  TempSensor.reset_search();

  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;

  return TemperatureSum;

}
