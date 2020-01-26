/* LIBRARIES */
#include <SPI.h>
#include "Firebase_Arduino_WiFiNINA.h"

/*******************************/
/*                             */
/*          CONSTANTS          */
/*                             */
/*******************************/
// Arduino States
#define ARDUINO_LOADING 1
#define ARDUINO_WAITING_FOR_TX_DATA 2
#define ARDUINO_WAITING_FOR_CHECK_IN 3
#define ARDUINO_TX_STARTED 4
#define ARDUINO_TX_ENDED 5
#define ARDUINO_RX_WAITING 6
#define ARDUINO_RX_STARTING 7
#define ARDUINO_RX_STARTED 8
#define ARDUINO_RX_ENDED 9
#define ARDUINO_EXIT 10
// Android states
#define ANDROID_LOADING 11
#define ANDROID_WAITING_FOR_START 12
#define ANDROID_WAITING_FOR_CHECK_IN_TX 13
#define ANDROID_TX_STARTING 14
#define ANDROID_TX_STARTED 15
#define ANDROID_TX_ENDED 16
#define ANDROID_WAITING_FOR_CHECK_IN_RX 17
#define ANDROID_RX_STARTING 18
#define ANDROID_RX_STARTED 19
#define ANDROID_RX_ENDED 20
#define ANDROID_EXIT 21

String stateToString(int state) {
  switch (state) {
  // ARDUINO
    case ARDUINO_LOADING:
      return "LOADING";
    case ARDUINO_WAITING_FOR_TX_DATA:
      return "WAITING_FOR_TX_DATA";
    case ARDUINO_WAITING_FOR_CHECK_IN:
      return "WAITING_FOR_CHECK_IN";
    case ARDUINO_TX_STARTED:
      return "TX_STARTED";
    case ARDUINO_TX_ENDED:
      return "TX_ENDED";
    case ARDUINO_RX_WAITING:
      return "RX_WAITING";
    case ARDUINO_RX_STARTING:
      return "RX_STARTING";
    case ARDUINO_RX_STARTED:
      return "RX_STARTED";
    case ARDUINO_RX_ENDED:
      return "RX_ENDED";
    case ARDUINO_EXIT:
      return "EXIT";
    // ANDROID
    case ANDROID_LOADING:
      return "LOADING";
    case ANDROID_WAITING_FOR_START:
      return "WAITING_FOR_START";
    case ANDROID_WAITING_FOR_CHECK_IN_TX:
      return "WAITING_FOR_CHECK_IN_TX";
    case ANDROID_TX_STARTING:
      return "TX_STARTING";
    case ANDROID_TX_STARTED:
      return "TX_STARTED";
    case ANDROID_TX_ENDED:
      return "TX_ENDED";
    case ANDROID_WAITING_FOR_CHECK_IN_RX:
      return "WAITING_FOR_CHECK_IN_RX";
    case ANDROID_RX_STARTING:
      return "RX_STARTING";
    case ANDROID_RX_STARTED:
      return "RX_STARTED";
    case ANDROID_RX_ENDED:
      return "RX_ENDED";
    case ANDROID_EXIT:
      return "EXIT";  
  }
}

// Firebase
#define FIREBASE_HOST "mt-ibai-ku.firebaseio.com"                 // Database API URL
#define FIREBASE_AUTH "4XzhMRfz9SyhAXR2FATTVsI7cCCbuWKUTylmvY5S"  // Database secret
// WiFi Connection
#define WIFI_SSID "mt-ibai-ku" // Network SSID
#define WIFI_PASSWORD "p455w0rd"   // Network password
// PINs
#define LED_PIN 11
#define PHOTOCELL_PIN 0
// Delays
#define LUM_CALC_DELAY_MILLIS 200
#define BLINK_DELAY_MILLIS 500
#define INITIAL_BLINK_DELAY_MILLIS 5000
// Parameters
#define LUM_CALC_STEPS 5
#define LUM_THRESH 1.1

/*******************************/
/*                             */
/*          VARIABLES          */
/*                             */
/*******************************/
// Initial luminance average
int initialLumAverage;
// Start and end sequence
char startSeq = B11100111;
char endSeq = B11111111;
// Arduino State
int state;
// Tx data
String txData;
String txMode;
int txRate;

/*******************************/
/*                             */
/*        USER FUNCTIONS       */
/*                             */
/*******************************/
// Initial WiFi Setup
void wifiSetup() {
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    while (true);       // don't continue
  }

  int status = WL_IDLE_STATUS;
  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(WIFI_SSID);                   // print the network name (SSID);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    // wait 10 seconds for connection:
    delay(5000);
  }
  printWifiStatus();                        // you're connected now, so print out the status
}

