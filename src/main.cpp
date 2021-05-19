

#include <Arduino.h>
#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <BlynkSimpleEsp8266.h>
#include "DHT.h"
#include <LiquidCrystal_I2C.h>
#include <Wire.h> 
#include <SimpleTimer.h>
#include <TimeLib.h>
#include <WidgetRTC.h>


#define DHTPIN D3
#define pump   D4
#define pump_a D5
#define pump_b D6
#define photoresistor D7
#define float_sensor D8

#define grow_light_pin 9
#define fan_pin 10

#define DHTTYPE DHT22 
#

//define virtual pin in Blynk
#define BLYNK_PIN_WIFI_SIGNAL           V11
#define BLYNK_PIN_MIN_AIR_TEMP          V20
#define BLYNK_PIN_MAX_AIR_TEMP          V21
#define BLYNK_PIN_MIN_AIR_HUMIDITY      V22
#define BLYNK_PIN_MAX_AIR_HUMIDITY      V23
#define BLYNK_PIN_MIN_pH                V24
#define BLYNK_PIN_MAX_pH                V25
#define BLYNK_PIN_TIME_PUMP_ACID_ON     V26
#define BLYNK_PIN_TIME_PUMP_BASE_ON     V27
#define BLYNK_PIN_GROW_LIGHT            V28

#define BLYNK_PIN_PUMP_ON_OFF           V40
#define BLYNK_PIN_THRESHOLD             V73
//SimpleTimer timer;
WidgetRTC rtc;
WiFiClientSecure client;
char auth[] = "wYQzdV9DOeIhA_sMkxroL_U7AslDK3dC";
//setup lcd
LiquidCrystal_I2C lcd(0x27,20,4);

//ssid-password
//const char* ssid     = "Internal";
//const char* password = "Passio@2021";

//const char* ssid     = "PassioCoffee";
//const char* password = "19009434";

//const char* ssid     = "The Coffee House";
//const char* password = "thecoffeehouse";

const char* ssid     = "PNAnh";
const char* password = "pnanh123";
//variable of array
int temp;
int n=10;
float a[10];
//api googlesheet
const char* host = "script.google.com"; 
const char* host_local = "192.168.1.175";
const char* fingerprint = "ac b5 51 ec 4d d7 3a 05 36 fe 80 1a 63 05 82 a8 84 9c 0d ed";
String url;
//pin pH sensor
const int ph_sensor = A0;
int ph_value = 0;
int buf[10]; 
float phValue;
// Desired range for pH
float range_ph_high = 6.2;
float range_ph_low = 5.5;
// pH range that will immediately cause a pH correction
float range_ph_high_danger = 7.0;
float range_ph_low_danger = 5.0;
// prediction variables
float test_value, pred_value, mae, threshold;
String status_pH_sensor;
//Blynk variable
int wifisignal;
int MIN_AIR_TEMP_ALM = 10;
int MAX_AIR_TEMP_ALM = 50;
// time input variable
const int MAX_SCHEDULE = 1;          //number of available schedules
const int START_TIMER_PIN = V35;      //START_T IMER_PIN is the virtualpin of  
unsigned long startseconds[MAX_SCHEDULE]; //th ine FIRST timerput pin. In case of  
unsigned long stopseconds[MAX_SCHEDULE];    //MAX_SCHEDULE = 3, the widget pins 
bool scheduleWeekDay[MAX_SCHEDULE][7];    //will be V10, V11 and V12
DHT dht(DHTPIN, DHTTYPE);
// sensor
int photoresistor_bool, float_sensor_bool;

//Widget LED
BlynkTimer timer;
WidgetLED led_pump_acid(V50);
WidgetLED led_pump_base(V51);
WidgetLED led_pump(V52);
WidgetLED led_growlight(V53);
WidgetLED led_fan(V54);

void setSchedule(const BlynkParam& param, int nSchedule)
{         
  TimeInputParam t(param);
  Serial.println("timer updated");
  startseconds[nSchedule] =  t.getStartHour()*3600 + t.getStartMinute()*60;
  stopseconds[nSchedule]  =  t.getStopHour()*3600  + t.getStopMinute()*60;
  for(int day = 1; day <=7; day++) { scheduleWeekDay[nSchedule][day%7] = t.isWeekdaySelected(day); } //Time library starts week on Sunday=1, Blynk on Monday=1, need to translate Blynk value to time value!! AND need to run from 0-6 instead of 1-7
}

