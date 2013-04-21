//#include <SPI.h>
//#include <Ethernet.h>
#include <HttpClient.h>
#include <Cosm.h>
#include <WiFlyHQ.h>
#include <SoftwareSerial.h>

#include "Wire.h"

#define HRMI_I2C_ADDR      127
#define HRMI_HR_ALG        1

SoftwareSerial wifiSerial(8,9);
WiFly wifly;

/* Change these to match your WiFi network */
const char mySSID[] =   "SBG658060"; // "internetz";
const char myPassword[] =  "SBG6580E58E60";  //"1nt3rn3tz";
char site[] = "api.cosm.com";
// Your Cosm key to let you upload data
char feedId[] = "104810"; //FEED ID
char cosmKey[] = "ewTCG0qri8i6jXsXxwXxnrAZpnKSAKxHL0tnbndNeEpPdz0g"; //API KEY
char sensorId[] = "Quincy"; //This should not contain a space ' ' char

const int buttonPin = A2;
int previousButtonState = HIGH; 

//  VARIABLES
//int pulsePin = 0;                 // Pulse Sensor purple wire connected to analog pin 0
//int blinkPin = 13;                // pin to blink led at each beat
//int fadePin = 5;                  // pin to do fancy classy fading blink at each beat
//int fadeRate = 0;                 // used to fade LED on with PWM on fadePin
//int numberOfBeats = 0;
//int averageBPM = 0;
//int averageNumber = 10;
//long previousUploadTime = 0;
//long uploadInterval = 100;
//long lastMillis = 0;
//long currentMillis = 0;

// these variables are volatile because they are used during the interrupt service routine!
//volatile int BPM;                   // used to hold the pulse rate
//volatile int Signal;                // holds the incoming raw data
//volatile int IBI = 600;             // holds the time between beats, the Inter-Beat Interval
//volatile boolean Pulse = false;     // true when pulse wave is high, false when it's low
//volatile boolean QS = false;        // becomes true when Arduoino finds a beat.




void setup() {
  // make the pushButton pin an input:
  pinMode(buttonPin, INPUT);
  // initialize control over the keyboard:
  Keyboard.begin();
  
  setupHeartMonitor(HRMI_HR_ALG);
  Serial.begin(115200);
//  while(!Serial){ //ONLY USE THIS FOR DEBUG!!!
//    ;
//  }

  //  pinMode(blinkPin,OUTPUT);         // pin that will blink to your heartbeat!
  //  pinMode(fadePin,OUTPUT);          // pin that will fade to your heartbeat!
  //  Serial.begin(115200);             // we agree to talk fast!

  Serial.println("Starting PULSE upload to Cosm...");
  //  Serial.println();

  char buf[32];

  Serial.println("Starting");
  Serial.print("Free memory: ");
  Serial.println(wifly.getFreeMemory(),DEC);
  Serial1.begin(9600); //THIS IS ONLY AVAILABLE ON LEONARDO
  //wifiSerial.begin(9600);
  if (!wifly.begin(&Serial1, &Serial)) {
    Serial.println("Failed to start wifly");
  }

  /* Join wifi network if not already associated */
  if (!wifly.isAssociated()) {
    /* Setup the WiFly to connect to a wifi network */
    Serial.println("Joining network");
    wifly.setSSID(mySSID);
    wifly.setPassphrase(myPassword);
    wifly.enableDHCP();
    if (wifly.join()) {
      Serial.println("Joined wifi network");
    } 
    else {
      Serial.println("Failed to join wifi network");    
    }
  } 
  else {
    Serial.println("Already joined network");
  }

  Serial.print("MAC: ");
  Serial.println(wifly.getMAC(buf, sizeof(buf)));
  Serial.print("IP: ");
  Serial.println(wifly.getIP(buf, sizeof(buf)));
  Serial.print("Netmask: ");
  Serial.println(wifly.getNetmask(buf, sizeof(buf)));
  Serial.print("Gateway: ");
  Serial.println(wifly.getGateway(buf, sizeof(buf)));
  wifly.setDeviceID("Wifly-WebClient");
  Serial.print("DeviceID: ");
  Serial.println(wifly.getDeviceID(buf, sizeof(buf)));

  if (wifly.isConnected()) {
    Serial.println("Old connection active. Closing");
    wifly.close();
  }

  if (wifly.open(site, 8081)) {
    Serial.print("Connected to ");
    Serial.println(site);

  } 
  else {
    Serial.println("Failed to connect");
  }
  //  interruptSetupLEO();                 // sets up to read Pulse Sensor signal every 2mS 

  Serial.println("Set up Complete"); 
}




