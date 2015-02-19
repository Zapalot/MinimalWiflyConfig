// Set up connection to Access Point and print incoming data.
// This Example is in the public domain.

// if you use a MEGA, you can hook up the Wifly to a harware-serial and comment the following lines out:
#include "SoftwareSerial.h"
SoftwareSerial softSerial(3,2); //The TX- and RX- Pins the Wifly is connected to.

#define wiflySerialType SoftwareSerial  // you can change this to either 'HardwareSerial' or 'SoftwareSerial' as you need it.
#define DEBUG_WIFI_RESPONSE 1 //shall we print all the communication with the wifly module to the debugSerial? [0/1]
#define DEBUG_WIFI_STATUS 1   //shall we show  wifly status information on the Serial? [0/1]

#include "MinimalWiflyConfig.h"
MinimalWiflyConfig wifly (softSerial, Serial);  // first Parameter is the serial used for communication with  the Wifly, second is for debugging

//if you use a MEGA and a harware-serial, use the following line instead. The first argument is the Serial the Wifly is connected to.
//MinimalWiflyConfig wifly (Serial3, Serial);  //

void setup()
{ 

  Serial.begin(115200);
  Serial.println(F("Starting Wifly Setup..."));
  
  // this setup function has to be run only once for a module, because it will remember the settings afterwards.
  // the commands used for configuration are fairly simple - they can be found in the datasheet of the module or in the source of the library.
  
  // The following protocols are supported by the Wifly 
  //  PROTO_UDP             // bidirectional UDP. Packets are sent after a timout. You want this for OSC
  //  PROTO_TCP_IN_OUT=2,   // accept incoming or make outgoing TCP connections 
  //  PROTO_TCP_SECURE=4,   // pseudo-secure TCP mode (see users manual for details)
  //  PROTO_TCP_OUT_ONLY=8, // only allow outgoing TCP connections
  //  PROTO_HTTP_CLIENT=16  // (see users manual for details)
	
  bool success =wifly.setup(
  19200,      // if you use a hardware serial, I would recommend the full 115200. 19200 is the maximum that works reliably with a software serial.
  true,	      // should we try to set the module to the specified baudrate if it doesn't answer when using the one specified above?
  "wally",    // Your Wifi Name (SSID)
  "robomint", // Your Wifi Password 
  "WiFly",    // Device name for identification in the network
  0,          // IP Adress of the Wifly. if 0 (without quotes), it will use dhcp to get an ip. Incoming packets must go to the IP adress shown during setup.
  8000,                   // WiFly receive port -  incoming packets have to be sent there
  "255.255.255.255",      // Where to send outgoing Osc messages. "255.255.255.255" will send to all hosts in the subnet
  8001,                   // outgoing port - outgoing packets will be sent there
  MinimalWiflyConfig::PROTO_UDP //the selected protocol (see above)
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