BLYNK_WRITE(V35) {
  TimeInputParam t(param);
  setSchedule(param,0);
  // Process start time

  if (t.hasStartTime())
  {
    Serial.println(String("Start: ") +
                   t.getStartHour() + ":" +
                   t.getStartMinute() + ":" +
                   t.getStartSecond());
  }
  else if (t.isStartSunrise())
  {
    Serial.println("Start at sunrise");
  }
  else if (t.isStartSunset())
  {
    Serial.println("Start at sunset");
  }
  else
  {
    // Do nothing
  }

  // Process stop time

  if (t.hasStopTime())
  {
    Serial.println(String("Stop: ") +
                   t.getStopHour() + ":" +
                   t.getStopMinute() + ":" +
                   t.getStopSecond());
  }
  else if (t.isStopSunrise())
  {
    Serial.println("Stop at sunrise");
  }
  else if (t.isStopSunset())
  {
    Serial.println("Stop at sunset");
  }
  else
  {
    // Do nothing: no stop time was set
  }

  // Process timezone
  // Timezone is already added to start/stop time

  Serial.println(String("Time zone: ") + t.getTZ());

  // Get timezone offset (in seconds)
  Serial.println(String("Time zone offset: ") + t.getTZ_Offset());

  // Process weekdays (1. Mon, 2. Tue, 3. Wed, ...)

  for (int i = 1; i <= 7; i++) {
    if (t.isWeekdaySelected(i)) {
      Serial.println(String("Day ") + i + " is selected");
    }
  }

  Serial.println();
}


void checkSchedule(){
  //setSchedule(,0);
  for (int nSchedule=0; nSchedule<MAX_SCHEDULE; nSchedule++){
    if( scheduleWeekDay[nSchedule][weekday()-1] )
    {    //Schedule is ACTIVE today
      unsigned long nowseconds =  hour()*3600 + minute()*60 + second();
      String s1 = String(nowseconds);
      String s2 = String(startseconds[0]);
      Serial.println("check schedule:");
      Serial.println(s1);
      Serial.println(s2);
  
      if(nowseconds >= startseconds[nSchedule] - 3 && nowseconds <= startseconds[nSchedule] + 3 )
      {  // 62s on 60s timer ensures 1 trigger command is sent
        //PUT YOUR START ACTIVITY CODE HERE!!
        Serial.println("Schedule started");
      //for example light up a LED at virtual pin 40
        Blynk.virtualWrite(V60, 200); //LED brightness runs from 0(off) to 255 (brightest)
      }
      if(nowseconds >= startseconds[nSchedule] - 3 && nowseconds <= stopseconds[nSchedule] + 3 )
      {
        Blynk.virtualWrite(V60, 200);
      }  
      else   Blynk.virtualWrite(V60, 0);  
  
      if(nowseconds >= stopseconds[nSchedule] - 3 && nowseconds <= stopseconds[nSchedule] + 3 )
      {
        //PUT YOUR STOP ACTIVITY CODE HERE!!
        Serial.println("Schedule ended");
        Blynk.virtualWrite(V60, 0);
      }//seconds
    }
    else Blynk.virtualWrite(V60, 0);
    //day
  }//for
}//checkSchedule
BLYNK_WRITE_DEFAULT()
{  //Read widget updates coming in from APP
  int pin = request.pin;
  //if(pin >= V35 && pin <= (START_TIMER_PIN + MAX_SCHEDULE) )
  Serial.println("check write:"+pin);
  if(pin == V35)
  {
      int nSchedule = pin - START_TIMER_PIN;
      Serial.println(String("Schedule ") + nSchedule + "update");
      setSchedule(param,nSchedule);
      
  }
  //you can add more vpin X checks in this route OR use BLYNK_WRITE(VX) as usual
}
BLYNK_WRITE(BLYNK_PIN_PUMP_ON_OFF) 
{
  int boolean = param.asInt();
  if (boolean == 1)
  {
    digitalWrite(pump,HIGH);
    led_pump.on();
  }
  if (boolean == 0)
  {
    digitalWrite(pump,LOW);
    led_pump.off();
  }
}

