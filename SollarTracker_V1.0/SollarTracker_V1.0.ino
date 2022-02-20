/*
 * SollarTracker_V1.0
 * 2021-04-02 ~ 2021-04-17
 * 태양광 추적기 프로젝트
 */

#include <Servo.h>
#include <OneWire.h>
#include <ThreeWire.h>
#include <RtcDS1302.h> // "툴-라이브러리 관리"에서 DS1302 검색 후 rtc by makuna 라이브러리 설치

int CTempSensorLeftUp = A3;
int CTempSensorRightUp = A0;
int CTempSensorLeftDown = A1;
int CTempSensorRightDown = A2;

ThreeWire myWire(3, 4, 2); // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);

Servo Yaw;
Servo Pitch;
int ServoYawDefualt = 90;
int ServoPitchDefualt = 45;

int YawPin = 5;
int PitchPin = 6;

int ServoDelay = 200;  // 서보모터 반응속도
int ServoSensitive = 127; // 0 ~ 255

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
  int LeftUp = analogRead(CTempSensorLeftUp);
  int LeftDown = analogRead(CTempSensorLeftDown);
  int RightUp = analogRead(CTempSensorRightUp);
  int RightDown = analogRead(CTempSensorRightDown);

  int AverageUp = (LeftUp + RightUp) / 2;
  int AverageDown = (LeftDown + RightDown) / 2;
  int AverageLeft = (LeftUp + LeftDown) / 2;
  int AverageRight = (RightUp + RightDown) / 2;

  int UDDifference = AverageUp - AverageDown;
  int LRDifference = AverageLeft - AverageRight;

  // 온도 센서
  float Temp = getTemp();


//  Serial.println(Temp);
//  Serial.println(" ");
//  Serial.println(LeftUp);
//  Serial.println(LeftDown);
//  Serial.println(RightUp);
//  Serial.println(RightDown);
//  Serial.println(" ");

  // RTC
  RtcDateTime now = Rtc.GetDateTime();

  // 밑에 펌프관련 코드들을 한시간에 한번만 동작하게 해주는 코드
  if (now.Minute() == 0) {
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

  // 한시간에 한번씩 청소
  if (now.Hour() > 7 && now.Hour() < 21) {
    if (now.Minute() == 0) {
      digitalWrite(Pump_1, HIGH);
      digitalWrite(Pump_2, LOW);
      delay(6000);
      digitalWrite(Pump_1, HIGH);
      digitalWrite(Pump_2, HIGH);
      digitalWrite(Linear_1, HIGH);
      digitalWrite(Linear_2, LOW);
      delay(12000);
      digitalWrite(Linear_1, LOW);
      digitalWrite(Linear_2, HIGH);
      delay(12000);
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
