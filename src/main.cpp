#include <UIPEthernet.h>
#include <OneWire.h>
#include <stdio.h>
#include <DallasTemperature.h>
#include <DS1307RTC.h>
#include <Time.h>
#include <DHT.h>
#include <BH1750FVI.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <PCF8574.h>

//EEPROM address
#define EEPROM_ADDR 0x50
#define morningTimeA 0
#define eveningTimeA 1
#define humidityFanA 2
#define tempFanA 3
#define frostA 4
#define lowTempHeaterA 5
#define hightTempHeaterA 6
#define darkA 7
#define lightA 8
#define brightA 9
#define clearA 10

const int loga[64] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 16, 18, 22, 25, 28, 29, 30, 33, 36, 39, 42, 46, 49, 53, 56, 60, 64, 68, 72, 77, 81, 86, 90, 95, 100, 105, 110, 116, 121, 127, 132, 138, 144, 150, 156, 163, 169, 176, 182, 189, 196, 203, 210, 218, 225, 233, 240, 248, 255}; //уровни яркости заметные глазу

const char *endstringlable = "</label></td><td><input type='number' value='";
const char *tdtr = "</td></tr>";

int lux;
int lampIndex = 0;
int tempBox;
//int tempIn2;
//int tempOut2;
int set1i; 
int set2i;
int tempIn1;
int tempOut1;
int humidityIn;
int humidityOut;
int darkI = 0;
unsigned long sensorMilis;
unsigned long pirSensorMilis;
unsigned long currentMillis;
unsigned long lampMilis;
unsigned long darkMilis;
unsigned long buttonMilis;
unsigned long buttonMilisInterval = 30;

int pirDelay;
int lamp = 0;
bool lampUp = 0;
bool lampDown = 0;
bool doorCloseTrigger = 0;
bool doorOpen = 0;
bool doorOpenTrigger = 0;
bool doorUp = 0;
bool doorDown = 0;
bool fan = 0;
bool heat = 0;

bool manualButton = 0;
bool lockButton = 0;
bool heatButton = 0;
bool fanButton = 0;
bool lampButton = 0;
bool doorButton = 0;

bool heatLock = 0;
bool fanLock = 0;
bool lampLock = 0;
bool doorLock = 0;


LiquidCrystal_I2C lcd(0x27, 20, 4); // встановлюємо адресу LCD в 0x20 для дисплею з 20 символами в 4 рядках

//датчик освещенности
BH1750FVI LightSensor;

//Датчик температуры и влажности DHT 22  (AM2302)
#define DHTIN 34      // вывод, к которому подключается датчик
#define DHTOUT 36     // вывод, к которому подключается датчик
#define DHTTYPE DHT22 // DHT 22  (AM2302)
DHT dhtIn(DHTIN, DHTTYPE);
DHT dhtOut(DHTOUT, DHTTYPE);

//ONEWIRE  термометры  DS18B20
#define ONE_WIRE_BUS 23
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
//DeviceAddress ThermometerIn = {  0x28, 0xFF, 0xE4, 0x19, 0x32, 0x17, 0x04, 0x54 };  // адрес датчика DS18B20 внутри
//DeviceAddress ThermometerOut = { 0x28, 0xFF, 0x8D, 0x21, 0xA3, 0x17, 0x04, 0x61  }; // адрес датчика DS18B20 снаружи  ;
DeviceAddress ThermometerBox = {0x28, 0xFF, 0x8D, 0x2F, 0x01, 0x17, 0x04, 0xCD}; // адрес датчика DS18B20 внутри бокса

#define manualButtonPin 1
#define doorButtonPin 2
#define lampButtonPin 3
#define heatButtonPin 4
#define fanButtonPin 5
#define lockButtonPin 6
//Свет на L298N
#define LAMP 10 // ENA подключен к выводу 10

//Двигатель двери на L298N
#define OPENDOOR 8  // Input3 подключен к выводу 6
#define CLOSEDOOR 6 // Input4 подключен к выводу 7