void loop() {
  //currentMillis = millis();
  //Serial.println(currentMillis);
  //Serial.println(millis());
  //    delay(20); 
  //  if (QS == true){                       // Quantified Self flag is true when arduino finds a heartbeat
  //    fadeRate = 255;                  // Set 'fadeRate' Variable to 255 to fade LED with pulse
  //    Serial.println(BPM);
  //sendDataToProcessing('B',BPM);   // send heart rate with a 'B' prefix

  //    QS = false;                           // reset the Quantified Self flag for next time    
  //  }
  //if 1 second has passed since the last data was sent
  //  if((currentMillis - lastMillis) >= 1000){
  //    //record the current time and set that to be the "last data sent time"
  //    lastMillis =  millis();
  //    sendDataToCosm(BPM);
  //    Serial.println("now!");
  //    Serial.println(BPM);
  //  }
  
  int buttonState = digitalRead(buttonPin);
  if ((buttonState != previousButtonState) 
  && (buttonState == HIGH)) {
    Keyboard.print(" ");
    Serial.print("button");
  }
  previousButtonState = buttonState; 
  
  int heartRate = getHeartRate();
  Serial.println(heartRate);

  if(!wifly.isConnected()){
    if (wifly.open(site, 8081)) {
      Serial.print("Connected to ");
      Serial.println(site);

    } 
    else {
      Serial.println("Failed to connect");
    }
  }
  sendDataToCosm(heartRate);
  delay(500);

  //  if (Serial.available() > 0) {
  //    wifly.write(Serial.read());
  //  }

  //  take a break
}




//void ledFadeToBeat(){
//  fadeRate -= 15;                         //  set LED fade value
//  fadeRate = constrain(fadeRate,0,255);   //  keep LED fade value from going into negative numbers!
//  analogWrite(fadePin,fadeRate);          //  fade LED
//}

//void sendDataToProcessing(char symbol, int data ){
//  Serial.print(symbol);                // symbol prefix tells Processing what type of data is coming
//  Serial.println(data);                // the data to send culminating in a carriage return
//}

void sendDataToCosm(int data){
  wifly.print("{\"method\" : \"put\",");
  wifly.print("\"resource\" : \"/feeds/");
  wifly.print(feedId);
  wifly.print("\", \"headers\" :{\"X-ApiKey\" : \"");
  wifly.print(cosmKey);
  wifly.print("\"},\"body\" :{ \"version\" : \"1.0.0\",\"datastreams\" : [{\"id\" : \"");
  wifly.print(sensorId);
  wifly.print("\",\"current_value\" : \"");
  wifly.print(data);
  wifly.print("\"}]}}");
  wifly.println();
}



void setupHeartMonitor(int type){
  //setup the heartrate monitor
  Wire.begin();
  writeRegister(HRMI_I2C_ADDR, 0x53, type); // Configure the HRMI with the requested algorithm mode
}

int getHeartRate(){
  //get and return heart rate
  //returns 0 if we couldnt get the heart rate
  byte i2cRspArray[3]; // I2C response array
  i2cRspArray[2] = 0;

  writeRegister(HRMI_I2C_ADDR,  0x47, 0x1); // Request a set of heart rate values 

  if (hrmiGetData(127, 3, i2cRspArray)) {
    return i2cRspArray[2];
  }
  else{
    return 0;
  }
}

void writeRegister(int deviceAddress, byte address, byte val) {
  //I2C command to send data to a specific address on the device
  Wire.beginTransmission(deviceAddress); // start transmission to device 
  Wire.write(address);       // send register address
  Wire.write(val);         // send value to write
  Wire.endTransmission();     // end transmission
}

boolean hrmiGetData(byte addr, byte numBytes, byte* dataArray){
  //Get data from heart rate monitor and fill dataArray byte with responce
  //Returns true if it was able to get it, false if not
  Wire.requestFrom(addr, numBytes);
  if (Wire.available()) {

    for (int i=0; i<numBytes; i++){
      dataArray[i] = Wire.read();
    }

    return true;
  }
  else{
    return false;
  }
}



