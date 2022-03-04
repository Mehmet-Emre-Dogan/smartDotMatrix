#include "irDefines.h"
// dot matrix
#include <MD_Parola.h>       
#include <MD_MAX72xx.h>

#include <virtuabotixRTC.h> // RTC module
#include <DHT.h> // DHT module

// IR remmote control
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

#include <ESP8266WiFi.h> //webserver
#include "secrets.h"

// ota
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>


#include "TR_Fonts_data.h" // about custom font 

// from parola-UTF-8 example
#define  DEBUG  0 // Turn on debug statements to the serial output
#if  DEBUG
#define PRINT(s, x) { Serial.print(F(s)); Serial.print(x); }
#define PRINTS(x) Serial.print(F(x))
#define PRINTX(s, x)  { Serial.print(F(s)); Serial.print(x, HEX); }
#else
#define PRINT(s, x)
#define PRINTS(x)
#define PRINTX(s, x)
#endif
;// Convert a single Character from UTF8 to Extended ASCII according to ISO 8859-1,
// also called ISO Latin-1. Codes 128-159 contain the Microsoft Windows Latin-1
// extended characters:
// - codes 0..127 are identical in ASCII and UTF-8
// - codes 160..191 in ISO-8859-1 and Windows-1252 are two-byte characters in UTF-8
//                 + 0xC2 then second byte identical to the extended ASCII code.
// - codes 192..255 in ISO-8859-1 and Windows-1252 are two-byte characters in UTF-8
//                 + 0xC3 then second byte differs only in the first two bits to extended ASCII code.
// - codes 128..159 in Windows-1252 are different, but usually only the €-symbol will be needed from this range.
//                 + The euro symbol is 0x80 in Windows-1252, 0xa4 in ISO-8859-15, and 0xe2 0x82 0xac in UTF-8.
//
// Modified from original code at http://playground.arduino.cc/Main/Utf8ascii
// Extended ASCII encoding should match the characters at http://www.ascii-code.com/
//
// Return "0" if a byte has to be ignored.
uint8_t utf8Ascii(uint8_t ascii){
  static uint8_t cPrev;
  uint8_t c = '\0';

  if (ascii < 0x7f)   // Standard ASCII-set 0..0x7F, no conversion
  {
    cPrev = '\0';
    c = ascii;
  }
  else
  {
    switch (cPrev)  // Conversion depending on preceding UTF8-character
    {
    case 0xC2: c = ascii;  break;
    case 0xC3: c = ascii | 0xC0;  break;
    case 0x82: if (ascii==0xAC) c = 0x80; // Euro symbol special case
    }
    cPrev = ascii;   // save last char
  }

  PRINTX("\nConverted 0x", ascii);
  PRINTX(" to 0x", c);

  return(c);
}

void utf8Ascii(char* s)
// In place conversion UTF-8 string to Extended ASCII
// The extended ASCII string is always shorter.
{
  uint8_t c;
  char *cp = s;

  PRINT("\nConverting: ", s);

  while (*s != '\0')
  {
    c = utf8Ascii(*s++);
    if (c != '\0')
      *cp++ = c;
  }
  *cp = '\0';   // terminate the new string
}

// about EEPROM
#include <SoftwareI2C.h>
#define DEVICE 0x50
#define SDA D1
#define SCL D2
#define addrMode 0
#define addrBottomH 1
#define addrBottomM 2
#define addrTopH 3
#define addrTopM 4
SoftwareI2C softwarei2c;

// about DHT 
#define DHTPIN 3             //DHT pin is defined //GPIO3 RX
#define DHTTYPE DHT22        //DHT model
DHT dht(DHTPIN, DHTTYPE);

// about RTC
#define CLK D0                                                    
#define DAT D3                                       
#define RST D5                                      
virtuabotixRTC myRTC(CLK, DAT, RST); 

// about matrix
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4       
#define CLK_PIN   D8        
#define DATA_PIN  D6
#define CS_PIN    D7
MD_Parola myMatrix = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// about ir remote controller
#include "irDefines.h"
#define RECV_PIN 1 //tx pin
IRrecv irrecv(RECV_PIN);
decode_results results;


// Variable to store the HTTP request
String header;
/*#####################################################################################################AUTO RESET TIME#######################################################################################################################################*/
const unsigned long ResetTime = 864000000; //10 GÜN (farklı istersen milisaniye olarak yaz)
uint8_t degC[] = { 6, 3, 3, 56, 68, 68, 68 }; //derece sembolü
String ttbp = "merhaba"; //text to be printed merhaba 
String ttbp2 = "*"; //text to be printed 2
String ttbp3 = "*"; //text to be printed 3
char curMessage2[4][8];
char curMessage3[250];
String gunler[7] = {"Pazartesi", "Salı", "Çarşamba", "Perşembe", "Cuma", "Cumartesi", "Pazar"};
//String gunler2[7] = {"monday", "tuesday", "wednesday", "thursday", "friday", "saturday", "sunday"};
char gunler2[][20] = {"pazartesi", "salì", "çaræamba", "peræembe", "cuma", "cumartesi", "pazar"};
bool isAutoNMEnabled = true; //otomatik gece modu etkin mi?
int gercek, alt, ust; //otomatik gece modu ile ilgili
bool isDayMode = true; // gece modunun zıttı olarak kullandım
byte bottomH = 23; // gece modu kaçta başlasın H saat, M min
byte bottomM = 45;
byte topH = 8; // gece modu kaçta bitsin H saat, M min
byte topM = 15;
byte mod = 7;
float t = 12.34; //sıcaklık
int h = 98; //nem
bool isUpdatable = true; //updateTN çalışabilir mi
const unsigned long updatePeriod = 10000; // sıcaklık ve nem ne kdar zamanda güncellensin
unsigned long previousUpdateTime = 0; //sıcaklık ve nem en son ne zaman güncellendi
unsigned long currentTime; //millis fonksiyonu kullanılırken o anki zamanı tutan değişken
bool canIreset = false;
const unsigned long tibtn = 5000; // transition interval between termo and nem
unsigned long pttbtn = 0; // previous transition time between termo and nem
const unsigned long clrInt2 = 3600000; // clear interval for ttbp2 //1 saat
unsigned long pct2 = 0; // previous clear time for ttbp2
const unsigned long clrInt3 = 90000000; // clear interval for ttbp3 //25 saat
unsigned long pct3 = 0; // previous clear time for ttbp3
int TorN = 0; // TERMO mu NEM mi
//unsigned long pst = 0; // previous scroll time for saniye
byte shown = 0; //varsayılan modda gösterilecek item
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////EFEKTLER////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint16_t hiz = 25;
uint16_t hiz2 = 50;
int16_t hiz3 = 5;
uint16_t zaman = 2500;
uint16_t zaman2 = 1500;
uint16_t zaman3 = 3500;
textEffect_t efektSaat = PA_RANDOM;
textEffect_t efektTarih = PA_OPENING_CURSOR;
textEffect_t efektYil = PA_CLOSING_CURSOR;
textEffect_t efektTemp = PA_SCROLL_DOWN;
textEffect_t efektNem = PA_SCROLL_UP; 
textEffect_t efektGun = PA_SCROLL_RIGHT;
textEffect_t efektCustom = PA_SCROLL_LEFT;
char charArr2[20];
String str2;

