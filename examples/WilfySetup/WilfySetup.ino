// Set up connection to Access Point and print incoming data.
// This Example is in the public domain.

#define wiflySerialType SoftwareSerial  // you can change this to either 'HardwareSerial' or 'SoftwareSerial' as you need it
#define debugSerialType Stream          // you can change this to either 'HardwareSerial' or 'SoftwareSerial' as you need it
#define DEBUG_WIFI_RESPONSE 0 //shall we print all the communication with the wifly module to the debugSerial? [0/1]
#define DEBUG_WIFI_STATUS 1   //shall we show status wifly information on the Serial? [0/1]

#include "MinimalWiflyConfig.h"

#include "SoftwareSerial.h"
SoftwareSerial softSerial(3,2);
MinimalWiflyConfig wifly (softSerial, Serial);  // first Parameter is the serial used for communication with  the Wifly, second is for debugging
void setup()
{ 

  Serial.begin(115200);
  Serial.println(F("Starting Wifly Setup..."));
  
  // this setup function has to be run only once for a module, because it will remember the settings afterwards.
  // the commands used for configuration are fairly simple - they can be found in the datasheet of the module or in the souce of the library.
  bool success =wifly.setup(
  19200,      // if you use a hardware serial, I would recommend the full 115200. 19200 is the maximum that works reliably with a software serial.
  true,	      // should we set the Module to the specified baudrate if it is currently configured othewise
  "wally",    // Your Wifi Name (SSID)
  "robomint", // Your Wifi Password 
  "WiFly",    // Device name for identification in the network
  0,          // IP Adress of the Wifly. if 0 (without quotes), it will use dhcp to get an ip. Incoming packets must go to the IP adress shown during setup.
  8000,                   // WiFly receive port -  incoming packets have to be sent there
  "255.255.255.255",      // Where to send outgoing Osc messages. "255.255.255.255" will send to all hosts in the subnet
  8001,                   // outgoing port - outgoing packets will be sent there
  MinimalWiflyConfig::PROTO_UDP //protocol bit (see datasheet or source)
  );
  Serial.print(F("Wifly setup finished - successful (0=no, 1=yes): "));
  Serial.println(success);
}


void loop()
{
  // you will notice that the module needs a few seconds until connecting and getting an IP from DHCP after beeing configured.
  // If you skip the configuration, it's gonna be up within a few hundred milliseconds after power on
  Serial.println(F("---------Network Status---------"));
  wifly.showStatus();
  delay(500);
}



