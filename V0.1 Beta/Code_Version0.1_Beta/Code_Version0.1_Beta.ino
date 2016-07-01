/*
 * V0.1 Beta
 * Released under CC BY-NC-SA 4.0 https://creativecommons.org/licenses/by-nc-sa/4.0/
 * 
 * MORE INFO ON THE PROJECT PAGE:
 * https://hackaday.io/project/11278-cocito-weather-station
*/
#include <Wire.h>               //libraries
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_AM2315.h>
#include <SoftwareSerial.h>
#include <Adafruit_SleepyDog.h> 

#define SSID "put your ssid here"
#define PASS "put your password here"
String apiKey1 = "put first channel write key here";    //you will need two channels to store all the data from the sensors
String apiKey2 = "put second channel write key here";
#define IP "184.106.153.149"            // thingspeak.com

int ledPin = 13;                 //led connected on pin 13 (it will blink in case of errors)
int transistorPin = 9;           //pin used to turn on and off the ESP8266 module, see schematic for more details 
SoftwareSerial esp8266(10, 11);  // RX, TX   connect tx pin of the ESP8266 to pin 10 on arduino and rx pin of the ESP8266 to pin 11
SoftwareSerial pm25(6, 7); // RX, TX serial connection with laser dust sensor module
Adafruit_BMP280 bmp280;          //BMP280 sensor (pressure and temperature)
Adafruit_AM2315 am2315;           //AM2315 sensor (temperature and humidity)
int pmPin = 4;
int solPin = A3;
int battPin = A2;

float pres, temp, hum, temp2,winds,windd,rains,solvolt,battvolt;

const byte WSPEED = 3;              //wind speed sensor
volatile long lastWindIRQ = 0;
volatile byte windClicks = 0;
long lastWindCheck = 0;

const byte WDIR = A0;               //wind direction sensor

const byte RAIN = 2;                //rain sensor connected to digital pin 2
byte minutes;
volatile unsigned long raintime, rainlast, raininterval, rain;
volatile float dailyrainin; // [rain inches so far today in local time]
volatile float rainHour[60]; //60 floating numbers to keep track of 60 minutes of rain

int PM01Value=0;          //define PM1.0 value of the air detector module
int PM2_5Value=0;         //define PM2.5 value of the air detector module
int PM10Value=0;         //define PM10 value of the air detector module
#define LENG 32

void setup() {
  pinMode(ledPin, OUTPUT);   
  pinMode(transistorPin, OUTPUT); 
  pinMode(pmPin, OUTPUT);
  digitalWrite(transistorPin, LOW);
  digitalWrite(pmPin, LOW);
  esp8266.begin(9600);               //serial communication with ESP8266 module (default 115200)
  pm25.begin(9600);
  bmp280.begin();
  am2315.begin();
  pinMode(WSPEED, INPUT_PULLUP); // input from wind meters windspeed sensor
  attachInterrupt(1, wspeedIRQ, FALLING);
  pinMode(RAIN, INPUT_PULLUP); // input from wind meters rain gauge sensor
  attachInterrupt(0, rainIRQ, FALLING);
  interrupts();
  esp8266.listen();
  //connectWiFi();                     //needed just for the first time (the ESP module will store that in his memory)
}

void loop() {
  digitalWrite(pmPin, HIGH);
  delay(15000);
  pm25.listen();
  pm();
  digitalWrite(pmPin, LOW);
  esp8266.listen();
  getip();                                                           //checks connection to your router
  hum = am2315.readHumidity();
  temp = am2315.readTemperature();
  if (temp==0){                                                      //sometimes the sensor gives 0 as an error, we check 2 times to mame sure the temperature is correct
    temp = am2315.readTemperature();
  }
  if (temp==0){                                                      
    temp = am2315.readTemperature();
  }
  pres = bmp280.readPressure();
  temp2 = bmp280.readTemperature();
  winds = get_wind_speed();
  windd = get_wind_direction();
  rains = dailyrainin;
  solvolt = averageAnalogRead(solPin)*0.00527*(95.81/14.81);
  battvolt = averageAnalogRead(battPin)*0.00552*(96/15);

  upload(apiKey1,pres,temp,hum, temp2,winds,windd,rains);              //(apikey,field1,field2,field3,field4,field5,field6,filed7)
  upload(apiKey2,PM01Value,PM2_5Value,PM10Value, solvolt,battvolt,0,0);
  digitalWrite(transistorPin, LOW);
  delay(550000);       //with that delay it uploads data about every 10 minutes
}



