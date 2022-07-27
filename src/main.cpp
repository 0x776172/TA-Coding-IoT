#include <Arduino.h>
#include <WiFiMulti.h>
#include <FirebaseESP32.h>
#include <BH1750.h>
#include <Adafruit_INA219.h>
#include <Wire.h>

#define SSID1 "R13"
#define PASS1 "enerpe31"
#define SSID2 "add"
#define PASS2 "11223344"
#define SSID3 "Ular Sanca"
#define PASS3 "12093487"

#define fhost "buatproyekakhir-default-rtdb.asia-southeast1.firebasedatabase.app"
#define fauth "0cEEtQH2xpGcy8uZCeNFCGlFVXKdwQmrLhvWM1X0"

WiFiMulti ap;
FirebaseData fbdo;
FirebaseJson fjson;

BH1750 lux;
Adafruit_INA219 sensor1(0x40);
Adafruit_INA219 sensor2(0x41);
Adafruit_INA219 sensor3(0x44);

String path = "/data";
unsigned long interval = 90000;
unsigned long checkInterval = 10000;
unsigned long prevTime = 0;
unsigned long prevCheckTime = 0;

float valLight = 0;
float sV_panel1, bus_panel1, panel1;
float sV_panel2, bus_panel2, panel2;
float sV_panel3, bus_panel3, panel3;
bool isCleaning = true;

void wifiSetup();
void FirebaseSetup();
bool sendDatabase(FirebaseJson jsonObj, float lightIntensity, float panel1, float panel2, float panel3);

double eqPv1(double lux);
bool isPv1Dirty(double lux, double VReal);
double eqPv0(double lux);
bool isPv0Dirty(double lux, double VReal);

void setup()
{
  Serial.begin(115200);
  Serial2.begin(115200);
  Wire.begin();
  wifiSetup();
  FirebaseSetup();
  // pengecekan sensor yang digunakan, yaitu INA219 3 buah, dan sensor cahaya 1 buah
  if (!lux.begin())
  {
    Serial.println("BH1750 tidak terdeteksi");
  }
  else
  {
    Serial.println("BH1750 Terdeteksi");
  }
  if (!sensor1.begin())
  {
    Serial.println("INA 1 tidak terdeteksi");
  }
  else
  {
    Serial.println("INA 1 Terdeteksi");
  }
  if (!sensor2.begin())
  {
    Serial.println("INA 2 tidak terdeteksi");
  }
  else
  {
    Serial.println("INA 2 Terdeteksi");
  }
  if (!sensor3.begin())
  {
    Serial.println("INA 3 tidak terdeteksi");
  }
  else
  {
    Serial.println("INA 3 Terdeteksi");
  }
}

void loop()
{
  unsigned long currTime = millis();

  if (currTime - prevCheckTime >= checkInterval)
  {
    valLight = lux.readLightLevel();

    sV_panel1 = sensor1.getShuntVoltage_mV();
    bus_panel1 = sensor1.getBusVoltage_V();
    panel1 = bus_panel1 + (sV_panel1 / 1000);

    sV_panel2 = sensor2.getShuntVoltage_mV();
    bus_panel2 = sensor2.getBusVoltage_V();
    panel2 = bus_panel2 + (sV_panel2 / 1000);

    sV_panel3 = sensor3.getShuntVoltage_mV();
    bus_panel3 = sensor3.getBusVoltage_V();
    panel3 = bus_panel3 + (sV_panel3 / 1000);

    Serial.println(panel1);
    Serial.println(panel2);
    Serial.println(panel3);
    Serial.println(valLight);

    if (!sendDatabase(fjson, valLight, panel1, panel2, panel3))
      Serial.printf("Error reason: %s\n", fbdo.errorReason().c_str());

    else
      Serial.println("Sukses");

    if (!isCleaning) // pengecekan apakah arduino sedang tidak membersihkan
    {
      Serial.println("Arduino is available");
      // pengecekan apakah panel surya kotor
      if (valLight > 1000 && (panel1 > 0 && panel2 > 0 && panel3 > 0))
      {
        if (isPv1Dirty(valLight, panel1))
        {
          isCleaning = true;
          Serial.println("P1;");
          Serial2.println("P1;");
        }
        else if (isPv0Dirty(valLight, panel2))
        {
          isCleaning = true;
          Serial.println("P2;");
          Serial2.println("P2;");
        }
        else if (isPv0Dirty(valLight, panel3))
        {
          isCleaning = true;
          Serial.println("P3;");
          Serial2.println("P3;");
        }
      }
    }
    else
    {
      Serial.println("Arduino is busy");
    }

    prevCheckTime = currTime;
  }
}

void serialEvent2()
{
  switch (Serial2.read())
  {
  case 'D':
    isCleaning = Serial2.parseInt() == 1 ? false : true;
    break;

  default:
    break;
  }
}

void wifiSetup()
{
  ap.addAP(SSID1, PASS1);
  ap.addAP(SSID2, PASS2);
  ap.addAP(SSID3, PASS3);
  Serial.print("Connecting");
  while (ap.run() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.print("Connected to: ");
  Serial.println(WiFi.SSID());
}

void FirebaseSetup()
{
  Firebase.begin(fhost, fauth);
  Serial.println("firebase init");
  Firebase.reconnectWiFi(true);
}

bool sendDatabase(FirebaseJson jsonObj, float lightIntensity, float panel1, float panel2, float panel3)
{
  jsonObj.set("Timestamp/.sv", "timestamp");
  jsonObj.set("lightIntensity", lightIntensity);
  jsonObj.set("panel1", panel1);
  jsonObj.set("panel2", panel2);
  jsonObj.set("panel3", panel3);
  return Firebase.pushJSON(fbdo, path, jsonObj);
}

bool isPv1Dirty(double lux, double VReal)
{
  bool isDirty;
  double resV = eqPv1(lux);
  if ((resV - (resV * 0.15)) > VReal)
    isDirty = true;
  else
    isDirty = false;
  return isDirty;
}

double eqPv1(double lux)
{
  double eqLux = log(lux);
  double res = -0.189 * pow(eqLux, 2) + 4.1348 * eqLux - 2.4183;
  return res;
}

bool isPv0Dirty(double lux, double VReal)
{
  bool isDirty;
  double resV = eqPv0(lux);
  if ((resV - (resV * 0.15)) > VReal)
    isDirty = true;
  else
    isDirty = false;
  return isDirty;
}

double eqPv0(double lux)
{
  double eqLux = log(lux);
  double res = -0.172 * pow(eqLux, 2) + 4.08 * eqLux - 3.758;
  return res;
}