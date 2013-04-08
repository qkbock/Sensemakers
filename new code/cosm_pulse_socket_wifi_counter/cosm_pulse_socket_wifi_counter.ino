 #include <SPI.h>
#include <Ethernet.h>
#include <HttpClient.h>
#include <Cosm.h>

//THIS IS FOR WIFI CONNECTION
#include <WiFlyHQ.h>
#include <SoftwareSerial.h>
//WIFI
//Setting the TX & TRX pins, in this case (8,9) 
SoftwareSerial wifiSerial(8,9);
WiFly wifly;

/* Change these to match your WiFi network */
const char mySSID[] = "SBG658060"; //"internetz"; 
const char myPassword[] = "SBG6580E58E60"; //"1nt3rn3tz"; 
char site[] = "api.cosm.com";

/*
>> Pulse Sensor Amped 1.1 <<
 This code is for Pulse Sensor Amped by Joel Murphy and Yury Gitman
 www.pulsesensor.com 
 >>> Pulse Sensor purple wire goes to Analog Pin 0 <<<
 Pulse Sensor sample aquisition and processing happens in the background via Timer 2 interrupt. 2mS sample rate.
 PWM on pins 3 and 11 will not work when using this code, because we are using Timer 2!
 The following variables are automatically updated:
 Signal :    int that holds the analog signal data straight from the sensor. updated every 2mS.
 IBI  :      int that holds the time interval between beats. 2mS resolution.
 BPM  :      int that holds the heart rate value, derived every beat, from averaging previous 10 IBI values.
 QS  :       boolean that is made true whenever Pulse is found and BPM is updated. User must reset.
 Pulse :     boolean that is true when a heartbeat is sensed then false in time with pin13 LED going out.
 
 This code is designed with output serial data to Processing sketch "PulseSensorAmped_Processing-xx"
 The Processing sketch is a simple data visualizer. 
 All the work to find the heartbeat and determine the heartrate happens in the code below.
 Pin 13 LED will blink with heartbeat.
 If you want to use pin 13 for something else, adjust the interrupt handler
 It will also fade an LED on pin fadePin with every beat. Put an LED and series resistor from fadePin to GND.
 Check here for detailed code walkthrough:
 http://pulsesensor.myshopify.com/pages/pulse-sensor-amped-arduino-v1dot1
 
 Code Version 02 by Joel Murphy & Yury Gitman  Fall 2012
 This update changes the HRV variable name to IBI, which stands for Inter-Beat Interval, for clarity.
 Switched the interrupt to Timer2.  500Hz sample rate, 2mS resolution IBI value.
 Fade LED pin moved to pin 5 (use of Timer2 disables PWM on pins 3 & 11).
 Tidied up inefficiencies since the last version. 
 */


//  VARIABLES
int pulsePin = 0;                 // Pulse Sensor purple wire connected to analog pin 0
int blinkPin = 13;                // pin to blink led at each beat
int fadePin = 5;                  // pin to do fancy classy fading blink at each beat
int fadeRate = 0;                 // used to fade LED on with PWM on fadePin
int numberOfBeats = 0;
int averageBPM = 0;
int averageNumber = 10;
long previousUploadTime = 0;
long uploadInterval = 100;
unsigned long lastMillis = 0;
unsigned long currentMillis = 0;
int counter = 0;

// these variables are volatile because they are used during the interrupt service routine!
volatile int BPM;                   // used to hold the pulse rate
volatile int Signal;                // holds the incoming raw data
volatile int IBI = 600;             // holds the time between beats, the Inter-Beat Interval
volatile boolean Pulse = false;     // true when pulse wave is high, false when it's low
volatile boolean QS = false;        // becomes true when Arduoino finds a beat.



// Your Cosm key to let you upload data
char feedId[] = "104810"; //FEED ID
char cosmKey[] = "ewTCG0qri8i6jXsXxwXxnrAZpnKSAKxHL0tnbndNeEpPdz0g"; //API KEY
char sensorId[] = "Quincy"; //This should not contain a space ' ' char

void setup() {
  // put your setup code here, to run once:
  pinMode(blinkPin,OUTPUT);         // pin that will blink to your heartbeat!
  pinMode(fadePin,OUTPUT);          // pin that will fade to your heartbeat!
  Serial.begin(115200);             // we agree to talk fast!
  

  Serial.println("Starting PULSE upload to Cosm...");
  Serial.println();
  
  char buf[32];
  
  Serial.println("Starting");
    Serial.print("Free memory: ");
    Serial.println(wifly.getFreeMemory(),DEC);

    wifiSerial.begin(9600);
    if (!wifly.begin(&wifiSerial, &Serial)) {
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
	} else {
	    Serial.println("Failed to join wifi network");
	    
	}
    } else {
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

	/* Send the request */
//	wifly.println("GET / HTTP/1.0");
//	wifly.println();
    } else {
        Serial.println("Failed to connect");
    }
    interruptSetup();                 // sets up to read Pulse Sensor signal every 2mS 
}

void loop() {
  currentMillis = millis();
  
  //sendDataToProcessing('S', Signal);     // send Processing the raw Pulse Sensor data
  if (QS == true){                       // Quantified Self flag is true when arduino finds a heartbeat
    fadeRate = 255;                  // Set 'fadeRate' Variable to 255 to fade LED with pulse
    Serial.println(BPM);
    sendDataToProcessing('B',BPM);   // send heart rate with a 'B' prefix
    //sendDataToProcessing('Q',IBI);   // send time between beats with a 'Q' prefix
    // sendDataToCosm(BPM);             //send data to cosm sockets using arduino ethernet/shield
    QS = false;                      // reset the Quantified Self flag for next time    
  }
  if(currentMillis - lastMillis >= 1000){
    lastMillis = millis();
    counter ++;
    sendDataToCosm(BPM);
    Serial.println("NOW!");
  }

  //ledFadeToBeat(); //This will fade the led to the beat if you have one plugged into a PWM pin (definded above)
  
  // from the server, read them and print them:
  
//  if (wifly.available() > 0) {
//	char ch = wifly.read();
//	Serial.write(ch);
//	if (ch == '\n') {
//	    /* add a carriage return */ 
//	    Serial.write('\r');
//	}
//    }

    if (Serial.available() > 0) {
	wifly.write(Serial.read());
    }

  delay(20);                             //  take a break
}

void ledFadeToBeat(){
  fadeRate -= 15;                         //  set LED fade value
  fadeRate = constrain(fadeRate,0,255);   //  keep LED fade value from going into negative numbers!
  analogWrite(fadePin,fadeRate);          //  fade LED
}


void sendDataToProcessing(char symbol, int data ){
  Serial.print(symbol);                // symbol prefix tells Processing what type of data is coming
  Serial.println(data);                // the data to send culminating in a carriage return
}

void sendDataToCosm(int counter, int data){
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


