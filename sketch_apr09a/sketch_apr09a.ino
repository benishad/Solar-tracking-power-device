#include <Servo.h>
#include <OneWire.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>

int CdsLeftUp = A0;
int CdsRightUp = A1;
int CdsLeftDown = A2;
int CdsRightDown = A3;

ThreeWire myWire(3, 4, 2); // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);

Servo Yaw;
int ServoYawDefualt = 90;
Servo Pitch;
int ServoPitchDefualt = 45;

int YawPin = 5;
int PitchPin = 6;

int ServoDelay = 100;  // 서보모터 반응속도
int ServoSensitive = 127; // 0 ~ 255

int TempPin = 7;
OneWire ds(TempPin);

int Linear_1 = 8;
int Linear_2 = 9;
int Pump_1 = 10;
int Pump_2 = 11;

void setup() {
  Serial.begin(9600);

  Serial.print("compiled: ");
  Serial.print(__DATE__);
  Serial.println(__TIME__);

  Rtc.Begin();

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  printDateTime(compiled);
  Serial.println();

  if (!Rtc.IsDateTimeValid())
  {
    Serial.println("RTC lost confidence in the DateTime!");
    Rtc.SetDateTime(compiled);
  }

  if (Rtc.GetIsWriteProtected())
  {
    Serial.println("RTC was write protected, enabling writing now");
    Rtc.SetIsWriteProtected(false);
  }

  if (!Rtc.GetIsRunning())
  {
    Serial.println("RTC was not actively running, starting now");
    Rtc.SetIsRunning(true);
  }

  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled)
  {
    Serial.println("RTC is older than compile time!  (Updating DateTime)");
    Rtc.SetDateTime(compiled);
  }
  else if (now > compiled)
  {
    Serial.println("RTC is newer than compile time. (this is expected)");
  }
  else if (now == compiled)
  {
    Serial.println("RTC is the same as compile time! (not expected but all is fine)");
  }

  Yaw.attach(YawPin);
  Pitch.attach(PitchPin);

  pinMode(TempPin, INPUT);

  pinMode(Linear_1, OUTPUT);
  pinMode(Linear_2, OUTPUT);
  pinMode(Pump_1, OUTPUT);
  pinMode(Pump_2, OUTPUT);

}

void loop() {
  int LeftUp = analogRead(CdsLeftUp);
  int LeftDown = analogRead(CdsLeftDown);
  int RightUp = analogRead(CdsRightUp);
  int RightDown = analogRead(CdsRightDown);

  int AverageUp = (LeftUp + RightUp) / 2;
  int AverageDown = (LeftDown + RightDown) / 2;
  int AverageLeft = (LeftUp + LeftDown) / 2;
  int AverageRight = (RightUp + RightDown) / 2;

  int UDDifference = AverageUp - AverageDown;
  int LRDifference = AverageLeft - AverageRight;

  float Temp = getTemp();
  Serial.println(Temp);
  Serial.println(" ");

  Serial.println(LeftUp);
  Serial.println(LeftDown);
  Serial.println(RightUp);
  Serial.println(RightDown);
  Serial.println(" ");

  // RTC
  RtcDateTime now = Rtc.GetDateTime();

  printDateTime(now);
  Serial.println();

  if (!now.IsValid())
  {
    // Common Causes:
    //    1) the battery on the device is low or even missing and the power line was disconnected
    Serial.println("RTC lost confidence in the DateTime!");
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
      // nothing
    }
    Yaw.write(ServoYawDefualt);
  }
  delay(ServoDelay);
}

float getTemp() {

  byte data[12];
  byte addr[8];

  if ( !ds.search(addr)) {
    //no more sensors on chain, reset search
    ds.reset_search();
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

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1); // start conversion, with parasite power on at the end

  byte present = ds.reset();
  ds.select(addr);
  ds.write(0xBE); // Read Scratchpad


  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds.read();
  }

  ds.reset_search();

  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;

  return TemperatureSum;

}

#define countof(a) (sizeof(a) / sizeof(a[0]))

void printDateTime(const RtcDateTime& dt)
{
  char datestring[20];

  snprintf_P(datestring,
             countof(datestring),
             PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
             dt.Month(),
             dt.Day(),
             dt.Year(),
             dt.Hour(),
             dt.Minute(),
             dt.Second() );
  Serial.print(datestring);
}

/*
  digitalWrite(Linear_1, HIGH);
  digitalWrite(Linear_2, LOW);
  digitalWrite(Pump_1, HIGH);
  digitalWrite(Pump_2, LOW);
  delay(12000);
  digitalWrite(Linear_1, LOW);
  digitalWrite(Linear_2, HIGH);
  digitalWrite(Pump_1, HIGH);
  digitalWrite(Pump_2, HIGH);
  delay(12000);
*/
