//libraries
#include <TinyGPS++.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <Servo.h>
#include <SoftwareSerial.h>

#define ledPin 5
#define buzzer 6
#define servoPin 23
#define dropAlt 300
#define ascAlt 10

//Vars
float seaLvl;
int ledFreq = 2000; //starting LED frequency
int lastAlt = 0;
unsigned long time, startTime;
unsigned long lastGroundCheck = 0;
unsigned long lastBlink = 0;

unsigned long lastBuzz = 0;

//setups
Adafruit_BME280 bme; //placeholder for barometer
TinyGPSPlus gps;
Servo myServo;


void flashLed()
{
  unsigned long currentTime = millis();
  if((currentTime - lastBlink) >= ledFreq)
  {
    lastBlink = currentTime;
  digitalWrite(ledPin, !digitalRead(ledPin));
  }
}

void deployPayload()
{
  myServo.write(40); //offset servo position by 90 degrees
  delay(500);
  myServo.detach();
}

void pulseBuzzer()
{
  unsigned long currentTime = millis();
  if((currentTime - lastBuzz) >= 800)
  {
    lastBuzz = currentTime;
  digitalWrite(buzzer, !digitalRead(buzzer));
  }
}



void preFlight()
{
  Serial3.println("Pre-Flight Start"); //PRE FLIGHT START DEBUG
  ledFreq = 2000;
  int curAlt = 0;
  while(curAlt < ascAlt) //loops until altitude increases 1m above set seaLvl
  {
    curAlt = bme.readAltitude(seaLvl); // DEBUG
    Serial.printf("%i\n", curAlt); //DEBUG
    delay(100);
    flashLed();
  }
  startTime = millis();
}



void ascentFlight()
{
  Serial3.println("Ascent Started"); //ascent FLIGHT START DEBUG
  ledFreq = 1000;
  int curAlt = 0;
  while(curAlt<dropAlt) // run until altitude passes defined dropAlt
  {
    processData();//takes & records data for 1 sec
    curAlt = bme.readAltitude(seaLvl);
    Serial.printf("%i\n", curAlt); //DEBUG
  }
  deployPayload();
  Serial.println("Deployed-----&");//DEBUG
}



void processData() //for 1 second, takes data every 250ms then averages the temperature and pressure values for that second, transmits data with outputData()
{
  unsigned long currentTime = millis();
  unsigned long lastRec = 0; //DEBUG
  float avgTemp = 0.0; float avgPress = 0.0;
  int i = 0;
  Serial.println("Processing Data"); //DEBUG
  while(i < 4)
  {
    flashLed();
    currentTime = millis();//DEBUG
    if((currentTime - lastRec) >= 250)
    {
      
      i++;
      lastRec = currentTime;
      avgTemp += bme.readTemperature();
      avgPress += bme.readPressure();
    }
  }
  Serial.println("averaged data for 1 sec"); // DEBUG
  //Serial1.println();
  avgTemp = avgTemp/4.0;
  avgPress = avgPress/4.0;
  outputData(currentTime, avgTemp, avgPress);
}

void outputData(unsigned long curTime, float temp, float pressure)
{
  int curAlt = bme.readAltitude(seaLvl);
  float timeSec = (curTime-startTime)/1000.0;
  
    if (Serial1.available() > 0)
  {
    if (gps.encode(Serial1.read()))
  double gpLat = gps.location.lat();
  double gpLong = gps.location.lng();
  }
  
 //Serial.printf("%i ",curAlt); //DEBUG
 //Serial.printf("%.2f\n", timeSec); //DEBUG
  Serial2.printf("%.2f, %i, %.2f, %.2f, %d, %d\n", timeSec, curAlt, temp, pressure/100.0, gpLat, gpLong); //Send to OpenLog //Pressure in millibars //Time in sec 
  Serial3.printf("%.2f,%i,%.2f,%.2f,%d,%d\n", timeSec, curAlt, temp, pressure/100.0, gpLat, gpLong); //Send to XBee //Gps coords in degrees //Temperature in Celcius
}



void descentFlight()
{
  Serial3.println("Descent Started"); //descent FLIGHT START DEBUG
  ledFreq = 500;
  lastGroundCheck = millis();
  bool hasLanded = false;
  Serial.println("LANDING BOOL ASSIGNED"); //DEBUG
  while(!hasLanded)
  {
    processData();
    //outputData will print Altitude
    hasLanded = landed(); // landing check
  }
  Serial.println("PASSED DESCENT LOOP"); //DEBUG
}



bool landed()
{
  //Serial3.println("Payload Landed"); //DEBUG
  unsigned long currentTime = millis();
  int curAlt;
  if((currentTime - lastGroundCheck) >= 3000) //check every 2 seconds to see if it has landed 
  {
    Serial.println("Checking if landed (2s)"); //DEBUG
    curAlt = bme.readAltitude(seaLvl);
    
    if((curAlt > lastAlt -1)&&(curAlt <lastAlt+1))
    {
     
      Serial3.println("Payload Landed"); //DEBUG
      return true;
    }
    
    lastGroundCheck = currentTime;
    lastAlt = curAlt;
    Serial.print("**Has not landed**\n"); //DEBUG
    return false;
  }
  return false;
}



void setup() {
  Serial.begin(9600); // DEBUG
  
  pinMode(ledPin, OUTPUT);
  pinMode(buzzer, OUTPUT);
  Serial1.begin(9600); //GPS
  Serial2.begin(9600); //begin serial port2 (for SD card)
  Serial3.begin(9600); //XBEE
  bme.begin();

  delay(2000); //WAIT BEFORE BEGINNING
  Serial.print("SeaLevel Set.\n"); // DEBUG
  seaLvl = bme.readPressure() / 100.0;
  
  
  myServo.attach(servoPin); //attach servo to pin #
  myServo.write(2); // Initial angle of the servo


  
  preFlight();
  ascentFlight();
  descentFlight();
  ledFreq = 100;
  digitalWrite(buzzer, HIGH); //DEBUG enable buzzer
  Serial.println("Flight Complete"); //DEBUG
}

//Post FLight Loop
void loop() {
  flashLed();
  pulseBuzzer();
}