//sıcaklık ve nem güncellemesi yapılıp yapılamayacağına (i.e. son güncellemeden sonra yeterli zaman geçmiş mi?) karar verir
void changeIsUpdatable(){
  currentTime = millis();
  if( currentTime - previousUpdateTime >= updatePeriod ){
    isUpdatable = true;
    previousUpdateTime = currentTime;
  }
}

//otomatik gece modu
void autoNM(){
 isDayMode = true;
 myRTC.updateTime();//Zaman güncellemesi
 gercek = (myRTC.hours)*60 + myRTC.minutes;
 alt = bottomH*60 + bottomM;
 ust = topH*60 + topM;
 if( (alt<ust) && ( (gercek<=ust) && (gercek>alt) ) ){
  isDayMode = false;
 }
 
  if( (alt>ust) && ( (gercek<ust) || (gercek>=alt) ) ){
  isDayMode = false;
 }
 
}

//sıcaklık ve nem değerini günceller ve rame atar
void updateTN(){
t = dht.readTemperature(); 
h = dht.readHumidity();
isUpdatable = false;
}

int getUptime(){
 return (int)( millis() /86400000 ); //1 gün 86400000 milisaniye
}

void(* resetFunc) (void) = 0; //bunu çağırınca reset atıyor

String saatHtml(){
   myRTC.updateTime();//Zaman güncellemesi

  if(myRTC.minutes<10){
    if (myRTC.seconds<10)
      return String(myRTC.hours) + ":" + "0" + String(myRTC.minutes) + ":" + "0" + String(myRTC.seconds) + " " + String(myRTC.dayofmonth) + "/"  + String(myRTC.month) + "/" + String(myRTC.year);  //Dakika değeri 0-9 aralığındaysa formatı bozmamak için başına 0 ekleyerek yazdırıyoruz
    else
      return String(myRTC.hours) + ":" + "0" + String(myRTC.minutes) + ":"  + String(myRTC.seconds) + " " + String(myRTC.dayofmonth) + "/"  + String(myRTC.month) + "/" + String(myRTC.year);
  }
  else{
    if (myRTC.seconds<10)
      return String(myRTC.hours) + ":" + String(myRTC.minutes) + ":" + "0" + String(myRTC.seconds) + " " + String(myRTC.dayofmonth) + "/"  + String(myRTC.month) + "/" + String(myRTC.year);  //Dakika değeri 0-9 aralığındaysa formatı bozmamak için başına 0 ekleyerek yazdırıyoruz
    else
      return String(myRTC.hours) + ":" + String(myRTC.minutes) + ":"  + String(myRTC.seconds) + " " + String(myRTC.dayofmonth) + "/"  + String(myRTC.month) + "/" + String(myRTC.year);

  }
}

void saat(){
myMatrix.setTextAlignment(PA_CENTER);
   myRTC.updateTime();//Zaman güncellemesi
   if(myRTC.seconds%2){
if(myRTC.hours<10 && myRTC.hours>=0){
 if(myRTC.minutes<10 && myRTC.minutes>=0)
   myMatrix.print("0" + String(myRTC.hours) + ":" + "0" + String(myRTC.minutes));  //Dakika değeri 0-9 aralığındaysa formatı bozmamak için başına 0 ekleyerek yazdırıyoruz
   else
   myMatrix.print("0" + String(myRTC.hours) + ":" + String(myRTC.minutes)); //Saat yazdırma
}
else{
  if(myRTC.minutes<10 && myRTC.minutes>=0)
   myMatrix.print(String(myRTC.hours) + ":" + "0" + String(myRTC.minutes));  //Dakika değeri 0-9 aralığındaysa formatı bozmamak için başına 0 ekleyerek yazdırıyoruz
   else
   myMatrix.print(String(myRTC.hours) + ":" + String(myRTC.minutes)); //Saat yazdırma
}
   }
else{
  
if(myRTC.hours<10 && myRTC.hours>=0){
 if(myRTC.minutes<10 && myRTC.minutes>=0)
   myMatrix.print("0" + String(myRTC.hours) + " " + "0" + String(myRTC.minutes));  //Dakika değeri 0-9 aralığındaysa formatı bozmamak için başına 0 ekleyerek yazdırıyoruz
   else
   myMatrix.print("0" + String(myRTC.hours) + " " + String(myRTC.minutes)); //Saat yazdırma
}
else{
  if(myRTC.minutes<10 && myRTC.minutes>=0)
   myMatrix.print(String(myRTC.hours) + " " + "0" + String(myRTC.minutes));  //Dakika değeri 0-9 aralığındaysa formatı bozmamak için başına 0 ekleyerek yazdırıyoruz
   else
   myMatrix.print(String(myRTC.hours) + " " + String(myRTC.minutes)); //Saat yazdırma
}
}
}

void saniye(){
myRTC.updateTime();//Zaman güncellemesi
myMatrix.setTextAlignment(PA_RIGHT);
if(myRTC.seconds<10)
myMatrix.print("0" + String(myRTC.seconds));
else
myMatrix.print(String(myRTC.seconds));
}

//ramdeki en güncel sıcaklık bilgisini ekrana yazar (zaten güncel olmayanlar otomatik siliniyor)
void termo(){
myMatrix.setTextAlignment(PA_LEFT);
char tArray[4]; //sıcaklığın char hali
dtostrf(t,3,1,tArray);
myMatrix.print(String(tArray) + "$");
}

//ramdeki en güncel nem bilgisini ekrana yazar (zaten güncel olmayanlar otomatik siliniyor)
void nem(){
myMatrix.setTextAlignment(PA_LEFT);
myMatrix.print("N: %" + String(h));
}