void connectWiFi(){
  digitalWrite(transistorPin, HIGH);
  String cmd="AT+CWJAP=\"";
  cmd+=SSID;
  cmd+="\",\"";
  cmd+=PASS;
  cmd+="\"";
  esp8266.println(cmd);
  unsigned long first;
  int count=0;
  first=millis();
  while(!(esp8266.find("WIFI CONNECTED"))){
      if((millis()-first)>20000){
        digitalWrite(ledPin,HIGH);
        digitalWrite(transistorPin, LOW);
        delay(500);
        digitalWrite(transistorPin, HIGH);
        first=millis();
        count++;
      }
      if (count>5){
        digitalWrite(transistorPin, LOW);
        count=0;
        for (int k=0;k<=20;k++){    //goes in standby for about 160 seconds, then retries
          Watchdog.sleep(); 
        }  
        digitalWrite(transistorPin, HIGH);         
      }
  }
  digitalWrite(transistorPin, LOW);
}


void getip(){
  digitalWrite(transistorPin, HIGH);
  unsigned long first;
  first=millis();
  while(!(esp8266.find("WIFI GOT IP"))){
      if((millis()-first)>20000){
        digitalWrite(ledPin,HIGH);
        digitalWrite(transistorPin, LOW);
        delay(500);
        digitalWrite(transistorPin, HIGH);
        first=millis();
      }     
  }
  digitalWrite(ledPin,LOW);
}


void upload(String apiKey,float p, float t, float h,float t2, float ws, float wd, float r){
  char buf[10], buf1[10], buf2[10],buf3[10],buf4[10],buf5[10],buf6[10],buf7[10];
  dtostrf(p, 3, 2, buf);                 //float need to be changed in to string
  dtostrf(t, 3, 2, buf1);
  dtostrf(h, 3, 2, buf2);
  dtostrf(t2, 3, 2, buf3);
  dtostrf(ws, 3, 2, buf4);
  dtostrf(wd, 3, 2, buf5);
  dtostrf(r, 3, 2, buf6);
  String cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += IP;                          
  cmd += "\",80";
  do{                                //tries to connect while errors are occurring
    esp8266.println(cmd);
    delay(2000);
  }while(esp8266.find("Error"));
  String getStr = "GET /update?api_key=";   //creates the string that will be sent
  getStr += apiKey;
  getStr +="&field1=";
  getStr += buf;
  getStr += "&field2=";
  getStr += buf1;
  getStr += "&field3=";
  getStr += buf2;
  getStr += "&field4=";
  getStr += buf3;
  getStr += "&field5=";
  getStr += buf4;
  getStr += "&field6=";
  getStr += buf5;
  getStr += "&field7=";
  getStr += buf6;
  getStr += "\r\n\r\n";
  cmd = "AT+CIPSEND=";
  cmd += String(getStr.length());           //calculates the length of the tcp request
  esp8266.println(cmd);                         //sends the length to the ESP8266
  delay(2500);

  if(esp8266.find(">")){                        //when the module is ready we send him the data
    esp8266.print(getStr);
  }
  else{
    esp8266.println("AT+CIPCLOSE");
  }
  delay(2000);
}


void wspeedIRQ()
// Activated by the magnet in the anemometer (2 ticks per rotation), attached to input D3
{
  if (millis() - lastWindIRQ > 10) // Ignore switch-bounce glitches less than 10ms (142MPH max reading) after the reed switch closes
  {
    lastWindIRQ = millis(); //Grab the current time
    windClicks++; //There is 1.492MPH for each click per second.
  }
}


