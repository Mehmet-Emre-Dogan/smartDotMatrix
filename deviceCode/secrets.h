#ifndef _secrets_h_
#define _secrets_h_
/*############################################################################ Classified information ############################################################################*/
//
// Replace with your network credentials
const char* ssid = "yourRouter"; // Input your wifi network name
const char* password = "yourPasswd"; // Input your wifi password
//
/*############################################################################ Classified information ############################################################################*/
const char* deviceName = "Matrix";
WiFiServer server(80);// Set web server port number to 80
IPAddress staticIP(192, 168, 1, 102); //ESP static ip
IPAddress gateway(192, 168, 1, 1);   //IP Address of your WiFi Router (Gateway)
IPAddress subnet(255, 255, 255, 0);  //Subnet mask
IPAddress dns(8, 8, 8, 8);  //DNS
#endif