void termoNem(){
    if( millis() - pttbtn >= tibtn ){
    TorN++;
    TorN %= 2;
    pttbtn = millis();
  }

  switch(TorN){
    case 0:
    termo();
    break;

    case 1:
    nem();
    break;

    default:
    myMatrix.setTextAlignment(PA_CENTER);
    myMatrix.print("e: ATN");
  }
}

void printText(String str){
  myMatrix.setTextAlignment(PA_CENTER);
  char charArr[100] = {""};
  //str = beautifyString(str);
  str.toCharArray(charArr, 100);
  utf8Ascii(charArr);
  if (str.length() <= 5){
  myMatrix.print(charArr);
  }
  else{
    if(myMatrix.displayAnimate()){
    myMatrix.displayText(charArr, PA_LEFT, 15, 2000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
    }
  } 
}

String beautifyString(String inputStr){
  inputStr += "$$$"; //add some dummy chars to prevent out of index error
  String str = ""; //the string holds beautified charset
  for( int i = 0; inputStr[i] != '$'; i++){
    if(inputStr[i] == '_' && inputStr[i+1] == '!'){
      switch(inputStr[i+2]){
        case 'B':
        str += ">";
        i += 2;
        break;

        case 'K':
        str += "<";
        i += 2;
        break;

        case 'G':
        str += "À";
        i += 2;
        break;

        case 'S':
        str += "Æ";
        i += 2;
        break;

        case 'C':
        str += "Ç";
        i += 2;
        break;

        case 'I':
        str += "Ì";
        i += 2;
        break;
        
        case 'O':
        str += "Ö";
        i += 2;
        break;

        case 'U':
        str += "Ü";
        i += 2;
        break;    

        case 'g':
        str += "à";
        i += 2;
        break;
        
        case 's':
        str += "æ";
        i += 2;
        break;
        
        case 'c':
        str += "ç";
        i += 2;
        break; 
        
        case 'i':
        str += "ì";
        i += 2;
        break; 

        case 'o':
        str += "ö";
        i += 2;
        break;

        case 'u':
        str += "ü";
        i += 2;
        break;

        default:
        ;
      }
    }
    else{
      str += inputStr[i];
    }
  }
  return str;
}


void multi(){
   myRTC.updateTime();//Zaman güncellemesi
   if (myMatrix.displayAnimate()){  
     switch (shown){
     case 0:
        if(myRTC.hours<10 && myRTC.hours>=0){
          if(myRTC.minutes<10 && myRTC.minutes>=0)
            str2 = "0" + String(myRTC.hours) + ":" + "0" + String(myRTC.minutes);  //Dakika değeri 0-9 aralığındaysa formatı bozmamak için başına 0 ekleyerek yazdırıyoruz
          else
          str2 = "0" + String(myRTC.hours) + ":" + String(myRTC.minutes); //Saat yazdırma
          }
        else{
         if(myRTC.minutes<10 && myRTC.minutes>=0)
           str2 = String(myRTC.hours) + ":" + "0" + String(myRTC.minutes); //Dakika değeri 0-9 aralığındaysa formatı bozmamak için başına 0 ekleyerek yazdırıyoruz
         else
         str2 = String(myRTC.hours) + ":" + String(myRTC.minutes); //Saat yazdırma
        }
        str2.toCharArray(charArr2, 20);
        
          myMatrix.displayText(charArr2, PA_CENTER, hiz3, zaman, efektSaat, efektSaat); 
    break;

    case 1:
      if (myRTC.dayofmonth < 10){
        if(myRTC.month < 10)
          str2 = "0" + String(myRTC.dayofmonth) + "/0"  + String(myRTC.month);
        else
          str2 ="0" + String(myRTC.dayofmonth) + "/"  + String(myRTC.month);
      }
      else{
        if(myRTC.month < 10)
          str2 = String(myRTC.dayofmonth) + "/0"  + String(myRTC.month);
        else
          str2 = String(myRTC.dayofmonth) + "/"  + String(myRTC.month);
      }
     str2.toCharArray(charArr2, 20);
      myMatrix.displayText(charArr2, PA_LEFT, hiz, zaman, efektTarih, PA_NO_EFFECT);
    break;
    
    case 2:
      String(myRTC.year).toCharArray(charArr2, 20);
      myMatrix.displayText(charArr2, PA_LEFT, hiz, zaman2, efektYil, PA_NO_EFFECT);
    break;

    case 3:
    char tArray[4]; //sıcaklığın char hali
    dtostrf(t,3,1,tArray);
    (String(tArray) + "$").toCharArray(charArr2, 20);
     myMatrix.displayText(charArr2, PA_LEFT, hiz2, zaman3, efektTemp, efektNem);
    break;

    case 4:
    //str2 = gunler2[myRTC.dayofweek-1];
    //str2.toCharArray(charArr2, 20);
    myMatrix.displayScroll(gunler2[myRTC.dayofweek-1], PA_LEFT, PA_SCROLL_LEFT, hiz);
    //myMatrix.displayText(charArr2, PA_LEFT, hiz, zaman, efektGun, PA_NO_EFFECT);
    break;

    case 5:
    myMatrix.displayScroll(gunler2[myRTC.dayofweek-1], PA_LEFT, PA_SCROLL_RIGHT, hiz);
    //myMatrix.displayText(charArr2, PA_LEFT, hiz, zaman, PA_PRINT, efektCustom);
    break;

    case 6:
    str2 = "N: %" + String(h);
    str2.toCharArray(charArr2, 20);
    myMatrix.displayText(charArr2, PA_LEFT, hiz2, zaman, efektNem, efektTemp);
    break;


       default:
        if (!ttbp2.equals("*")){
          switch(shown){
          case 7:
          if (myMatrix.displayAnimate())
            myMatrix.displayText(curMessage2[0], PA_CENTER, hiz, zaman3, PA_SCROLL_LEFT, PA_SCROLL_RIGHT);
          break;
  
          case 8:
          if (myMatrix.displayAnimate())
            myMatrix.displayText(curMessage2[1], PA_CENTER, hiz, zaman3, PA_SCROLL_RIGHT, PA_SCROLL_LEFT);
          break;
  
          case 9:
          if (myMatrix.displayAnimate())
            myMatrix.displayText(curMessage2[2], PA_RIGHT, hiz, zaman3, PA_SCROLL_LEFT, PA_SCROLL_RIGHT);
          break;

          case 10:
          if (myMatrix.displayAnimate())
            myMatrix.displayText(curMessage2[3], PA_RIGHT, hiz, zaman3, PA_BLINDS, PA_BLINDS);
          break;
  
          default:
          ;
        }
      }

      if(!ttbp3.equals("*")){
        switch(shown){
        case 11:
        ttbp3.toCharArray(curMessage3, 250);
        utf8Ascii(curMessage3); 
        myMatrix.displayScroll(curMessage3, PA_LEFT, PA_SCROLL_LEFT, hiz);
        break;

        default:
        ;
      }
      }

     }
     shown++;
     shown %= 12;
  }
}

void clrTtbps(){
if( millis() - pct2 >= clrInt2 ){
    ttbp2 = "*";
    pct2 = millis();
  }

if( millis() - pct3 >= clrInt3 ){
    ttbp3 = "*";
    pct3 = millis();
  }
}

void parseTtbp2(){
  String str0, str1, str2, str3;
  //bool flag0 = false, flag1 = false, flag2 = false; //hepsi yerine sadece bazılarının gelmesi durmunu ele alacak olursam lazım olacak if flagN --> strtochararray[n]
  String box =  ttbp2 + '!';
  for(int i = 0; box[i] != '!'; i++){
    if(box[i] == '_'){
      switch (box[i+1]){
        case 'a':
        for(int j = i+2; box[j] != '?'; j++){
          str0 += box[j];      
        }
        break;

        case 'd':
        for(int j = i+2; box[j] != '?'; j++){
          str1 += box[j];      
        }
        break;

        case 'e':
        for(int j = i+2; box[j] != '?'; j++){
          str2 += box[j];      
        }
        break;
        
        case 'h':
        for(int j = i+2; box[j] != '?'; j++){
          str3 += box[j];      
        }
        str3 += '$';
        break;

        default:
        ;  
      }
    }
    
  }
  str0.toCharArray(curMessage2[0], 8);
  str1.toCharArray(curMessage2[1], 8);
  str2.toCharArray(curMessage2[2], 8);
  str3.toCharArray(curMessage2[3], 8);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////SETUP//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
pinMode(LED_BUILTIN, OUTPUT);
digitalWrite(LED_BUILTIN, HIGH);
pinMode(1, FUNCTION_3); //GPIO 1 (TX) swap the pin to a GPIO.
pinMode(3, FUNCTION_3);  //GPIO 3 (RX) swap the pin to a GPIO.
myMatrix.begin();
myMatrix.setFont(TR);
myMatrix.print("start");

dht.begin();
softwarei2c.begin(SDA, SCL);
/*
pinMode(1, FUNCTION_0);//GPIO 1 (TX) restore
pinMode(3, FUNCTION_0); //GPIO 3 (RX) restore.*/

//Serial.begin(9600);
  // Connect to Wi-Fi network with SSID and password
  WiFi.disconnect(); 
  WiFi.begin(ssid, password);
  WiFi.hostname(deviceName);
  WiFi.mode(WIFI_STA); //WiFi mode station (connect to wifi router only
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(400);
    digitalWrite(LED_BUILTIN, HIGH);                // orijinal kodda 500 delay var
    delay(100);                            
  }
  WiFi.config(staticIP, subnet, gateway, dns);
  server.begin();

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////OTA//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    //Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    //Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    //Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    //Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      //Serial.println("Auth Failed");
      ;
    } else if (error == OTA_BEGIN_ERROR) {
      //Serial.println("Begin Failed");
      ;
    } else if (error == OTA_CONNECT_ERROR) {
      //Serial.println("Connect Failed");
      ;
    } else if (error == OTA_RECEIVE_ERROR) {
      //Serial.println("Receive Failed");
      ;
    } else if (error == OTA_END_ERROR) {
      //Serial.println("End Failed");
      ;
    }
  });
  ArduinoOTA.begin();

  //initialise the important saved variables
  mod = readEEPROM(addrMode);
  myMatrix.print(mod);
  delay(500);
  bottomH = readEEPROM(addrBottomH);
  bottomM = readEEPROM(addrBottomM);
  topH = readEEPROM(addrTopH);
  topM = readEEPROM(addrTopM);

  analogWrite(LED_BUILTIN, 900); 
  irrecv.enableIRIn(); //init kumanda
  myMatrix.addChar('$', degC); //derece sembolünü ekle ve "dolar işareti ($)" ile onu kullan.
  myMatrix.setIntensity(15);
  myMatrix.setTextAlignment(PA_CENTER);
  myMatrix.print("ready");
   for (uint8_t i=0; i<ARRAY_SIZE(gunler2); i++)
      utf8Ascii(gunler2[i]);
  delay(1500);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////LOOP//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {

  //web arayüzünden gelen bilgileri sisteme aktar
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    //Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        //Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // assign selected mode to variable named mod
            if (header.indexOf("GET /mode/1") >= 0)
            {
              mod = 1;
            }
            else if (header.indexOf("GET /mode/2") >= 0)
            {
              mod = 2;
            }
            else if (header.indexOf("GET /mode/3") >= 0) {
              mod = 3;
            }
            else if (header.indexOf("GET /mode/4") >= 0) {             
               mod = 4;
            }
            else if (header.indexOf("GET /mode/5") >= 0)
            { 
              mod = 5;
            }
            else if (header.indexOf("GET /mode/6") >= 0)
            {
              mod = 6;
            }
            else if (header.indexOf("GET /mode/7") >= 0) {
               mod = 7;
            }
            
            //oto açılma-kapanma ayarlarını set et
            else if (header.indexOf("GET /a/h/?") >= 0) {
                  int index = header.indexOf('?') + 1;
                  String mystr = "";
                  for(; header[index] != '?'; index++){
                    mystr += header[index];
                  }
              int sayi = mystr.toInt();
              if (sayi >= 0 && sayi < 24){
              bottomH = sayi; 
              //Serial.println(sayi); 
              }
              else
              //Serial.println("saat degeri 0 ile 23 arasında olmalı!"); 
              ;
            }
            else if (header.indexOf("GET /a/m/?") >= 0) {
                  int index = header.indexOf('?') + 1;
                  String mystr = "";
                  for(; header[index] != '?'; index++){
                    mystr += header[index];
                  }
              int sayi = mystr.toInt();
              if (sayi >= 0 && sayi < 60){
              bottomM = sayi; 
              //Serial.println(sayi); 
              }
              else
              //Serial.println("dakika degeri 0 ile 59 arasında olmalı!"); 
              ;
            }
            else if (header.indexOf("GET /u/h/?") >= 0) {
                  int index = header.indexOf('?') + 1;
                  String mystr = "";
                  for(; header[index] != '?'; index++){
                    mystr += header[index];
                  }
              int sayi = mystr.toInt();
              if (sayi >= 0 && sayi < 24){
              topH = sayi; 
              //Serial.println(sayi); 
              }
              else
              //Serial.println("saat degeri 0 ile 23 arasında olmalı!"); 
              ;
            }
            else if (header.indexOf("GET /u/m/?") >= 0) {
                  int index = header.indexOf('?') + 1;
                  String mystr = "";
                  for(; header[index] != '?'; index++){
                    mystr += header[index];
                  }
              int sayi = mystr.toInt();
              if (sayi >= 0 && sayi < 60){
              topM = sayi; 
              //Serial.println(sayi); 
              }
              else
              //Serial.println("dakika degeri 0 ile 59 arasında olmalı!"); 
              ;
            }
            
            //otomatik gece modu ayarı açık-kapalı set et
            else if (header.indexOf("GET /anm/on") >= 0)
            {
              isAutoNMEnabled = true;
            }
            else if (header.indexOf("GET /anm/off") >= 0)
            {
              isAutoNMEnabled = false;
            }

            //gece modunu manuel set et
            else if (header.indexOf("GET /nm/off") >= 0)
            {
              isDayMode = true;
            }
            else if (header.indexOf("GET /nm/on") >= 0)
            {
              isDayMode = false;
            }
            
            //set text
            else if (header.indexOf("GET /setText/$") >= 0) {
                  int index = header.indexOf('$') + 1;
                  String mystr = "";
                  for(; header[index] != '$'; index++){
                    // ardı ardına iki tane "_" ("__") gelirse yerine " " (space) yaz
                    if(header[index] == '_' && header[index+1] =='_'){ 
                         mystr += ' ';
                         index++;                      
                    }
                    
                    else{
                      mystr += header[index];
                    }
                  }
              ttbp = beautifyString(mystr);
            }

            //set text2
            else if (header.indexOf("GET /setText2/$") >= 0) {
                  int index = header.indexOf('$') + 1;
                  String mystr = "";
                  for(; header[index] != '$'; index++){
                    // ardı ardına iki tane "_" ("__") gelirse yerine " " (space) yaz
                    if(header[index] == '_' && header[index+1] =='_'){ 
                         mystr += ' ';
                         index++;                      
                    }
                    
                    else{
                      mystr += header[index];
                    }
                  }
              ttbp2 = mystr;
              pct2 = millis(); // yeni bilgi gelirse silme sayacını tekrar başlat
            }

            //set text3
            else if (header.indexOf("GET /setText3/$") >= 0) {
                  int index = header.indexOf('$') + 1;
                  String mystr = "";
                  for(; header[index] != '$'; index++){
                    // ardı ardına iki tane "_" ("__") gelirse yerine " " (space) yaz
                    if(header[index] == '_' && header[index+1] =='_'){ 
                         mystr += ' ';
                         index++;                      
                    }
                    
                    else{
                      mystr += header[index];
                    }
                  }
              ttbp3 = beautifyString(mystr);
              pct3 = millis(); // yeni bilgi gelirse silme sayacını tekrar başlat
            }
            
            //set rtc hour
            else if (header.indexOf("GET /setRtc/hour/?") >= 0) {
                  int index = header.indexOf('?') + 1;
                  String mystr = "";
                  for(; header[index] != '?'; index++){
                    mystr += header[index];
                  }
              int sayi = mystr.toInt();
              if (sayi >= 0 && sayi < 23){
              myRTC.updateTime();
              myRTC.setDS1302Time(myRTC.seconds,  myRTC.minutes, sayi, myRTC.dayofweek, myRTC.dayofmonth, myRTC.month, myRTC.year); //saniye, dakika, saat, haftanın günü, gün, ay, yıl  
              //Serial.println(sayi); 
              }
              else
              //Serial.println("saat degeri 0 ile 23 arasında olmalı!");
              ; 
            }
            
            //set rtc minute
            else if (header.indexOf("GET /setRtc/min/?") >= 0) {
                  int index = header.indexOf('?') + 1;
                  String mystr = "";
                  for(; header[index] != '?'; index++){
                    mystr += header[index];
                  }
              int sayi = mystr.toInt();
              if (sayi >= 0 && sayi < 60){
              myRTC.updateTime();
              myRTC.setDS1302Time(myRTC.seconds, sayi, myRTC.hours, myRTC.dayofweek, myRTC.dayofmonth, myRTC.month, myRTC.year); //saniye, dakika, saat, haftanın günü, gün, ay, yıl  
              //Serial.println(sayi); 
              }
              else
              //Serial.println("dakika degeri 0 ile 59 arasında olmalı!"); 
              ;
            }

            //set rtc seconds
            else if (header.indexOf("GET /setRtc/sec/?") >= 0) {
                  int index = header.indexOf('?') + 1;
                  String mystr = "";
                  for(; header[index] != '?'; index++){
                    mystr += header[index];
                  }
              int sayi = mystr.toInt();
              if (sayi >= 0 && sayi < 60){
              myRTC.updateTime();
              myRTC.setDS1302Time(sayi, myRTC.minutes, myRTC.hours, myRTC.dayofweek, myRTC.dayofmonth, myRTC.month, myRTC.year); //saniye, dakika, saat, haftanın günü, gün, ay, yıl  
              //Serial.println(sayi); 
              }
              else
              //Serial.println("saniye degeri 0 ile 59 arasında olmalı!"); 
              ;
            }


             //set rtc day of week
            else if (header.indexOf("GET /setRtc/dow/?") >= 0) {
                  int index = header.indexOf('?') + 1;
                  String mystr = "";
                  for(; header[index] != '?'; index++){
                    mystr += header[index];
                  }
              int sayi = mystr.toInt();
              if (sayi >= 1 && sayi < 8){
              myRTC.updateTime();
              myRTC.setDS1302Time(myRTC.seconds,  myRTC.minutes, myRTC.hours, sayi, myRTC.dayofmonth, myRTC.month, myRTC.year); //saniye, dakika, saat, haftanın günü, gün, ay, yıl  
              //Serial.println(sayi); 
              }
              else
              //Serial.println("haftanın günü degeri 1 ile 7 arasında olmalı!"); 
              ;
            }

            //set rtc day
            else if (header.indexOf("GET /setRtc/day/?") >= 0) {
                  int index = header.indexOf('?') + 1;
                  String mystr = "";
                  for(; header[index] != '?'; index++){
                    mystr += header[index];
                  }
              int sayi = mystr.toInt();
              if (sayi >= 1 && sayi < 32){
              myRTC.updateTime();
              myRTC.setDS1302Time(myRTC.seconds,  myRTC.minutes, myRTC.hours, myRTC.dayofweek, sayi, myRTC.month, myRTC.year); //saniye, dakika, saat, haftanın günü, gün, ay, yıl  
              //Serial.println(sayi); 
              }
              else
              //Serial.println("gün degeri 1 ile 31 arasında olmalı!"); 
              ;
            }

            //set rtc month
            else if (header.indexOf("GET /setRtc/mon/?") >= 0) {
                  int index = header.indexOf('?') + 1;
                  String mystr = "";
                  for(; header[index] != '?'; index++){
                    mystr += header[index];
                  }
              int sayi = mystr.toInt();
              if (sayi >= 1 && sayi < 13){
              myRTC.updateTime();
              myRTC.setDS1302Time(myRTC.seconds,  myRTC.minutes, myRTC.hours, myRTC.dayofweek, myRTC.dayofmonth, sayi, myRTC.year); //saniye, dakika, saat, haftanın günü, gün, ay, yıl  
              //Serial.println(sayi); 
              }
              else
              //Serial.println("ay degeri 1 ile 12 arasında olmalı!");
              ;
            }

            //set rtc year
            else if (header.indexOf("GET /setRtc/yr/?") >= 0) {
                  int index = header.indexOf('?') + 1;
                  String mystr = "";
                  for(; header[index] != '?'; index++){
                    mystr += header[index];
                  }
              int sayi = mystr.toInt();
              if (sayi >= 2000 && sayi < 2100){
              myRTC.updateTime();
              myRTC.setDS1302Time(myRTC.seconds,  myRTC.minutes, myRTC.hours, myRTC.dayofweek, myRTC.dayofmonth,  myRTC.month, sayi); //saniye, dakika, saat, haftanın günü, gün, ay, yıl  
              //Serial.println(sayi); 
              }
              else
              //Serial.println("yıl degeri 2000 ile 2099 arasında olmalı!"); 
              ;
            }

            //reset if reset button clicked
            else if (header.indexOf("GET /reboot") >= 0)
            {
              canIreset = true;
            }
            
            

            
            // Display the HTML web page