void sendWifi()
{
  wifisignal = map(WiFi.RSSI(), -105, -40, 0, 100);
  Blynk.virtualWrite(BLYNK_PIN_WIFI_SIGNAL, wifisignal);
  //Serial.println(wifisignal);
}
void clockvalue() // Digital clock display of the time
{

  int gmthour = hour();
  if (gmthour == 24)
  {
    gmthour = 0;
  }
  String displayhour = String(gmthour, DEC);
  int hourdigits = displayhour.length();
  if (hourdigits == 1)
  {
    displayhour = "0" + displayhour;
  }
  

  String displayminute = String(minute(), DEC);
  int minutedigits = displayminute.length();
  if (minutedigits == 1)
  {
    displayminute = "0" + displayminute;
  }


  String displaysecond = String(second(), DEC);
  int seconddigits = displaysecond.length();
  if (seconddigits == 1)
  {
    displaysecond = "0" + displaysecond;
  }
  String timenow = displayhour + ":" + displayminute + ":" + displaysecond ;
  Blynk.virtualWrite(V1, timenow);
  Serial.println(timenow);
  //displaycurrenttimepluswifi = "                                          Clock:  " + displayhour + ":" + displayminute + "               Signal:  " + wifisignal + " %";
  //Blynk.setProperty(V3, "label", displaycurrenttimepluswifi);
}
BLYNK_APP_DISCONNECTED() 
{
  Serial.println("App Disconnected.");
}
void datevalue()
{
  String displayday = String(day(), DEC);
  int daydigits = displayday.length();
  if (daydigits == 1)
  {
    displayday = "0" + displayday;
  }

  String displaymonth = String(month(), DEC);
  int monthdigits = displaymonth.length();
  if (monthdigits == 1)
  {
    displaymonth = "0" + displaymonth;
  }

  String currentDate = displayday + "/" + displaymonth + "/" + year();
     // Send date to the App
  Blynk.virtualWrite(V2, currentDate);
  Serial.print(currentDate);
  clockvalue();
  sendWifi();

}
BLYNK_CONNECTED() {
  // Synchronize time on connection
  rtc.begin();
  Blynk.syncAll();
}
BLYNK_WRITE(BLYNK_PIN_MIN_AIR_TEMP)
{
  // Get value as integer
    MIN_AIR_TEMP_ALM = param.asInt();
    Serial.print("V20:");
    Serial.println(MIN_AIR_TEMP_ALM);

}
BLYNK_WRITE(BLYNK_PIN_MAX_AIR_TEMP)
{
  
    MAX_AIR_TEMP_ALM = param.asInt();
    Serial.print("V21:");
    Serial.println(MAX_AIR_TEMP_ALM);
  
}

int MIN_AIR_HUMIDITY = 30;
BLYNK_WRITE(BLYNK_PIN_MIN_AIR_HUMIDITY)
{
  MIN_AIR_HUMIDITY = param.asInt(); // Get value as integer
  Serial.print("V22:");
  Serial.println(MIN_AIR_HUMIDITY);
}

int MAX_AIR_HUMIDITY = 95;
BLYNK_WRITE(BLYNK_PIN_MAX_AIR_HUMIDITY)
{
  MAX_AIR_HUMIDITY = param.asInt(); // Get value as integer
  Serial.print("V23:");
  Serial.println(MAX_AIR_HUMIDITY);
}
BLYNK_WRITE(BLYNK_PIN_MIN_pH)
{
  range_ph_low_danger = param.asFloat(); // Get value as integer
  Serial.print("V24:");
  Serial.println(range_ph_low_danger);
}
BLYNK_WRITE(BLYNK_PIN_MAX_pH)
{
  range_ph_high_danger = param.asFloat(); // Get value as integer
  Serial.print("V25:");
  Serial.println(range_ph_high_danger);
}

int TIME_PUMP_ACID_ON = 1000;
BLYNK_WRITE(BLYNK_PIN_TIME_PUMP_ACID_ON)
{
  TIME_PUMP_ACID_ON = param.asInt() * 1000; // Get value as seconds, converting to ms
  Serial.print("V26:");
  Serial.println(TIME_PUMP_ACID_ON);
  
}

