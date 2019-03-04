// Notes:
//  Certificate must be added to WiFi module in order to connect to Firebase!
//  ie: myproject-12345.firebaseio.com:443
//
// TMP36 Pin Variables
//  The analog pin the TMP36's Vout (sense) pin is connected to
//  the resolution is 10 mV / degree centigrade with a
//  500 mV offset to allow for negative temperatures

#include <SPI.h>
#include <WiFiNINA.h>

#include "arduino_secrets.h"
// Please enter your sensitive data in the added file/tab arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;        // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                 // your network key Index number (needed only for WEP)

const char fireBaseServer[] = FIREBASE_SERVER;                          // Path to the Firebase application
const String(elementPath) = FIREBASE_ELEMENT;                           // Sub path to the Firebase data element we want to update

// Initialize the Wifi client library
int status = WL_IDLE_STATUS;
WiFiSSLClient client;

String elementValue = "";                                               // Initialize the value of the variable used to update the data element

int sensorPin = A0;
int tempCalibration = -4;                                               // temperature calibration value 
float prevVoltage = 0;                                                  // last voltage detected during sensor read
int prevTempF = 0;                                                      // last temperature value sent to the database

// -------------------------------------------
// Method to print the wifi status information
// -------------------------------------------
void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

// -----------------------------------------------------
// Method makes a HTTP connection to the Firebase server
// -----------------------------------------------------
void httpRequest() {
  // Close any connection before send a new request.
  // This will free the socket on the Nina module
  client.stop();

  //Serial.print("Trying to Connect to Firebase server: ");
  //Serial.println(fireBaseServer);

  if (client.connectSSL(fireBaseServer, 443)) {

    //Serial.println("Connected to Firebase server");

    // Make the HTTP request to the Firebase server
    String putCommand = "PUT " + elementPath + " HTTP/1.1";
    client.println(putCommand);
    client.print("Host: ");
    client.println(fireBaseServer);
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(elementValue.length());
    client.println("Connection: close");
    client.println();
    client.print(elementValue);

    // note the time that the connection was made:
    // lastConnectionTime = millis();
  } else {
    // if you couldn't make a connection:
    //Serial.println("Connection to Firebase server failed");
  }
}

// ------------
// Setup method
// ------------
void setup() {

  // Initialize serial and wait for port to open (Needed for native USB port only)
  Serial.begin(9600);
  while (!Serial) {
    // waiting for serial port to connect
  }

  // Check for the WiFi module
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < "1.2.1") {
    Serial.println("Please upgrade the firmware");
  }

  // Attempt to connect to Wifi network
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection
    delay(10000);
  }

  // Successfully connected, print out the status
  printWifiStatus();
}

// ------------
// Loop method
// ------------
void loop() {

  // Read the temperature sensor 60 times in 1 second intervals to smooth the data variances
  int totalValue = 0;
  for (int i = 0; i <= 60; i++) {
    int readValue = analogRead(sensorPin);
    totalValue += readValue;
    delay(1000);
  }

  // Get the average sensor value and convert the reading value to voltage
  float voltage = (totalValue/60) * 5.0;
  voltage /= 1024.0;

  // Process the data if the voltage has changed since the last time it was processed
  if (voltage != prevVoltage) {
    // Calculate the temperature in centigrade converting from 10 mv per degree with 500 mV offset
    // to degrees ((voltage - 500mV) times 100)
    float temperatureC = (voltage - 0.5) * 100 ;

    // Convert from Centigrade to Fahrenheit
    float temperatureF = (temperatureC * 1.8) + 32.0;

    // Convert temperatureF to an integer and round up if necessary
    int intTempF = temperatureF;
    float decimalTemp = temperatureF - intTempF;
    if (decimalTemp > .49) {
      intTempF += 1;
    }

    // Adjust intTempF by the determined calibration value
    intTempF += tempCalibration;

    // Only send the temperature data if it has changed
    if (intTempF != prevTempF) {
      // If there's incoming data from the net connection. Then
      // send it out the serial port. This is for debugging purposes only:
      //while (client.available()) {
      //char c = client.read();
      //Serial.write(c);
      //}

      // Populate the elementValue by converting intTempF to a string
      elementValue = String(intTempF);

      // Connect and send the data to Firebase
      httpRequest();
      prevTempF = intTempF;
    }
    prevVoltage = voltage;
  }
}