client.println("<!DOCTYPE html><html>");
client.println("<head><meta charset='utf-8' name='viewport' content='width=device-width, initial-scale=1'>");
client.println("<style>html, body { font-family: Helvetica; display: block; margin: 0px; text-align: left; padding-left: 15px; background-color: rgb(37, 37, 38);}");
client.println(".button3 { background-color: #14b2b8; border: none; color: white;  width: 200px; padding: 12px 24px;  ");
client.println("text-decoration: none; font-size: 20px;  cursor: pointer;}");
client.println();
client.println(".mytext3 {font-size: 18px; font-family: 'Consolas';  text-align: right; color: rgb(253, 100, 100); }");
client.println(".mytext2 {font-size: 16px; font-family: 'Consolas';  text-align: left; color: white; }");
client.println("a { text-decoration: none; color:white }");
client.println(".div1 {background-color: rgb(20, 26, 9); position:absolute; left: 370px; width: 200px; height: auto ; padding: 10px; border-style:dotted; border-color: white;}");
client.println(".div2 {background-color: black; width: 293px; padding: 10px;  border-style:solid; border-color: white;}");
client.println(".div3 {background-color: black; width: 293px; padding: 10px;  border-style:solid; border-color: white; display: block;  text-align: center;}");
client.println();
client.println(".button { background-color: #209e48; border: none; color: white; padding: 12px 24px;");
client.println("text-decoration: none; font-size: 20px; margin: 2px; cursor: pointer;}");
client.println(".button2 {background-color: #c20a0a; padding: 12px 28px;}");
client.println(".textbox {width: 150px; border: 1px solid #333; padding: 16px 20px; background-image: linear-gradient(180deg, #fff, #ddd 40%, #ccc);}");
client.println(".mytext {font-size: 16px; font-weight:bold; font-family:Arial ; text-align: center; color: black;}");
client.println("#container3 {width: 100%; height: 100%; margin-top: 10px; margin-bottom: 10px; margin-left: 0px;  padding: 0px; display: -webkit-flex; -webkit-justify-content: center; display: flex; justify-content: center;} ");
client.println();
client.println(".container2 {");
client.println("  display: block;");
client.println("  position: relative;");
client.println("  padding-left: 0;");
client.println("  margin-bottom: 12px;");
client.println("  cursor: pointer;");
client.println("  font-size: 22px;");
client.println("  -webkit-user-select: none;");
client.println("  -moz-user-select: none;");
client.println("  -ms-user-select: none;");
client.println("  user-select: none;");
client.println("}");
client.println();
client.println(".container {");
client.println("  display: block;");
client.println("  position: relative;");
client.println("  padding-left: 35px;");
client.println("  margin-bottom: 12px;");
client.println("  cursor: pointer;");
client.println("  font-size: 22px;");
client.println("  -webkit-user-select: none;");
client.println("  -moz-user-select: none;");
client.println("  -ms-user-select: none;");
client.println("  user-select: none;");
client.println("}");
client.println();
client.println("/* Hide the browser's default radio button */");
client.println(".container input {");
client.println("  position: absolute;");
client.println("  opacity: 50;");
client.println("  cursor: pointer;");
client.println("  transform: scale(0);");
client.println("}");
client.println();
client.println("/* Create a custom radio button */");
client.println(".checkmark {");
client.println("  position: absolute;");
client.println("  top: 0;");
client.println("  left: 0;");
client.println("  height: 25px;");
client.println("  width: 25px;");
client.println("  background-color: #eee;");
client.println("  border-radius: 50%;");
client.println("}");
client.println();
client.println("/* On mouse-over, add a grey background color */");
client.println(".container:hover input ~ .checkmark {");
client.println("  background-color: #ccc;");
client.println("}");
client.println();
client.println("/* When the radio button is checked, add a blue background */");
client.println(".container input:checked ~ .checkmark {");
client.println("  background-color: #037428;");
client.println("}");
client.println();
client.println("/* Create the indicator (the dot/circle - hidden when not checked) */");
client.println(".checkmark:after {");
client.println("  content: '';");
client.println("  position: absolute;");
client.println("  display: none;");
client.println("}");
client.println();
client.println("/* Show the indicator (dot/circle) when checked */");
client.println(".container input:checked ~ .checkmark:after {");
client.println("  display: block;");
client.println("}");
client.println();
client.println("/* Style the indicator (dot/circle) */");
client.println(".container .checkmark:after {");
client.println("        top: 9px;");
client.println("        left: 9px;");
client.println("        width: 8px;");
client.println("        height: 8px;");
client.println("        border-radius: 50%;");
client.println("        background: white;");
client.println("}");
client.println("</style></head>");
client.println();
client.println();
client.println("<body><h1 style='color:white;'>Matrix Kontrol</h1>");
client.println("  <div class='div1'>");
client.println("    <h2 style='color:rgb(74, 255, 210); text-decoration: underline;'>Durum</h2>");
client.println("  <p class='mytext3'>"+ saatHtml() +"</p>");
client.println("  <p class='mytext3'>"+ gunler[myRTC.dayofweek-1] +"</p>");
client.println("  <p class='mytext2'>Sıcaklık: "+ String(t) +"°C</p>");
client.println("  <p class='mytext2'>Nem: %"+ String(h) +"</p>");
if (topM < 10)
  client.println("  <p class='mytext2'>Açılma Saati: "+ String(topH) + ":0" + String(topM) + "</p>");