// Printing WiFi information
void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

// Setting up Firebase
void firebaseSetup() {
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH, WIFI_SSID, WIFI_PASSWORD);
  Firebase.reconnectWiFi(true);
  FirebaseData fd;
  if (fd.httpConnected()) {
    Serial.println("Firebase connected");
  } else {
    Serial.println("Firebase NOT connected");
  }
  delay(1000);
}

// Firebase GET Method
String firebaseGetString(String path){
  FirebaseData firebaseData;
  if (Firebase.getString(firebaseData, path)) {
    return firebaseData.stringData();
  } else {
    Serial.println("----------Can't get data--------");
    Serial.println("REASON: " + firebaseData.errorReason());
    Serial.println("--------------------------------");
    Serial.println();
    return "";
  }
}

int firebaseGetInt(String path){
  FirebaseData firebaseData;
  if (Firebase.getInt(firebaseData, path)) {
    return firebaseData.intData();
  } else {
    Serial.println("----------Can't get data--------");
    Serial.println("REASON: " + firebaseData.errorReason());
    Serial.println("--------------------------------");
    Serial.println();
    return "";
  }
}

bool firebaseSetString(const String path, const String value) {
  FirebaseData firebaseData;
  Serial.print(path);
  Serial.print(" -> ");
  Serial.println(value);
  boolean b = Firebase.setString(firebaseData, path, value);
  Serial.println(b);

  if (b) {
    Serial.print("StringData: ");
    Serial.println(firebaseData.stringData());
    Serial.print("DataPath: ");
    Serial.println(firebaseData.dataPath());
    Serial.print("DataType: ");
    Serial.println(firebaseData.dataType());
    Serial.print("ErrorReason: ");
    Serial.println(firebaseData.errorReason());
    Serial.print("HTTP Code: ");
    Serial.println(firebaseData.httpCode());
    Serial.print("Payload: ");
    Serial.println(firebaseData.payload());
  } else {
    Serial.print("ErrorReason: ");
    Serial.println(firebaseData.errorReason());
  }

  return b;
}

int setState(int newState) {
  if (firebaseSetString("/variables/arduino/state", stateToString(newState))) {
    state = newState;
  }
  return state;
}

// Initial Luminance setup
void setupInitialLuminance() {
  int photocellReading = 0;
  for (int i = 0; i < LUM_CALC_STEPS; i++) {
    photocellReading += analogRead(PHOTOCELL_PIN);
    delay(LUM_CALC_DELAY_MILLIS);
  }
  initialLumAverage = photocellReading / LUM_CALC_STEPS;
  Serial.print("InitialLumAverage: ");
  Serial.println(initialLumAverage);
}

// Blink single charachter
void blink8bits(char messageToBlink) {
  for (int i = 0; i < 8; ++i) {
    digitalWrite(LED_PIN, (PinStatus) ((messageToBlink >> i) & 0x01));  // turn the LED on (HIGH is the voltage level)
    delay(BLINK_DELAY_MILLIS);                                                    // wait for BLINK_DELAY_MILLIS
  }
}

// Blink sequence of bytes
void blinkWholeSequence(char* message) {
  Serial.println("Blinking start sequence...");
  blink8bits(startSeq);

  Serial.println("Blinking message...");
  for (int i = 0; i < strlen(message); i++) {
    Serial.println(i);
    blink8bits(message[i]);
  }

  Serial.println("Blinking end sequence...");
  blink8bits(endSeq);
}

bool bitIsSet(int photocellReading) {
  return ((double) photocellReading) / initialLumAverage > LUM_THRESH;
}

// Detect LED Sequence
// TODO

/*******************************/
/*                             */
/*      SYSTEM FUNCTIONS       */
/*                             */
/*******************************/
// Setup function
void setup(void) {
  Serial.begin(115200); // Initialize serial
  delay(3000);

  Serial.println("********* SETUP Begin *********");

  pinMode(LED_PIN, OUTPUT); // Init LED as output
  pinMode(PHOTOCELL_PIN, INPUT); // Init photocell as input
  digitalWrite(LED_PIN, LOW); // Init LED to zero

  wifiSetup(); // Initialize WiFi connection
  firebaseSetup(); // Initialize Firebase database

  setState(ARDUINO_LOADING); // Initialize state

  Serial.println("********* SETUP End *********");

}
 
