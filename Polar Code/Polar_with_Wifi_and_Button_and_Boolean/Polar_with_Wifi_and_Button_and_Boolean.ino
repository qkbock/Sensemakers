#include <HttpClient.h>
#include <Cosm.h>
#include <WiFlyHQ.h>
#include <SoftwareSerial.h>
#include "Wire.h"
#define HRMI_I2C_ADDR      127
#define HRMI_HR_ALG        1

SoftwareSerial wifiSerial(8,9);
WiFly wifly;

const char mySSID[] =   "SBG658060"; // "internetz";
const char myPassword[] =  "SBG6580E58E60";  //"1nt3rn3tz";
char site[] = "api.cosm.com";
char feedId[] = "104810"; //FEED ID
char cosmKey[] = "ewTCG0qri8i6jXsXxwXxnrAZpnKSAKxHL0tnbndNeEpPdz0g";
char sensorId[] = "Quincy"; 

const int buttonPin = A2;
const int ledPin = 8;
int previousButtonState = HIGH; 
boolean sendData = false;
long currentMillis = 0;
long lastMillis = 0;

void setup() {
  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  Keyboard.begin();

  setupHeartMonitor(HRMI_HR_ALG);
  Serial.begin(115200);
  Serial.println("Starting PULSE upload to Cosm...");

  char buf[32];

  Serial.println("Starting");
  Serial.print("Free memory: ");
  Serial.println(wifly.getFreeMemory(),DEC);
  Serial1.begin(9600); //THIS IS ONLY AVAILABLE ON LEONARDO
  if (!wifly.begin(&Serial1, &Serial)) {
    Serial.println("Failed to start wifly");
  }

  if (!wifly.isAssociated()) {
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
  Serial.println("Set up Complete");

}




void loop() {
  currentMillis = millis();
  int buttonState = digitalRead(buttonPin);
  if ((buttonState != previousButtonState) 
    && (buttonState == HIGH)) {
    Keyboard.print(" ");
    sendData = !sendData;
  }
  previousButtonState = buttonState; 

  int heartRate = getHeartRate();

  if(!wifly.isConnected()){
    if (wifly.open(site, 8081)) {
      Serial.print("Connected to ");
      Serial.println(site);

    } 
    else {
      Serial.println("Failed to connect");
    }
  }

  if(sendData){
    if(currentMillis - lastMillis >= 1000){
      sendDataToCosm(heartRate);
      Serial.println(heartRate);
      lastMillis = millis();
    }
    digitalWrite(ledPin, HIGH);
  }
  else{
    digitalWrite(ledPin, LOW); 
  }


  delay(10);

}


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
  Wire.beginTransmission(deviceAddress); // start transmission to device 
  Wire.write(address);       // send register address
  Wire.write(val);         // send value to write
  Wire.endTransmission();     // end transmission
}

boolean hrmiGetData(byte addr, byte numBytes, byte* dataArray){
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