else
  client.println("  <p class='mytext2'>Açılma Saati: "+ String(topH) + ":" + String(topM) + "</p>");
if (bottomM < 10)
  client.println("  <p class='mytext2'>Kapanma Saati: "+ String(bottomH) + ":0" + String(bottomM) + "</p>");
else
  client.println("  <p class='mytext2'>Kapanma Saati: "+ String(bottomH) + ":" + String(bottomM) + "</p>");  
client.println("  <p class='mytext2'>Çalışma Zamanı: "+ String(getUptime()) +" Gün</p>");
client.println("  <p class='mytext2' style='color:maroon; text-decoration:underline;'>-> Gösterilecek metin:</p>");
client.println("  <p class='mytext2' style='font-family: Arial;'>"+ ttbp +"</p>");
client.println("  <p class='mytext2' style='color:maroon; text-decoration:underline;'>--> Metin 2:</p>");
client.println("  <p class='mytext2' style='font-family: Arial;'>"+ ttbp2 +"</p>");
client.println("  <p class='mytext2' style='color:maroon; text-decoration:underline;'>--> Metin 3:</p>");
client.println("  <p class='mytext2' style='font-family: Arial;'>"+ ttbp3 +"</p>");
client.println("  <pre>");
client.println();
client.println();
client.println();
client.println("  </pre>");
client.println("  <div><a href='/reboot' style='display: block;' ><button class='button3'>Reset</button></a></div>");
client.println("  </div>");
client.println();
client.println("<div class='div2'>");
client.println("    <h2 style='color:rgb(74, 255, 210); text-decoration: underline;'>Mod Seçimi</h2>");