// Loop function
void loop(void) {

  switch (state) {
    case ARDUINO_LOADING:
      Serial.println("ARDUINO_LOADING");
      loading();
      break;
    case ARDUINO_WAITING_FOR_TX_DATA:
      Serial.println("ARDUINO_WAITING_FOR_TX_DATA");
      waiting_for_tx_data();
      break;
    case ARDUINO_WAITING_FOR_CHECK_IN:
      Serial.println("ARDUINO_WAITING_FOR_CHECK_IN");
      waiting_for_check_in();
      break;
    case ARDUINO_TX_STARTED:
      Serial.println("ARDUINO_TX_STARTED");
      startTx();
      break;
    case ARDUINO_TX_ENDED:
      Serial.println("ARDUINO_TX_ENDED");
      endTx();
      break;
    case ARDUINO_RX_STARTING:
      Serial.println("ARDUINO_RX_STARTING");
      startRx();
      break;
    case ARDUINO_RX_ENDED:
      Serial.println("ARDUINO_RX_ENDED");
      endRx();
      break;
    case ARDUINO_EXIT:
      Serial.println("ARDUINO_EXIT");
      break;
  }

}

/*******************************/
/*                             */
/*   STATE MACHINE FUNCTIONS   */
/*                             */
/*******************************/
void loading(void) {
  setState(ARDUINO_WAITING_FOR_TX_DATA);
}

void waiting_for_tx_data(void) {
  String androidState = firebaseGetString("/variables/android/state");
  if (androidState == stateToString(ANDROID_WAITING_FOR_CHECK_IN_TX) || androidState == stateToString(ANDROID_WAITING_FOR_CHECK_IN_RX)) {
    txMode = firebaseGetString("/variables/common/tx_mode");
    txData = firebaseGetString("/variables/common/tx_data");
    txRate = firebaseGetInt("/variables/common/tx_rate");
    if (txMode == "ARDUINO_ANDROID") {
      setState(ARDUINO_WAITING_FOR_CHECK_IN);
    } else if (txMode == "ANDROID_ARDUINO") {
      setupInitialLuminance();
      setState(ARDUINO_RX_STARTING);
    }
  }
}

void waiting_for_check_in(void) {
  String androidState = firebaseGetString("/variables/android/state");
  if (androidState == stateToString(ANDROID_RX_STARTING)) {
    setState(ARDUINO_TX_STARTED);
  }
}

void startTx(void) {
  char* utf8String;
  size_t size = sizeof(txData);
  txData.toCharArray(utf8String, size);
  blinkWholeSequence(utf8String);
  setState(ARDUINO_TX_ENDED);
}

void endTx(void) {
  setState(ARDUINO_EXIT);
}

void startRx(void) {
  unsigned long startTime;
  char* rxData;
  char tmpData = B00000000;
  int index = 0;
  int seqNum = 0;
  while (state != ARDUINO_RX_ENDED) {
    switch (state){
      case ARDUINO_RX_WAITING:
        int photocellReading = analogRead(PHOTOCELL_PIN);
        if (bitIsSet(photocellReading)) {
          startTime = millis();
          bitSet(tmpData, index);
          setState(ARDUINO_RX_STARTING);
          index++;
          seqNum++;
        }
        break;
      case ARDUINO_RX_STARTING:
        if (millis() - startTime > seqNum * (((double) 1000)/((double) txRate))) {
          int photocellReading = analogRead(PHOTOCELL_PIN);
          if (bitIsSet(photocellReading)) {
            bitSet(tmpData, index);
          }
          index++;
          seqNum++;
          if (index == 7) {
            if (tmpData == startSeq) {
              setState(ARDUINO_RX_STARTED);
            } else {
              setState(ARDUINO_RX_WAITING);
              seqNum = 0;
              Serial.println("UNKNOWN startSeq: " + tmpData);
            }
            tmpData = B00000000;
            index = 0;
          }
        }
        break;
      
      case ARDUINO_RX_STARTED:
        if (millis() - startTime > seqNum * (((double) 1000)/((double) txRate))) {
          int photocellReading = analogRead(PHOTOCELL_PIN);
          if (bitIsSet(photocellReading)) {
            bitSet(tmpData, index);
          }
          index++;
          seqNum++;
          if (index == 7) {
            if (tmpData == endSeq) {
              String s = "";
              for (int i = 0; i < sizeof(rxData); i++) {
                s += rxData[i];
              }
              firebaseSetString("/variables/arduino/tx_result", s);
              setState(ARDUINO_RX_ENDED);
            } else {
              rxData[(int) seqNum / 8] = tmpData;
            }
            tmpData = B00000000;
            index = 0;
          }
        }
        break;
    }
    delay(50);
  }
}

void endRx(void) {
  setState(ARDUINO_EXIT);
}