int TIME_PUMP_BASE_ON = 1000;
BLYNK_WRITE(BLYNK_PIN_TIME_PUMP_BASE_ON)
{
  TIME_PUMP_BASE_ON = param.asInt() * 1000; // Get value as seconds, converting to ms
   Serial.print("V27:");
  Serial.println(TIME_PUMP_BASE_ON);
}
BLYNK_WRITE(BLYNK_PIN_GROW_LIGHT)
{
  int on_off = param.asInt(); 
  Serial.print("V28:");
  Serial.println(on_off);
}
BLYNK_WRITE(BLYNK_PIN_THRESHOLD)
{
  threshold = param.asFloat(); 
  Serial.print("V73:");
  Serial.println(threshold);
}
void turn_on_fan()
{
  digitalWrite(fan_pin, HIGH);
  Blynk.virtualWrite(V54, 200);
}
void turn_off_fan()
{
  digitalWrite(fan_pin, LOW);
  Blynk.virtualWrite(V54, 0);
}


float measure_ph()
{ //lưu 10 giá trị đọc từ cảm biến vào buffer
  for (int i = 0; i < 10; i++)
  {
    buf[i] = analogRead(ph_sensor);
    delay(30);
  }
  // sắp xếp giá trị trong buffer tăng dần
  for (int i = 0; i < 9; i++)
  {
    for (int j = i + 1; j < 10; j++)
    {
      if (buf[i] > buf[j])
      {
        temp = buf[i];
        buf[i] = buf[j];
        buf[j] = temp;
      }
    }
  }
  // lấy giá trị trung bình 6 giá trị ở giữa
  int avgValue = 0;
  for (int i = 2; i < 8; i++)
    avgValue += buf[i];
  float pHVol = (float)avgValue*  3.3 / 1024 / 6;
  //float phValue = 14* pHVol/3.3;
  //float phValue = 4.97*pHVol - 1.24;
  float phValue = 4.25715*pHVol - 0.78549;
  return phValue;
}
void display_lcd(float t, float h, float phValue, int float_sensor, int photoresistor )
{ 
  //Page 1
  lcd.clear();
  lcd.setCursor(2,0);  // Print a message to the LCD
  lcd.print("Gia tri cam bien:"); 
  lcd.setCursor(0, 1);  // Print a message to the LCD
  lcd.print("Temp:");  lcd.print(t); lcd.print("oC");
  lcd.setCursor(0, 2);  // Print a message to the LCD
  lcd.print("Hud:");  lcd.print(h); lcd.print("%");
  lcd.setCursor(0, 3);  // Print a message to the LCD
  lcd.print("PH:");  lcd.print(phValue);
  // Page 2
  delay(2000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Float_sensor:"); lcd.print(float_sensor + String(" bool") );
  lcd.setCursor(0,1);
  lcd.print("Photoresistor:"); lcd.print(photoresistor + String(" bool") );
  

}
void sendSensor()
{
  float h = dht.readHumidity();
  float t = dht.readTemperature(); // or dht.readTemperature(true) for Fahrenheit
  phValue = measure_ph();
  Serial.println(phValue);
  // Xuất giá trị cảm biến ra lcd
  display_lcd(t, h, phValue, float_sensor_bool, photoresistor_bool);

  if (isnan(h) || isnan(t)) 
  {
    Serial.println("Failed to read from DHT sensor!");
    //return;
    t = 50;
    h= 1000;
  }
  Serial.println(int(t));
  Serial.println(int(MAX_AIR_TEMP_ALM));
  if ( int(t) >= MAX_AIR_TEMP_ALM)
  {
    Blynk.notify("Temperature is too high: ");
    turn_on_fan();
    Serial.println("Turn on fan");

  }
  else turn_off_fan();
  
  if ( t <= float(MIN_AIR_TEMP_ALM))
  {
    Blynk.notify("Temperature is too low: ");

  }

   if ( h >= MAX_AIR_HUMIDITY)
  {
    Blynk.notify("Humidity is too high: ");

  }
   if ( h <= MIN_AIR_HUMIDITY)
  {
    Blynk.notify("Humidity is too low: ");

  }
   if ( phValue >= range_ph_high_danger)
  {
    Blynk.notify("pH is too high: ");

  }
   if ( phValue <= range_ph_low_danger)
  {
    Blynk.notify("pH is too low: ");

  }


  // You can send any value at any time.
  // Please don't send more that 10 values per second.
  Blynk.virtualWrite(V5, h);
  //delay(50);
  Blynk.virtualWrite(V6, t);
  //delay(50);
  Blynk.virtualWrite(V7, phValue);
}
void send_data_to_googlesheet() {

  client.setInsecure();
  Serial.print("connecting to ");
  Serial.println(host);

  const int httpPort = 443;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    //return;
  }

 
 
 if (client.verify(fingerprint,host )) {
  Serial.println("certificate matches");
  } else {
  Serial.println("certificate doesn't match");
  }

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  //Serial.println(t);
  //Serial.println(h);
  if (isnan(h) || isnan(t)) {
     Serial.println("Failed to read from DHT sensor!");
     t = 50;
     h = 1000;
      //return;
  }
  //https://script.google.com/macros/s/AKfycbywGIJQEbRYQUY7ROeF7jTHdO91HbiNHAQtq9qYDGLn5-obhTZjYQPcJT-oLv0X7_m8/exec
  //https://script.google.com/macros/s/AKfycbxuqxXnLdb34Pg4a_ie1MaYWhHWXZNo1l_VubxRnFw9MPrP-lp5o0PHhZ7pv7GV3ktv/exec
  //https://script.google.com/macros/s/AKfycbzjy0mwez0NApENozEMBG96-kX9C8ihkYouLMSF8o3PRXZQ-5fiBbi7qzRBeOsc3A/exec
  url = "/macros/s/AKfycbywGIJQEbRYQUY7ROeF7jTHdO91HbiNHAQtq9qYDGLn5-obhTZjYQPcJT-oLv0X7_m8/exec?value1="+ String(t)+
        "&value2=" + String(h) + "&value3=" + String(phValue) + "&value4=" + String(test_value)
        + "&value5=" + String(pred_value) + "&value6=" + String(mae) + "&value7=" + status_pH_sensor;
  Serial.print("Requesting URL: ");
  Serial.println(url);
  
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  delay(500);
  String section="header";
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  Serial.println();
  Serial.println("closing connection");
}
void send_data_to_webserver()
{
  float h = dht.readHumidity();
  float t = dht.readTemperature(); // or dht.readTemperature(true) for Fahrenheit

  if (isnan(h) || isnan(t)) 
  {
    t = 50;
    h = 100;
    Serial.println("Failed to read from DHT sensor!");
    //return;
  }
 for (int i=0; i<n; i++)
  {
  if (a[i] == 0) a[i] = phValue ;

  }
  temp = a[n-1];
  for (int i=n-1;i>=0;i--)
  {
    a[i+1] = a[i];
  }
   a[0] = phValue;

for (int i=0; i<n; i++)
  {
  Serial.print(a[i]);
  Serial.print(" ");  
  }
  Serial.println();

if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
 
    HTTPClient http;  //Declare an object of class HTTPClient
 
    http.begin("http://192.168.1.11:6868/predict?value1="+String(a[0],2)
                                                +"&value2="+String(a[1],2)
                                                +"&value3="+String(a[2],2)
                                                +"&value4="+String(a[3],2)
                                                +"&value5="+String(a[4],2)
                                                +"&value6="+String(a[5],2)
                                                +"&value7="+String(a[6],2)
                                                +"&value8="+String(a[7],2)
                                                +"&value9="+String(a[8],2)
                                                +"&value10="+String(a[9],2)
                                                );  
    String test_code;
    test_code = ("http://192.168.1.11:6868/predict?value1="+String(a[0],2)
                                                +"&value2="+String(a[1],2)
                                                +"&value3="+String(a[2],2)
                                                +"&value4="+String(a[3],2)
                                                +"&value5="+String(a[4],2)
                                                +"&value6="+String(a[5],2)
                                                +"&value7="+String(a[6],2)
                                                +"&value8="+String(a[7],2)
                                                +"&value9="+String(a[8],2)
                                                +"&value10="+String(a[9],2)
                                                );
                                                Serial.println(test_code);
    int httpCode = http.GET(); //Send the request
    if (httpCode > 0) { //Check the returning code
 
      String payload = http.getString();   //Get the request response payload
      Serial.println(payload);             //Print the response payload
      int index1, index2, index3;
      index1 = payload.indexOf("a");
      index2 = payload.indexOf("b");
      index3 = payload.indexOf("c");
      
      String data1, data2, data3;
      data1 = payload.substring(0, index1);
      data2 = payload.substring(index1 + 1, index2);
      data3 = payload.substring(index2 + 1, index3);
      
      test_value = data1.toFloat();
      pred_value = data2.toFloat();
      mae        = data3.toFloat(); 

      Serial.println(test_value);  
      Serial.println(pred_value);  
      Serial.println(mae); 

      
      Blynk.virtualWrite(V70, test_value);
      Blynk.virtualWrite(V71, pred_value);
      Blynk.virtualWrite(V72, mae);

      if (mae > threshold)
      {
        status_pH_sensor = "Anomaly_Detection";
        Blynk.virtualWrite(V74, status_pH_sensor);
      }  
      else 
       {
        status_pH_sensor = "Normal";
        Blynk.virtualWrite(V74, status_pH_sensor);
      }
    }
 
    http.end();   //Close connection
 
  }
 

}
void peristaltic_pump()//pump_a = acid
{
  
  //float phValue = measure_ph();
  //Serial.println(phValue);
  if (phValue < range_ph_low_danger)
  {
    digitalWrite(pump_b,HIGH); // add base solution
    led_pump_base.on();
    delay(TIME_PUMP_BASE_ON);
    digitalWrite(pump_b,LOW);
    led_pump_base.off();
    Serial.println("add base");
  }
  else if(phValue > range_ph_high_danger)
  {
    digitalWrite(pump_a,HIGH); // add acid solution
    led_pump_acid.on();
    delay(TIME_PUMP_ACID_ON);
    digitalWrite(pump_a,LOW);
    led_pump_acid.off();
    Serial.println("add acid");
    
    
  }
}