switch(mod){
  case 1:
client.println("<p><label class='container2'><a class='container' style='color:rgb(0, 150, 0)' href='/mode/1'>Saat <input type='radio'  name='radio' checked='checked'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/2'>Termometre ve Nem Ölçer <input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/3'>Sadece Termometre <input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/4'>Sadece Nem Ölçer <input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/5'>Saniye<input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/6'>Print Text <input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/7'>Varsayılan <input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");


  break;

  case 2:
client.println("<p><label class='container2'><a class='container' href='/mode/1'>Saat <input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' style='color:rgb(0, 150, 0)' href='/mode/2'>Termometre ve Nem Ölçer <input type='radio'  name='radio' checked='checked'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container'  href='/mode/3'>Sadece Termometre <input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/4'>Sadece Nem Ölçer <input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/5'>Saniye<input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/6'>Print Text <input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/7'>Varsayılan <input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");

  break;

  case 3:
client.println("<p><label class='container2'><a class='container' href='/mode/1'>Saat <input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/2'>Termometre ve Nem Ölçer <input type='radio'  name='radio' ><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' style='color:rgb(0, 150, 0)' href='/mode/3'>Sadece Termometre <input type='radio'  name='radio' checked='checked'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/4'>Sadece Nem Ölçer <input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/5'>Saniye<input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/6'>Print Text <input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/7'>Varsayılan <input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");

  break;

  case 4:
client.println("<p><label class='container2'><a class='container' href='/mode/1'>Saat <input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/2'>Termometre ve Nem Ölçer <input type='radio'  name='radio' ><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/3'>Sadece Termometre <input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' style='color:rgb(0, 150, 0)' href='/mode/4'>Sadece Nem Ölçer <input type='radio'  name='radio' checked='checked'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/5'>Saniye<input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/6'>Print Text <input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/7'>Varsayılan <input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");

  break;

  case 5:
client.println("<p><label class='container2'><a class='container' href='/mode/1'>Saat <input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/2'>Termometre ve Nem Ölçer <input type='radio'  name='radio' ><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/3'>Sadece Termometre <input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/4'>Sadece Nem Ölçer <input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' style='color:rgb(0, 150, 0)' href='/mode/5'>Saniye<input type='radio'  name='radio' checked='checked'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/6'>Print Text <input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/7'>Varsayılan <input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");

  break;

    case 6:
client.println("<p><label class='container2'><a class='container' href='/mode/1'>Saat <input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/2'>Termometre ve Nem Ölçer <input type='radio'  name='radio' ><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/3'>Sadece Termometre <input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/4'>Sadece Nem Ölçer <input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/5'>Saniye<input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' style='color:rgb(0, 150, 0)' href='/mode/6'>Print Text <input type='radio'  name='radio' checked='checked'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/7'>Varsayılan <input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");

  break;

  case 7:
client.println("<p><label class='container2'><a class='container' href='/mode/1'>Saat <input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/2'>Termometre ve Nem Ölçer <input type='radio'  name='radio' ><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/3'>Sadece Termometre <input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/4'>Sadece Nem Ölçer <input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/5'>Saniye<input type='radio'  name='radio'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' href='/mode/6'>Print Text <input type='radio'  name='radio' checked='checked'><span class='checkmark'></span></a></label></p>");
client.println("<p><label class='container2'><a class='container' style='color:rgb(0, 150, 0)' href='/mode/6'>Varsayılan <input type='radio'  name='radio' checked='checked'><span class='checkmark'></span></a></label></p>");

  break;
  
  default:
  ;
}
client.println("</div>");
client.println();
client.println("<pre>");
client.println();
client.println("</pre>");
client.println();
client.println("<div class='div3'>");
client.println("  <div id='container3'>");
client.println("    <p></p><div class='textbox mytext'>Oto Gece Modu</div>");
if (!isAutoNMEnabled)
  client.println("    <a href='/anm/on'><button class='button'>OFF</button></a><p></p>");