float get_wind_speed()
{
  float deltaTime = millis() - lastWindCheck; //750ms

  deltaTime /= 1000.0; //Covert to seconds

  float windSpeed = (float)windClicks / deltaTime; //3 / 0.750s = 4

  windClicks = 0; //Reset and start watching for new wind
  lastWindCheck = millis();
  windSpeed *= 2.4;  //km/h
  return(windSpeed);
}


int averageAnalogRead(int pinToRead)
{
  byte numberOfReadings = 8;
  unsigned int runningValue = 0;

  for(int x = 0 ; x < numberOfReadings ; x++)
    runningValue += analogRead(pinToRead);
  runningValue /= numberOfReadings;

  return(runningValue);
}


int get_wind_direction()
// read the wind direction sensor, return heading in degrees
{
  unsigned int adc;

  adc = averageAnalogRead(WDIR); // get the current reading from the sensor

  // The following table is ADC readings for the wind direction sensor output, sorted from low to high.
  // Each threshold is the midpoint between adjacent headings. The output is degrees for that ADC reading.
  // Note that these are not in compass degree order! See Weather Meters datasheet for more information.

  if (adc < 65) return (158); 
  if (adc < 83) return (203);
  if (adc < 92) return (180);
  if (adc < 126) return (113);
  if (adc < 185) return (135);
  if (adc < 245) return (68);
  if (adc < 288) return (90);
  if (adc < 407) return (248);
  if (adc < 463) return (225);
  if (adc < 600) return (23);
  if (adc < 631) return (45);
  if (adc < 704) return (293);
  if (adc < 787) return (270); 
  if (adc < 829) return (338);
  if (adc < 889) return (315);
  if (adc < 946) return (0); 
  return (-1); // error, disconnected?
}


void rainIRQ()
// Count rain gauge bucket tips as they occur
// Activated by the magnet and reed switch in the rain gauge, attached to input D2
{
  raintime = millis(); // grab current time
  raininterval = raintime - rainlast; // calculate interval between this and last event

  if (raininterval > 10) // ignore switch-bounce glitches less than 10mS after initial edge
  {
    dailyrainin += 0.011; //Each dump is 0.011" of water
    rainHour[minutes] += 0.011; //Increase this minute's amount of rain

    rainlast = raintime; // set up for next event
  }
}


void pm(){
  char buf[LENG];
  delay(2000);
  if(pm25.available()) 
  {
    pm25.readBytes(buf,LENG);
    if(buf[0] == 0x42 && buf[1] == 0x4d){
      if(checkValue(buf,LENG)){
        PM01Value=transmitPM01(buf); //count PM1.0 value of the air detector module
        PM2_5Value=transmitPM2_5(buf);//count PM2.5 value of the air detector module
        PM10Value=transmitPM10(buf); //count PM10 value of the air detector module 
      }           
    } 
  }
}


char checkValue(char *thebuf, char leng)
{  
  char receiveflag=0;
  int receiveSum=0;
  char i=0;
 
  for(i=0;i<leng;i++)
  {
  receiveSum=receiveSum+thebuf[i];
  }
    
  if(receiveSum==((thebuf[leng-2]<<8)+thebuf[leng-1]+thebuf[leng-2]+thebuf[leng-1]))  //check the serial data 
  {
    receiveSum=0;
    receiveflag=1;
  }
  return receiveflag;
}

int transmitPM01(char *thebuf)
{
  int PM01Val;
  PM01Val=((thebuf[4]<<8) + thebuf[5]); //count PM1.0 value of the air detector module
  return PM01Val;
}

 
int transmitPM2_5(char *thebuf)
{
  int PM2_5Val;
  PM2_5Val=((thebuf[6]<<8) + thebuf[7]);//count PM2.5 value of the air detector module
  return PM2_5Val;
}
 

int transmitPM10(char *thebuf)
{
  int PM10Val;
  PM10Val=((thebuf[8]<<8) + thebuf[9]); //count PM10 value of the air detector module  
  return PM10Val;
}
