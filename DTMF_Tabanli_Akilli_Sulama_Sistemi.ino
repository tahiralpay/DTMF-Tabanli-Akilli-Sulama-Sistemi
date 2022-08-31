/*                      ARALIK 2020 TALPAY
                      AKILLI SULAMA SİSTEMİ
*******************************************************************
  BAĞLANTI PİNLERİ:
  TOPRAK NEM SENSÖRÜ A0 NOLU ANALOG PİNE
  DHT11 A1 NOLU ANALOG PİNE
  HC SR-04 ECHO PİNİ A2 ANALOG PİNE
  HC SR-04 TRİG PİNİ A3 NOLU ANALOG PİNE
  İ2C SDA A4 NOLU ANALOG PİNE
  İ2C SCL A5 NOLU ANALOG PİNE

  GSM MODÜLÜ TX PİNİ 3 NOLU DİGİTAL PİNE
  GSM MODÜLÜ RX PİNİ 2 NOLU DİGİTAL PİNE
*******************************************************************
  GEREKLİ KÜTÜPHANELER:
``https://www.arduino.cc/reference/en/libraries/dht-sensor-library/``
``https://www.arduino.cc/reference/en/libraries/liquidcrystal-i2c/``
``https://www.arduino.cc/reference/en/libraries/l293/``
*/

#include "SoftwareSerial.h"
#include <AFMotor.h>
#include <Wire.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>

#define DHTTYPE DHT11

#define SIM800L_Tx 3
#define SIM800L_Rx 2

const int prob = A0;
#define DHTpin A1
#define echoPin A2
#define trigPin A3

AF_DCMotor motor1(1);
AF_DCMotor motor2(2);
AF_DCMotor motor3(3);

LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial SIM800L(SIM800L_Tx, SIM800L_Rx);
DHT dht(DHTpin, DHTTYPE);

int toprak_durum;
String smsMetni = "";
float t, h;
String i;
char dtmf_cmd;
bool call_flag = false;
bool motor1_STATE = false;
long sure, mesafe;

void init_gsm();
void control();
void SMSgonder();

/********************** SİM800L KONTROL FONKSİYONU *********************/
void SMSgonder(String mesaj) {
  SIM800L.print("AT+CMGF=1\r");
  delay(100);
  SIM800L.println("AT+CMGS=\"+905377324141\"");// 1.sahıs telefon numarasi
  delay(100);
  SIM800L.println(mesaj);
  delay(100);
  SIM800L.println((char)26);
  delay(100);
  SIM800L.println();
  delay(100);
  SIM800L.println("AT+CMGD=1,4");
  delay(100);
  SIM800L.println("AT+CMGF=1");
  delay(100);
  SIM800L.println("AT+CNMI=1,2,0,0,0");
  delay(200);
  smsMetni = "";
}

void setup()
{
  SIM800L.begin(9600);
  Serial.begin(9600);
  dht.begin();
  lcd.init();

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  motor1.setSpeed(255);
  motor2.setSpeed(255);
  motor3.setSpeed(255);

  motor1.run(RELEASE);
  motor2.run(RELEASE);
  motor3.run(RELEASE);

  init_gsm();
  smsMetni = "AKILLI SULAMA SISTEMI             Suleyman HOCAOGLU";
  SMSgonder(smsMetni);
}

void loop() {
  String gsm_data;
  int x = -1;

  while (SIM800L.available())
  {
    char c = SIM800L.read();
    gsm_data += c;
    delay(10);
  }

  if (call_flag)
  {
    x = gsm_data.indexOf("DTMF");
    if (x > -1)
    {
      dtmf_cmd = gsm_data[x + 6];
      Serial.println(dtmf_cmd);
      control();
    }

    x = gsm_data.indexOf("NO CARRIER");
    if (x > -1)
    {
      SIM800L.println("ATH");
      call_flag = false;
    }
  }
  else {

    x = gsm_data.indexOf("RING");
    if (x > -1)
    {
      delay(5000);
      SIM800L.println("ATA");
      call_flag = true;
    }
  }

  /********************** ÖLÇÜMLERİN YAPILMASI ************************************/
  h = dht.readHumidity();
  t = dht.readTemperature();
  toprak_durum = analogRead(prob);

  Serial.print("toprak islakligi ");
  Serial.println(toprak_durum);
  delay(500);

  Serial.print("hava sicakliği: ");
  Serial.print(t);
  Serial.println("C");
  Serial.print("havadaki nem: ");
  Serial.print(h);
  Serial.println("%");

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  sure = pulseIn(echoPin, HIGH);
  mesafe = (sure / 2) / 29.1;
  Serial.print("mesafe: ");
  Serial.println(mesafe);

  if (toprak_durum <= 500) {
    i = "toprak islak";
    Serial.println(i);

    lcd.clear();
    lcd.setCursor(2, 1);
    lcd.print("toprak islak");
    lcd.home();
    lcd.print("T=");
    lcd.print(t);
    lcd.print("C");
    lcd.print("H=");
    lcd.print(h);
    lcd.print("%");
    delay(1000);
  }

  if (toprak_durum > 500 and toprak_durum < 800 ) {
    i = "toprak nemli";
    Serial.println(i);

    lcd.clear();
    lcd.setCursor(2, 1);
    lcd.print("toprak nemli");
    lcd.home();
    lcd.print("T=");
    lcd.print(t);
    lcd.print("C");
    lcd.print("H=");
    lcd.print(h);
    lcd.print("%");
    delay(1000);
  }

  if (toprak_durum >= 801) {
    i = "toprak kuru";
    Serial.println(i);

    lcd.clear();
    lcd.setCursor(2, 1);
    lcd.print("toprak kuru");
    lcd.home();
    lcd.print("T=");
    lcd.print(t);
    lcd.print("C");
    lcd.print("H=");
    lcd.print(h);
    lcd.print("%");
    delay(1000);
  }

  /************* KUYUDAN BİDONA SU AKTARAN MOTOR KONTROL ************/
  if (mesafe > 100 ) {
    motor2.run(FORWARD);
    motor3.run(RELEASE);
    Serial.println("MOTOR 2 ACILDI");
    Serial.println("  ");
  }

  if (mesafe < 20 ) {
    motor2.run(RELEASE);
    Serial.println("MOTOR 2 KAPATILDI");
    Serial.println("  ");
  }

  /************* KUYUDAN TARLAYA SU AKTARAN MOTORUN KAPATILMASI ************/
  if (toprak_durum < 500) {
    motor1.run(RELEASE);
    Serial.println("MOTOR 1 KAPATILDI");
  }

}