else
  client.println("    <a href='/anm/off'><button class='button button2'>ON</button></a><p></p>");
client.println("    </div>");
client.println();
client.println("    <div id='container3'>");
if (!isAutoNMEnabled){
client.println("      <p></p><div class='textbox mytext'>Gece Modu</div>");
  if(isDayMode)
    client.println("      <a href='/nm/on'><button class='button '>OFF</button></a><p></p>");
  else
    client.println("      <a href='/nm/off'><button class='button button2'>ON</button></a><p></p>");

}

else{
 client.println("        <p></p><div class='textbox mytext'>Gece Modu</div>");
  if(isDayMode)
    client.println("        <a ><button class='button' disabled='disabled' style='background-color:#333'>OFF</button></a><p></p>");
  else
    client.println("        <a ><button class='button button2' disabled='disabled' style='background-color:#333'>ON</button></a><p></p>");
}

client.println("        </div>");
client.println("</div>");
client.println();
client.println("</body></html>");

            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();/*
    //Serial.println("Client disconnected.");
    //Serial.println("");*/
  }

  // some vital jobs
  if(isAutoNMEnabled)
    autoNM();

  if(isUpdatable)
    updateTN(); 

  changeIsUpdatable();

  clrTtbps();

  updateEEPROM();

  parseTtbp2();