//модуль 2 реле
#define FAN 2    // IN1 подключен к выводу 11
#define HEATER 4 // IN2 подключен к выводу 12

int doorOpenPin = 27;
int doorClosePin = 31;
#define bufferMax 128
// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
    0xDE, 0xAD, 0xBE, 0xEF, 0x11, 0xED};
IPAddress ip(192, 168, 1, 7);
char post;
char buf[30];
// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

// "HC-SR505 движение"
int pirPin = 19; // назначение пина для мини ИК датчика
int pirMove;     // переменная для хранения положения датчика

int settings[10];


byte PageData[11]; // byte array that will hold  data for a page
byte PageRead[11]; // array that will hold result of data for a page

int freeRam()
{
  extern int __heap_start, *__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

// EEprom BYTE read & write functions
//===================================

byte i2c_eeprom_read_byte(int deviceaddress, unsigned int eeaddress)
{
  byte rdata = 0xFF;
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // Address High Byte
  Wire.write((int)(eeaddress & 0xFF)); // Address Low Byte
  Wire.endTransmission();
  Wire.requestFrom(deviceaddress, 1);
  if (Wire.available())
    rdata = Wire.read();
  return rdata;
}

void i2c_eeprom_write_byte(int deviceaddress, unsigned int eeaddress, byte data)
{
  int rdata = data;
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // Address High Byte
  Wire.write((int)(eeaddress & 0xFF)); // Address Low Byte
  Wire.write(rdata);
  Wire.endTransmission();
}

// Address is a page address, 6-bit (63). More and end will wrap around
// But data can be maximum of 28 bytes, because the Wire library has a buffer of 32 bytes
void i2c_eeprom_write_page(int deviceaddress, unsigned int eeaddresspage, byte *data, byte length)
{
  //void eeprom_read_block (void * pointer_ram , const void * pointer_eeprom , size_t n)
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddresspage >> 8));   // Address High Byte
  Wire.write((int)(eeaddresspage & 0xFF)); // Address Low Byte
  byte c;
  for (c = 0; c < length; c++) //this is the little loop that puts each character in 'data' char into the page
    Wire.write(data[c]);
  Wire.endTransmission();
  delay(10); // need some delay
}
// should not read more than 28 bytes at a time!
void i2c_eeprom_read_buffer(int deviceaddress, unsigned int eeaddress, byte *buffer, int length)
{
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // Address High Byte
  Wire.write((int)(eeaddress & 0xFF)); // Address Low Byte
  Wire.endTransmission();
  Wire.requestFrom(deviceaddress, length);
  //int c = 0;
  for (int c = 0; c < length; c++)
    if (Wire.available())
      buffer[c] = Wire.read();
}





void setup()
{
  Wire.begin();
  TWBR = 12;

  sensors.begin();
  // Open serial communications and wait for port to open:
  Serial.begin(9600);

  i2c_eeprom_read_buffer(EEPROM_ADDR, 100, PageRead, 11);

  if (PageRead[clearA] != 42)
  {
    PageData[morningTimeA] = 6;
    PageData[eveningTimeA] = 21;
    PageData[humidityFanA] = 90;
    PageData[tempFanA] = 28;
    PageData[frostA] = 4;
    PageData[lowTempHeaterA] = 7;
    PageData[hightTempHeaterA] = 15;
    PageData[darkA] = 2;
    PageData[lightA] = 25;
    PageData[brightA] = 50;
    PageData[clearA] = 42;
    i2c_eeprom_write_page(EEPROM_ADDR, 100, PageData, 11);
    delay(30);
    Serial.println("eeprom standart value write ");
  }
   Serial.println("eeprom  value read ");
  for (int s = 0; s < 10; s++)
  {
    settings[s] = PageRead[s];
    Serial.print(PageRead[s]);
       Serial.print(";  ");
  }
   Serial.println("");


  pinMode(FAN, OUTPUT);
  pinMode(HEATER, OUTPUT);
  pinMode(LAMP, OUTPUT);
  digitalWrite(FAN, LOW);
  digitalWrite(HEATER, LOW);
  pinMode(pirPin, INPUT);       // пин датчика работает как вход
  pinMode(doorOpenPin, INPUT);  // пин датчика работает как вход
  pinMode(doorClosePin, INPUT); // пин датчика работает как вход
  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  //  call begin Function so turn the light sensor On .
  LightSensor.begin();
  LightSensor.setMode(Continuously_High_Resolution_Mode); // see datasheet page 5 for modes

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.home();
  lcd.setCursor(0, 0);
}