/********************** GSM MODÜLÜ KONTROL FONKSİYONU *********************/
void init_gsm()
{
  boolean gsm_Ready = 1;
  Serial.println("initializing GSM module");
  while (gsm_Ready > 0)
  {
    SIM800L.println("AT");
    Serial.println("AT");
    while (SIM800L.available())
    {
      if (SIM800L.find("OK") > 0)
        gsm_Ready = 0;
    }
    delay(2000);
  }
  Serial.println("AT READY");

  boolean ntw_Ready = 1;
  Serial.println("finding network");
  while (ntw_Ready > 0)
  {
    SIM800L.println("AT+CPIN?");
    Serial.println("AT+CPIN?");
    while (SIM800L.available())
    {
      if (SIM800L.find("+CPIN: READY") > 0)
        ntw_Ready = 0;
    }
    delay(2000);
  }
  Serial.println("NTW READY");
  boolean DTMF_Ready = 1;
  Serial.println("turning DTMF ON");
  while (DTMF_Ready > 0)
  {
    SIM800L.println("AT+DDET=1");
    Serial.println("AT+DDET=1");
    while (SIM800L.available())
    {
      if (SIM800L.find("OK") > 0)
        DTMF_Ready = 0;

    }
    delay(2000);
  }
  Serial.println("DTMF READY");
}

/**************************** DTMF KONTROL *****************************/
void control()
{
  /********************** TÜM MOTORLARIN KAPATILMASI *********************/
  if (dtmf_cmd == '0') {
    motor1.run(RELEASE);
    motor2.run(RELEASE);
    motor3.run(RELEASE);
    Serial.println("Tüm motorlar kapatıldı");
    smsMetni = "Tum motorlar kapatildi";
    SMSgonder(smsMetni);
  }

  /******************** TARLAYA SU VEREN MOTORLARUN AÇILMASI *************/
  if (dtmf_cmd == '1') {
    motor1.run(FORWARD);
    motor2.run(RELEASE);
    motor3.run(RELEASE);
    Serial.println("MOTOR 1 ACILDI");
    smsMetni = "Tarlaya su veren motor acildi";
    SMSgonder(smsMetni);
  }

  /******************** TARLAYA İLAÇ VEREN MOTORLARUN AÇILMASI VE KAPATILMASI *************/
  if (dtmf_cmd == '2') {
    motor3.run(FORWARD);
    motor1.run(RELEASE);
    motor2.run(RELEASE);
    Serial.println("MOTOR 3 ACILDI");
    smsMetni = "Tarlaya ilac veren motor acildi";
    SMSgonder(smsMetni);
    delay(3000);
    motor3.run(RELEASE);
    Serial.println("MOTOR 3 KAPATILDI");
    smsMetni = "Tarlaya ilac veren motor kapatildi";
    SMSgonder(smsMetni);
    motor1.run(FORWARD);
    delay(1000);
  }

  /********************HAVA SICAKLIĞI,NEMİ VE TOPRAK DURMUNUN SMS İLE GÖNDERİLMESİ *************/
  if (dtmf_cmd == '3') {
    smsMetni = "Havanin sicakligi = " + String(t, 1) + " C" + " \nHavanin nem orani = " + String(h, 1) + " %" + " \nToprak durum =" + String(i);
    SMSgonder(smsMetni);
  }

}