// çalışma süresi ResetTime'ı geçmişse reset at
  if(millis() >= ResetTime ){
  myMatrix.setTextAlignment(PA_CENTER); 
  myMatrix.print("wait");
  delay(1000); // deneylerim sonucunda reset öncesi biraz delay randımanlı çalışması için gerekli
  resetFunc(); 
  }
  
  if(canIreset){
    myMatrix.setTextAlignment(PA_CENTER);
    myMatrix.print("wait");
    delay(1000);
     resetFunc();
  }
  
  //otadan yazılım atılırsa yükle
  ArduinoOTA.handle();

  //kumandadan gelen bilgileri sisteme aktar
  if (irrecv.decode(&results)){
    switch(results.value){
    case BTN_NUM0:
    mod = 0;
    break;
    
    case BTN_NUM1:
    mod = 1;
    break;
    
    case BTN_NUM2:
    mod = 2;
    break;
    
    case BTN_NUM3:
    mod = 3;
    break;

    case BTN_NUM4:
    mod = 4;
    break;

    case BTN_NUM5:
    mod = 5;
    break;

    case BTN_NUM6:
    mod = 6;
    break;

    case BTN_NUM7:
    mod = 7;
    break;

    case BTN_OK:
    if(mod == 0)
      canIreset = true;
    break;
    
    case BTN_ASTERISK:
    isAutoNMEnabled = !isAutoNMEnabled; // toggle auto night mode
    myMatrix.setTextAlignment(PA_CENTER);
    if(isAutoNMEnabled)
      myMatrix.print("OGM: 1");// oto gece modu açık (GM: gece modu)
    else
      myMatrix.print("OGM: 0");// oto gece modu kapalı (GM: gece modu)
    delay(1500);
    break;
    
    case BTN_HASHTAG:
    if(!isAutoNMEnabled){
    isDayMode = !isDayMode; //toggle night/day mode
    myMatrix.setTextAlignment(PA_CENTER);
    if(isDayMode)
      myMatrix.print("GM: 0");// daymode ise nightmode kapalı (GM: gece modu)
    else
      myMatrix.print("GM: 1");// daymode değil ise nightmode açık (GM: gece modu)
    delay(1500);
    }
    else
      myMatrix.print("auto:1");
      delay(1500);
    break;
    
    default:
     ;
    }
    
    irrecv.resume();
    }

  //matrixte göster
  if(isDayMode){
    switch(mod){
      case 0:
      myMatrix.print("Reset?");
      break;
      
      case 1:
      saat();
      break;

      case 2:
      termoNem();
      break;
      
      case 3:
      termo();
      break;

      case 4:
      nem();
      break;
      
      case 5:
      saniye();
      break;

      case 6:
      printText(ttbp);
      break;

      case 7:
      multi();
      break;

      default:
      myMatrix.setTextAlignment(PA_CENTER);
      myMatrix.print("e: mod");
      break;
    }
  }
  else
    myMatrix.print(" "); //boşluk bas ki sönsün 
  
}

//EEPROM'U UPDATE EDEN FUNCT
void updateEEPROM(){
  if(mod != 0 && mod != readEEPROM(addrMode) ){
      writeEEPROM(addrMode, mod);
      myMatrix.print("u: mod");
      delay(500);
   }

   if (bottomH != readEEPROM(addrBottomH)){
      writeEEPROM(addrBottomH, bottomH);
      myMatrix.print("u: alH");
      delay(500);
   }

   if (bottomM != readEEPROM(addrBottomM)){
      writeEEPROM(addrBottomM, bottomM);
      myMatrix.print("u: alM");
      delay(500);
   }

   if (topH != readEEPROM(addrTopH)){
      writeEEPROM(addrTopH, topH);
      myMatrix.print("u: usH");
      delay(500);
   }

   if (topM != readEEPROM(addrTopM)){
      writeEEPROM(addrTopM, topM);
      myMatrix.print("u: usM");
      delay(500);
   }

}

// Function to write to EEPROOM
void writeEEPROM(int address, byte val)
{
  // Begin transmission to I2C EEPROM
  softwarei2c.beginTransmission(DEVICE);
 
  // Send memory address as two 8-bit bytes
  softwarei2c.write((int)(address >> 8));   // MSB
  softwarei2c.write((int)(address & 0xFF)); // LSB
 
  // Send data to be stored
  softwarei2c.write(val);
 
  // End the transmission
  softwarei2c.endTransmission();
 
  // Add 5ms delay for EEPROM
  delay(5);
  //Serial.println("yazıldı");
}
 
// Function to read from EEPROM
byte readEEPROM(int address)
{
  // Define byte for received data
  byte rcvData = 0xFF;
 
  // Begin transmission to I2C EEPROM
  softwarei2c.beginTransmission(DEVICE);
 
  // Send memory address as two 8-bit bytes
  softwarei2c.write((int)(address >> 8));   // MSB
  softwarei2c.write((int)(address & 0xFF)); // LSB
 
  // End the transmission
  softwarei2c.endTransmission();
 
  // Request one byte of data at current memory address
  softwarei2c.requestFrom(DEVICE, 1);
 
  // Read the data and assign to variable
  rcvData =  softwarei2c.read();
 
  // Return the data as function output
  return rcvData;
}