void ICACHE_RAM_ATTR growlight()
{ 
  photoresistor_bool = digitalRead(photoresistor);
  if (photoresistor_bool == HIGH)
  {
    led_growlight.on();
    Serial.println("TURN ON GROW LIGHT");
    digitalWrite(grow_light_pin, HIGH);
  }
  else if (photoresistor_bool == LOW)
  {
    led_growlight.off();
    Serial.println("TURN OFF GROW LIGHT");
    digitalWrite(grow_light_pin, LOW);
  }

}

void ICACHE_RAM_ATTR pump_water()
{
  led_pump.on();
  //Serial.println("bơm nước");
  float_sensor_bool = digitalRead(float_sensor);
  if (float_sensor_bool == LOW)
  {
    digitalWrite(pump,HIGH);
    Serial.println("pumping");
    led_pump.on();
  }
  else if(float_sensor_bool == HIGH)
  {
    digitalWrite(pump,LOW);
    Serial.println("not pumping");
    led_pump.off();
  }
 

}
void setup() {
  //lcd init
  /*
  lcd.init();       //Start the LC communication
  lcd.backlight();  //Turn on backlight for LCD
  lcd.setCursor(6, 0);  // Print a message to the LCD
  lcd.print("Loading:");
  */
  //define pinmode
  pinMode(pump_a, OUTPUT);
  pinMode(pump_b, OUTPUT);
  pinMode(pump, OUTPUT);
  pinMode(photoresistor,INPUT_PULLUP);
  pinMode(float_sensor, INPUT_PULLUP);
  //pinMode(fan_pin, OUTPUT);
  //pinMode(grow_light_pin, OUTPUT);
  //
  Serial.begin(115200);
  delay(100);
  dht.begin();
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password); 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Netmask: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("Gateway: ");
  Serial.println(WiFi.gatewayIP());
  Blynk.begin(auth, ssid, password, IPAddress(192,168,1,11),8080);
  timer.setInterval(5000L, sendSensor);
  timer.setInterval(5000L, datevalue);
  timer.setInterval(5000L, send_data_to_googlesheet);
  timer.setInterval(5000L, send_data_to_webserver);
  timer.setInterval(5000L, peristaltic_pump);
  timer.setInterval(5000L, checkSchedule); 
  attachInterrupt(digitalPinToInterrupt(photoresistor), growlight, CHANGE);
  attachInterrupt(digitalPinToInterrupt(float_sensor), pump_water, CHANGE);
}

void loop()
{
  Blynk.run();
  timer.run(); 
}