void loop() {

tmElements_t tm; // Время
currentMillis=millis();

 //----  дверь  ------
doorOpenTrigger=!digitalRead(doorOpenPin); 
if(doorUp && doorOpenTrigger) {doorUp=false; doorOpen=true;}

doorCloseTrigger=!digitalRead(doorClosePin); 
if(doorDown && doorCloseTrigger) {doorDown=false; doorOpen=false; }

if(doorUp) digitalWrite(OPENDOOR, HIGH); else digitalWrite(OPENDOOR, LOW);
if(doorDown) digitalWrite(CLOSEDOOR, HIGH); else digitalWrite(CLOSEDOOR, LOW);

if (!doorOpen && !heat && lux > settings[lightA] && !manualButton  && !doorLock && tempOut1 > settings[frostA]) doorUp=1;
if (doorOpen && lux <= settings[darkA] && !manualButton && !doorLock){
  if(currentMillis-darkMilis>60000){darkI++; darkMilis=currentMillis;}
  if (darkI>=20) {darkI=0; doorDown=1;}
}

  //-----вентилятор-------
  if(fan) digitalWrite(FAN, HIGH); else digitalWrite(FAN, LOW);
  if (!fan && humidityIn  >= settings[humidityFanA]  && humidityIn  >= humidityOut && !heat && tempOut1 > settings[frostA]  && !fanLock || tempIn1 >= settings[tempFanA] &&  !heat && tempOut1 > tempFanA && !fanLock) fan=1;
  if (fan && humidityIn < settings[humidityFanA] - 2 && tempIn1 < settings[tempFanA] - 1 && !fanLock || heat && !fanLock) fan=0;

  //------отопитель-------
  if(heat) digitalWrite(HEATER, HIGH); else digitalWrite(HEATER, LOW);
  if ( !heat && tempIn1 <= settings[lowTempHeaterA]  && !manualButton && !heatLock ) heat=1;
  if ( heat && tempIn1 > settings[hightTempHeaterA]  && !manualButton && !heatLock ) heat=0;

 //------свет-------
if( currentMillis- lampMilis > 20000){
  if(lampUp){
     if (lampIndex>=63){lampIndex=63; lampUp=0; lamp=255;}
     else {lamp=loga[lampIndex]; lampIndex++;} 
      analogWrite(LAMP, lamp);
     }
  if(lampDown){
    if(lampIndex<=0){lampIndex=0; lampDown=0; lamp=0;}
    else{ lamp=loga[lampIndex];  lampIndex--; }
     analogWrite(LAMP, lamp);
  }
 
  lampMilis=currentMillis;
}

   if (RTC.read(tm)) {   //Зажигаем и гасим свет по времени
       //Day
    if (tm.Hour >= settings[morningTimeA] && tm.Hour < settings[eveningTimeA] && !manualButton && !lampLock && lamp<255 && !lampUp)  lampUp = 1; //Serial.println( int(settings[morningTimeA])  );
    //Night
    if ( tm.Hour >=  settings[eveningTimeA] && !manualButton && !lampLock && lamp>0 && !lampDown)  lampDown = 1;// Serial.println(lampDown);
   }


//----  кнопки ------
if(digitalRead(lockButtonPin) && currentMillis-buttonMilis>buttonMilisInterval)  {  lockButton = 1;   buttonMilis=currentMillis; }

if(lockButton){
       
    if(digitalRead(doorButtonPin) && currentMillis-buttonMilis>buttonMilisInterval)  { 
      doorLock = !doorLock; 
      buttonMilis=currentMillis;  
      lockButton = 0;  }
     
    if(digitalRead(lampButtonPin) && currentMillis-buttonMilis>buttonMilisInterval)  { 
      lampLock = !lampLock; 
      buttonMilis=currentMillis;  
      lockButton = 0;  }
     
    if(digitalRead(heatButtonPin) && currentMillis-buttonMilis>buttonMilisInterval)  { 
      heatLock = !heatLock; 
      buttonMilis=currentMillis;  
      lockButton = 0;  }
    
    if(digitalRead(fanButtonPin) && currentMillis-buttonMilis>buttonMilisInterval)  { 
      fanLock = !fanLock; 
      buttonMilis=currentMillis;  
      lockButton = 0;  }
   
    if(digitalRead(lockButtonPin) && currentMillis-buttonMilis>buttonMilisInterval)  { lockButton = 0;   buttonMilis=currentMillis; }
    
    if(digitalRead(manualButtonPin) && currentMillis-buttonMilis>buttonMilisInterval)  { lockButton = 0;   buttonMilis=currentMillis; }
        if(digitalRead(doorButtonPin) && currentMillis-buttonMilis>buttonMilisInterval)  { 
      doorLock = !doorLock; 
      buttonMilis=currentMillis;  
      lockButton = 0;  }
     
    if(digitalRead(lampButtonPin) && currentMillis-buttonMilis>buttonMilisInterval)  { 
      lampLock = !lampLock; 
      buttonMilis=currentMillis;  
      lockButton = 0;  }
     
    if(digitalRead(heatButtonPin) && currentMillis-buttonMilis>buttonMilisInterval)  { 
      heatLock = !heatLock; 
      buttonMilis=currentMillis;  
      lockButton = 0;  }
    
    if(digitalRead(fanButtonPin) && currentMillis-buttonMilis>buttonMilisInterval)  { 
      fanLock = !fanLock; 
      buttonMilis=currentMillis;  
      lockButton = 0;  }
     
  
     }
    

if(digitalRead(manualButtonPin) && currentMillis-buttonMilis>buttonMilisInterval)  { manualButton = !manualButton; buttonMilis=currentMillis; }
   
 if(manualButton){

    if(digitalRead(doorButtonPin) && currentMillis-buttonMilis>buttonMilisInterval)  { 
      if(doorOpen) doorDown = 1;
      else doorUp = 1; 
      buttonMilis=currentMillis;  
         }
     
    if(digitalRead(lampButtonPin) && currentMillis-buttonMilis>buttonMilisInterval)  { 
      if (lampUp || lampIndex==255) lampUp = 0; lampDown = 1;
      if (lampDown || lampIndex==0) lampDown = 0; lampUp = 1;
      
      buttonMilis=currentMillis;  
         }
     
    if(digitalRead(heatButtonPin) && currentMillis-buttonMilis>buttonMilisInterval)  { 
      heat=!heat;
      buttonMilis=currentMillis;  
         }
    
    if(digitalRead(fanButtonPin) && currentMillis-buttonMilis>buttonMilisInterval)  { 
      fan=!fan;
      buttonMilis=currentMillis;  
         }

 }
 


  //------датчики и вывод на экран-------  
 if( currentMillis- sensorMilis > 5000){
 lux = LightSensor.getAmbientLight(); // Уровень света на улице 
 
sensors.requestTemperatures();//DS18B20 читаем температуры
  
tempBox = sensors.getTempC(ThermometerBox); //DS18B20 температура в боксе
//int tempIn2 = sensors.getTempC(ThermometerIn); //DS18B20 температура внутри
//int tempOut2 = sensors.getTempC(ThermometerOut); //DS18B20 температура снаружи
tempIn1 = dhtIn.readTemperature(); // Считывание температуры в цельсиях
tempOut1 = dhtOut.readTemperature(); // Считывание температуры в цельсиях
humidityIn = dhtIn.readHumidity(); // Считывание влажности
humidityOut = dhtOut.readHumidity(); // Считывание влажности
lcd.clear();
lcd.setCursor(0, 0); 
lcd.print("Ti ");lcd.print(tempIn1);
Serial.print("Ti "); Serial.print(tempIn1);
lcd.print(" To ");lcd.print(tempOut1);
Serial.print(" To "); Serial.print(tempOut1);
lcd.print(" lux ");lcd.print(lux);
Serial.print(" lux "); Serial.print(lux);
lcd.setCursor(0, 1);
lcd.print("Hi ");lcd.print(humidityIn);
Serial.print(" Hi "); Serial.print(humidityIn);
lcd.print(" Ho ");lcd.print(humidityOut);
Serial.print(" Ho "); Serial.print(humidityOut);
lcd.print(" lamp");
if (lampUp)lcd.print("'");
   else if (lampDown)lcd.print(".");
      else lcd.print(" ");
lcd.print(lamp);
Serial.print(" lamp "); Serial.print(lamp);                  
lcd.setCursor(0, 2);
lcd.print("heat ");lcd.print(int(heat));
Serial.print(" heat "); Serial.print(heat);
lcd.print(" fan ");lcd.print(int(fan));
Serial.print(" fan "); Serial.print(fan);
lcd.print(" door");
if (doorUp)lcd.print("'");
   else if (doorDown)lcd.print(".");
      else lcd.print(" ");
lcd.print(doorOpen); 
Serial.print(" doorOpen "); Serial.print(doorOpen);
lcd.setCursor(0, 3);
 if (RTC.read(tm)) {
   if(tm.Hour<10)lcd.print(0); lcd.print(tm.Hour); lcd.print(':');if(tm.Minute<10)lcd.print(0); lcd.print(tm.Minute); Serial.print("   "); Serial.print(tm.Hour); Serial.print(':'); Serial.println(tm.Minute);
   } else lcd.print('time err'); 
 if (lampLock)lcd.print(" lL");
 if (doorLock)lcd.print(" dL");
 if (fanLock)lcd.print(" fL");
 if (heatLock)lcd.print(" hL");
 if (manualButton)lcd.print(" M!");
 
 sensorMilis=currentMillis;
 
 }
 if(currentMillis- pirSensorMilis > pirDelay){
  pirMove = digitalRead (pirPin) ;// чтение значения с датчика
 
  if (pirMove == HIGH) {  lcd.backlight(); pirDelay=15000;  Serial.print("pirMove "); Serial.println(pirMove);}
  else { lcd.noBacklight(); pirDelay=500;}
 pirSensorMilis=currentMillis;
 }


 //------web-------
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {   
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    String buffer = String("");
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
 

   
 while(client.available()) {      //Обработка запроса POST(находится после пустой строки заголовка)
            post = client.read();  
            if (buffer.length() <= bufferMax) {
          buffer += post;
          
        } 
     }
Serial.println(buffer);
     
if(buffer.indexOf("morning") >= 0) {
 
 
  //Serial.println("settings_post_request");
  
  // int set3i; int set4i; int set5i; 
 // int set6i; int set7i; int set8i; int set9i; int set10i;

  set1i=buffer.indexOf("morning")+8; set2i=buffer.indexOf("&",set1i);
  settings[morningTimeA]=buffer.substring(set1i, set2i).toInt();
  
  set1i=buffer.indexOf("evening")+8; set2i=buffer.indexOf("&",set1i);
  settings[eveningTimeA]=buffer.substring(set1i, set2i).toInt();

  set1i=buffer.indexOf("humidity")+9; set2i=buffer.indexOf("&",set1i);
  settings[humidityFanA]=buffer.substring(set1i, set2i).toInt(); 

  set1i=buffer.indexOf("heat")+5; set2i=buffer.indexOf("&",set1i);
  settings[tempFanA]=buffer.substring(set1i, set2i).toInt(); 

  set1i=buffer.indexOf("frost")+6; set2i=buffer.indexOf("&",set1i);
  settings[frostA]=buffer.substring(set1i, set2i).toInt(); 

  set1i=buffer.indexOf("heateron")+9; set2i=buffer.indexOf("&",set1i);
  settings[lowTempHeaterA]=buffer.substring(set1i, set2i).toInt(); 

  set1i=buffer.indexOf("heateroff")+10;  set2i=buffer.indexOf("&",set1i);
  settings[hightTempHeaterA]=buffer.substring(set1i, set2i).toInt(); 

  set1i=buffer.indexOf("openlight")+10; set2i=buffer.indexOf("&",set1i);
  settings[lightA]=buffer.substring(set1i, set2i).toInt(); 

  set1i=buffer.indexOf("closelite")+10; set2i=buffer.indexOf("&",set1i);
  settings[darkA]=buffer.substring(set1i, set2i).toInt(); 

  set1i=buffer.indexOf("brightly")+9; set2i=buffer.indexOf("&",set1i);
  settings[brightA]=buffer.substring(set1i, set2i).toInt(); 


    for (int s = 0; s < 10; s++)
  {
     PageData[s] = settings[s] ;
    Serial.print(PageData[s]);
       Serial.print(";  ");
  }
   Serial.println("");

  i2c_eeprom_write_page(EEPROM_ADDR, 100, PageData, 11);

  //Serial.println( buffer.substring(set1i, set2i).toInt());
  }
if(buffer.indexOf("manual") >= 0) {
  
  //Serial.println("manage_post_request");
  if(buffer.indexOf("doorLock=on") >= 0) doorLock=1; else  doorLock=0;
  if(buffer.indexOf("heatLock=on") >= 0) heatLock=1; else  heatLock=0;
  if(buffer.indexOf("fanLock=on") >= 0) fanLock=1; else  fanLock=0;
  if(buffer.indexOf("lampLock=on") >= 0) lampLock=1; else  lampLock=0;
  
  set1i=buffer.indexOf("manual")+7; set2i=buffer.indexOf("&",set1i);
  if(buffer.substring(set1i, set2i).toInt()>0) manualButton=1; 
    else manualButton=0;

    

  set1i=buffer.indexOf("door")+5; set2i=buffer.indexOf("&",set1i);
  if(buffer.substring(set1i, set2i).toInt()>0) if(doorLock || manualButton) if(!doorOpen) doorUp = 1;
    else if(doorLock  || manualButton) if(doorOpen) doorDown = 1;

  set1i=buffer.indexOf("heat")+5; set2i=buffer.indexOf("&",set1i);
  if(buffer.substring(set1i, set2i).toInt()>0)  if(heatLock || manualButton) heat = 1;
     else if(heatLock  || manualButton) heat = 0;
  

  set1i=buffer.indexOf("fan")+4; set2i=buffer.indexOf("&",set1i); 
   if(buffer.substring(set1i, set2i).toInt()>0)  if(fanLock || manualButton) fan = 1;
    else if(fanLock  || manualButton) fan = 0;
     
  set1i=buffer.indexOf("lamp")+5; set2i=buffer.indexOf("&",set1i); 
  if(buffer.substring(set1i, set2i).toInt()>0)  if(lampLock || manualButton) lampUp = 1;
    else if(lampLock  || manualButton) lampDown = 1;
    }

          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.print("Connection: close\r\n\r\n");  // the connection will be closed after completion of the response
client.print("<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><style>table {margin:auto;border-collapse:collapse;width:100%;max-width:600px;}td,th{min-width:160px;text-align:left;padding:6px;}tr:nth-child(even){background-color:#dddddd;}</style><link rel='icon' href='data:;base64,='></head><body><table>");
client.print("<tr><td colspan='2'><span style='font-size: 36pt' >&#128020; </span>");
 if (RTC.read(tm)) {  client.print(tm.Hour); client.print(':'); client.print(tm.Minute); client.print(':'); client.print(tm.Second); client.print(",&nbsp;"); client.print(tm.Day); 
 client.print('/');  client.print(tm.Month);  client.print('/');   client.print(tmYearToCalendar(tm.Year)); }
   client.print(tdtr);

client.println("<form id='work' action='\' method='POST'>");
client.print("<tr><td>&#129302;&nbsp;Режим</td><td>");
client.print("<select name='manual'><option ");
if (!manualButton) client.print(" selected ");
 client.print(" value='1'>Ручной</option><option ");
if (manualButton) client.print(" selected ");
 client.print(" value='0'>Автоматический</option></select>");
 client.print(tdtr);


client.print("<tr><td>&#128682;&nbsp;Дверь</td><td>");
client.print("<select name='door'><option");
if (doorOpen)client.print(" selected ");
 client.print(" value='1'>Открыта </option><option");
if (!doorOpen)client.print(" selected ");
 client.print(" value='0'>Закрыта </option></select>");
  if(doorDown)client.print("&#11015;");
  if(doorUp)client.print("&#11014;");
client.print(" <span><input type='checkbox'");
if (doorLock)client.print(" checked ");
client.print(" name='doorLock'>&#128274;</span> ");
client.print(tdtr);


client.print("<tr><td>&#9832;&nbsp;Отопитель</td><td>");
client.print("<select name='heat'><option");
if (heat)client.print(" selected ");
 client.print(" value='1'>Включен </option><option");
if (!heat)client.print(" selected ");
 client.print(" value='0'>Выключен </option></select>");
client.print(" <span> <input type='checkbox'");
if (heatLock)client.print(" checked ");
client.print(" name='heatLock'>&#128274;</span> ");
client.print(tdtr);


client.print("<tr><td>&#10059;&nbsp;Вентилятор</td><td>");
client.print("<select name='fan'><option");
if (fan)client.print(" selected ");
 client.print(" value='1'>Включен </option><option");
if (!fan)client.print(" selected ");
 client.print(" value='0'>Выключен </option></select>");
client.print("  <span><input type='checkbox'");
if (fanLock)client.print(" checked ");
client.print(" name='fanLock'>&#128274;</span> ");
client.print(tdtr);


client.print("<tr><td>&#128161;&nbsp;Лампа (0-255)</td><td>");
client.print(lamp);
client.print("  &nbsp;<select name='lamp'><option");
if (lamp>0)client.print(" selected ");
 client.print(" value='1'>Включен </option><option");
if (lamp==0)client.print(" selected ");
 client.print(" value='0'>Выключен </option></select> ");
   if(lampDown)client.print("&#11015;");
  if(lampUp)client.print("&#11014;");
client.print("  <span><input type='checkbox'");
if (lampLock)client.print(" checked ");
client.print(" name='lampLock'>&#128274;</span> ");
client.print(tdtr);

 client.print("<tr><td colspan='2' style='text-align: center;'><input type='submit' value='Ввести' form='work'></form>"); client.print(tdtr);
 

client.print("<tr><td><h3>&#128065;&nbsp;Датчики</h3></td></tr>");
client.print("<tr><td>&#9728;&nbsp;Свет за окном</td><td>");
client.print(lux);
client.print(" lx");
client.print(tdtr);

//client.print("<tr><td>Температура внутри (ds18b20)</td><td>");
//client.print(tempIn2);
//client.print(tdtr);
client.print("<tr><td>&#127777;&nbsp;Температура внутри </td><td>");
client.print(tempIn1);
client.print("&#8451;");
client.print(tdtr);
//client.print("<tr><td>Температура снаружи (ds18b20)</td><td>");
//client.print(tempOut2);
//client.print(tdtr);
client.print("<tr><td>&#127777;&nbsp;Температура снаружи </td><td>");
client.print(tempOut1);
client.print("&#8451;");
client.print(tdtr);
client.print("<tr><td>&#127777;&nbsp;Температура в боксе </td><td>");
client.print(tempBox);
client.print("&#8451;");
client.print(tdtr);
client.print("<tr><td>&#128166;&nbsp;Влажность внутри </td><td>");
client.print(humidityIn);
client.print("%");
client.print(tdtr);
client.print("<tr><td>&#128166;&nbsp;Влажность снаружи </td><td>");
client.print(humidityOut);
client.print("%");
client.print(tdtr);
client.print("<tr></tr>");


client.print("<tr><td><h3>&#9881;&nbsp;Настройки</h3></td></tr>");
client.println("<form id='settings' action='\' method='POST'>");
client.print("<tr> <td><label for='morning'>&#128336; Час начала дня (досветка)"); client.print(endstringlable);
client.print(settings[morningTimeA]);
client.print("' name='morning' min='4' max='10' id='morning'>"); client.print(tdtr);
client.print("<tr> <td><label for='evening'>&#128336; Час конца дня (досветка)"); client.print(endstringlable);
client.print(settings[eveningTimeA]);
client.print("' name='evening' min='16' max='23' id='evening'>"); client.print(tdtr);
client.print("<tr> <td><label for='humidity'>&#128166;&nbsp;Порог влажности (вентилятор)"); client.print(endstringlable);
client.print(settings[humidityFanA]);
client.print("' name='humidity' min='50' max='100' id='humidity'>");client.print(tdtr);
client.print("<tr> <td><label for='heat'>&#127777;&nbsp;Высокая температура (вентилятор)"); client.print(endstringlable);
client.print(settings[tempFanA]);
client.print("' name='heat' min='20' max='40' id='heat'>"); client.print(tdtr);
client.print("<tr> <td><label for='frost'>&#127777;&nbsp;Низкая температура (не откроет дверь)"); client.print(endstringlable);
client.print(settings[frostA]);
client.print("' name='frost' min='0' max='18' id='frost'>"); client.print(tdtr);
client.print("<tr> <td><label for='heateron'>&#127777;&nbsp;Температура вкл. отопления"); client.print(endstringlable);
client.print(settings[lowTempHeaterA]);
client.print("' name='heateron' min='4' max='20' id='heateron'>"); client.print(tdtr);
client.print("<tr>  <td><label for='heateroff'>&#127777;&nbsp;Температура выкл. отопления"); client.print(endstringlable);
client.print(settings[hightTempHeaterA]);
client.print("' name='heateroff' min='6' max='25' id='heateroff'>"); client.print(tdtr);
client.print("<tr>  <td><label for='openlight'>&#9728;&nbsp;Светло - уровень света (откроет дверь)"); client.print(endstringlable);
client.print(settings[lightA]);
client.print("' name='openlight' min='10' max='100' id='openlight'>"); client.print(tdtr);
client.print("<tr> <td><label for='closelite'>&#9728;&nbsp;Темно - уровень света (закроет дверь)"); client.print(endstringlable);
client.print(settings[darkA]);
client.print("' name='closelite' min='0' max='10' id='closelite'>"); client.print(tdtr);
client.print("<tr> <td><label for='brightly'>&#9728;&nbsp;Ярко - уровень света (погасит досветку)"); client.print(endstringlable);
client.print(settings[brightA]);
client.print("' name='brightly' min='30' max='100' id='brightly'>"); client.print(tdtr);
client.print("<tr><td colspan='2' style='text-align: center;'><input type='submit' value='Ввести' form='settings'></form>"); client.print(tdtr);
client.print("</table></body></html>");

// Serial.println(buffer);  // Распечатка POST запроса
 break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println();
    Serial.println(" client disconnected"); 
     freeRam ();  
  }

}
