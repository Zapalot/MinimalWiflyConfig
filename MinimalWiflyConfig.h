#pragma once
#include "Arduino.h"

//debug output that can be disabled but is still checked at compile time.
#define debugSerialType Stream          // you can change this to either 'HardwareSerial' or 'SoftwareSerial' as you need it
#define TRACE_WIFLY_S(x) do { if (DEBUG_WIFI_STATUS) debugSerial.print( x); } while (0)
#define TRACELN_WIFLY_S(x) do { if (DEBUG_WIFI_STATUS) debugSerial.println( x); } while (0)

#define TRACE_WIFLY_R(x) do { if (DEBUG_WIFI_RESPONSE) debugSerial.print( x); } while (0)
#define TRACELN_WIFLY_R(x) do { if (DEBUG_WIFI_RESPONSE) debugSerial.println( x); } while (0)

class MinimalWiflyConfig{
public:
  wiflySerialType &wiflySerial;   ///< we use this to talk with the Wifly
  debugSerialType &debugSerial;   ///< we use this one for debug output

  unsigned long initialTimeoutMillis;       ///< time to wait for a command response to begin
  unsigned long interCharTimeoutMillis;     ///< time to wait for additional characters once a response has started to come in
  // protocol bits according to datasheet. can be combined (i.e. accept both TCP and UDP == 3)
  enum protocolType{
    PROTO_UDP=1,
    PROTO_TCP_IN_OUT=2,
    PROTO_TCP_SECURE=4, 
    PROTO_TCP_OUT_ONLY=8,
    PROTO_HTTP_CLIENT=16
  };

  MinimalWiflyConfig(wiflySerialType &wiflySerial,debugSerialType &debugSerial); /// you can supply two Serials, one for WiFly control, one for showing debug output.
  //simplified setup for a typical connection. returns true if succelssful
  bool setup(
  const long wiflySerialSpeed,
  const bool setBaudRate,	  ///< should we change the baudrate if the module isn't set up for the one specified (WARNING! - this can lock you out if you choose one that is too high!)
  const char* SSID,
  const char* password,
  const char* deviceID,           ///< for identifacation in the network
  const char* localIP,            ///< a string with numbers, if 0, we will use dhcp to get an ip
  const int   localPort,
  const char* remoteHost,
  const long  remotePort,
  const unsigned int protocol          ///< TCP or UDP?
  );
  void showStatus();              ///< prints information about connection, adresses etc. to the debug serial
  
  void exitCmdMode();                  ///< try to exit cmd mode of wifly (will send data over the network if not currently in cmd mode)
  boolean enterCmdMode();                  ///< try to enter cmd mode of wifly (takes time)
  
  void changeBaudRateTo(unsigned long newRate); ///< issue "change baud rate" commands at all known rates. (brute force...)

  // a bunch of convenience functions for sending commands and receiving replies
  void waitForChar(Stream &in, unsigned long timeout);    ///< waits at most timeout millis for another char to arrive.
  boolean checkForResponse(const __FlashStringHelper *resp); ///< wait for a response and check if it matches the expected one

  void readResponse(bool showResponse);   ///< get data from wiflySerial and (optionally) print on debugSerial.
  void readResponse(Stream &in, Stream &out,bool showResponse); ///< read and (optionally) pipe data from one serial to another

  void sendCmdWaitAndRelay(const __FlashStringHelper *cmd,bool showResponse);
  void sendCmdWaitAndRelay(const __FlashStringHelper *cmd, const char* param,bool showResponse); //
  void sendCmdWaitAndRelay(const __FlashStringHelper *cmd, long param,bool showResponse);
  void sendCmdWaitAndRelay(const __FlashStringHelper *cmd, int param,bool showResponse);
};

MinimalWiflyConfig::MinimalWiflyConfig(wiflySerialType &wiflySerial,debugSerialType &debugSerial):
wiflySerial(wiflySerial),
debugSerial(debugSerial),
initialTimeoutMillis(1500),
interCharTimeoutMillis(10)
{
}
////////////////////////////////
bool MinimalWiflyConfig::setup(
const long wiflySerialSpeed,
const bool setBaudRate,	///< should we change the baudrate if the module isn't set up for the one specified (WARNING! - this can lock you out if you choose one that is too high!)
const char* SSID,
const char* password,
const char* deviceID,      ///< for identifacation in the network
const char* localIP,       ///< a string with numbers, if 0, we will use dhcp to get an ip
const int   localPort,
const char* remoteHost,
const long  remotePort,
const unsigned int protocol  ///TCP or UDP?
){
  // try to connect with given baudrate
  TRACE_WIFLY_S(F("Trying to connect to WiFly with "));
  TRACELN_WIFLY_S(wiflySerialSpeed); 
  wiflySerial.begin(wiflySerialSpeed); // initialize serial to wifly 
  exitCmdMode(); //issue "exit cmd mode" command first to make sure we can enter it again...
  
  if (!enterCmdMode()){ //try to enter CMD-mode... if it fails:
    TRACELN_WIFLY_S(F("Connection failed."));
	if(setBaudRate){
		TRACELN_WIFLY_S(F("------Trying to set Baudrate...---------"));
		changeBaudRateTo(wiflySerialSpeed); // try to change baud rate - continue even if it didn't work
		TRACELN_WIFLY_S(F("------finished brute force approach to setting Baudrate...---------"));
		// initialize serial to wifly 
		wiflySerial.begin(wiflySerialSpeed);
		exitCmdMode(); //issue "exit cmd mode" command first to make sure we can enter it again...
		if (!enterCmdMode())return false; // try to enter command mode - if no "CMD" reply comes from the Wifly, something has gone wrong and we stop.
	}else return false; // setting CMD mode has failed, and we were told to not change the rate - give up...
  }

  // now lets set that thing up...
  // first we turn off all kinds of status messages that condfuse our response matching- this might have to be done twice...
  sendCmdWaitAndRelay(F("set u m 1"),DEBUG_WIFI_RESPONSE);           // turn off echo  
  sendCmdWaitAndRelay(F("set sys printlvl 0"),DEBUG_WIFI_RESPONSE);  // turn off debug output of the module
  sendCmdWaitAndRelay(F("set comm remote 0"),DEBUG_WIFI_RESPONSE);   // dont send a hello message to a client that has opened a tcp connection

  
  // set ip or dhcp.
  if(localIP==0){
    //set dhcp = on
    sendCmdWaitAndRelay(F("set ip dhcp 1"),DEBUG_WIFI_RESPONSE); // enable dhcp  
  }
  else{
    sendCmdWaitAndRelay(F("set ip dhcp 0"),DEBUG_WIFI_RESPONSE); // disable dhcp
    sendCmdWaitAndRelay(F("set ip a "),localIP,DEBUG_WIFI_RESPONSE); // set adress
  }
  
  sendCmdWaitAndRelay(F("set ip protocol "),(int)protocol,DEBUG_WIFI_RESPONSE);   //set protocol (must be one or a sum of constants in protocolType)
  sendCmdWaitAndRelay(F("set ip localport "),localPort,DEBUG_WIFI_RESPONSE); //set incoming port
  sendCmdWaitAndRelay(F("set ip host "),remoteHost,DEBUG_WIFI_RESPONSE);     //set outgoing target ip
  sendCmdWaitAndRelay(F("set ip remote "),remotePort,DEBUG_WIFI_RESPONSE);   //set outgoing port
  sendCmdWaitAndRelay(F("set wlan auth 3"),DEBUG_WIFI_RESPONSE); 		     //set security to wpa [2]
  sendCmdWaitAndRelay(F("set wlan ssid "),SSID,DEBUG_WIFI_RESPONSE);         //set WIfi Name (SSID)
  sendCmdWaitAndRelay(F("set wlan p "),password,DEBUG_WIFI_RESPONSE);   //set Wifi Pass 
  sendCmdWaitAndRelay(F("set comm time "),1,DEBUG_WIFI_RESPONSE);            //set packet flush timer (increase if you get fragmented outgoingpackets
  sendCmdWaitAndRelay(F("set wlan join 1"),DEBUG_WIFI_RESPONSE);             //enable autojoin
  sendCmdWaitAndRelay(F("save"),DEBUG_WIFI_RESPONSE);                        //save settings
  sendCmdWaitAndRelay(F("reboot"),DEBUG_WIFI_RESPONSE);                      //reboot to apply changes
  return true;
}

// prints information about connection, adresses etc. to the debug serial
void MinimalWiflyConfig::showStatus(){
  enterCmdMode();
  sendCmdWaitAndRelay(F("show net"),true); //ask for status and force show reply
  sendCmdWaitAndRelay(F("get mac"),true); //ask for status and force show reply
  sendCmdWaitAndRelay(F("get ip"),true); //ask for status and force show reply
  sendCmdWaitAndRelay(F("exit"),DEBUG_WIFI_RESPONSE); //ask for status and force show reply
}       
/////////////////////////////////
void MinimalWiflyConfig::sendCmdWaitAndRelay(const __FlashStringHelper *cmd,bool showResponse){
  TRACE_WIFLY_R(F("->\n"));
  TRACELN_WIFLY_R(cmd); //debug out
  wiflySerial.print(cmd);
  wiflySerial.print("\r");
  readResponse(showResponse); // if response printing is disabled, we still read incoming data to clear it from the buffer and avoid confusion 
}

void MinimalWiflyConfig::sendCmdWaitAndRelay(const __FlashStringHelper *cmd, const char* param,bool showResponse){
  TRACE_WIFLY_R(F("->\n"));
  TRACE_WIFLY_R(cmd); //debug out
  TRACELN_WIFLY_R(param); //debug out 
  wiflySerial.print(cmd);
  wiflySerial.print(param);
  wiflySerial.print(F("\r"));    
  readResponse(showResponse); // if response printing is disabled, we still read incoming data to clear it from the buffer and avoid confusion 
}

void MinimalWiflyConfig::sendCmdWaitAndRelay(const __FlashStringHelper *cmd, long param,bool showResponse){
  TRACE_WIFLY_R(F("->\n"));
  TRACE_WIFLY_R(cmd); //debug out
  TRACELN_WIFLY_R(param); //debug out 
  wiflySerial.print(cmd);
  wiflySerial.print(param);
  wiflySerial.print(F("\r"));  
  readResponse(showResponse); // if response printing is disabled, we still read incoming data to clear it from the buffer and avoid confusion 
}
void MinimalWiflyConfig::sendCmdWaitAndRelay(const __FlashStringHelper *cmd, int param,bool showResponse){
  sendCmdWaitAndRelay(cmd,(long) param, showResponse);
}
boolean  MinimalWiflyConfig::enterCmdMode(){
  // try to set command mode
  TRACELN_WIFLY_S(F("Trying to set Wifly to Command Mode. Answer should be 'CMD'"));
  delay(260);
  wiflySerial.print(F("$$$"));
  delay(260);
  
  //the wifly might answer with all kind of garbage before replyinf with "CMD". So we parse data until we find it...
  TRACE_WIFLY_R(F("<-\n"));
  waitForChar(wiflySerial,1000 ); // wait at most 1s for anything at all to come.
  int CMDpos=0; //which position of "CMD" are we expecting next?
  while(wiflySerial.available()){
    char inChar=wiflySerial.read();
	TRACE_WIFLY_R(inChar);
	//check for the letter we expect next. This is a simple "acceptor" state machine...
	switch(CMDpos){
	case 0:
		if(inChar=='C')CMDpos=1; //first letter found, now look for the 'M'
		break;
	case 1:
		if(inChar=='M')CMDpos=2;else CMDpos=0;//if second letter was found, look for the 'D', else start from beginning
		break;
	case 2:
		if(inChar=='D')return true;else CMDpos=0;//if third  letter was found,return "success", else start from beginning
		break;
	}
    waitForChar(wiflySerial,1000 ); // wait at most 1000ms for the next char.
  }
  
  return false; // no CMD was found within timeout - so settung command mode must have failed.
}

// try to exit cmd mode of wifly (will send data over the network if not currently in cmd mode)
void MinimalWiflyConfig::exitCmdMode()                 
{
 sendCmdWaitAndRelay(F(""),DEBUG_WIFI_RESPONSE);     //terminate last line
 sendCmdWaitAndRelay(F("exit"),DEBUG_WIFI_RESPONSE); //issue "exit cmd mode" command  
}

//wait at most timeout millis for another char to arrive.
void MinimalWiflyConfig::waitForChar(Stream &in, unsigned long timeout){
  unsigned long startMillis=millis();
  while(in.available()==0&&((millis()-startMillis)<timeout)){
    //__asm__("nop\n\t"); //an empty loop tends to be optimized away bz the compiler...
    delay(1);
  }
}

// get data from wiflySerial and (optionally) print on debugSerial.
void MinimalWiflyConfig::readResponse(bool showResponse){
  readResponse(wiflySerial,debugSerial, showResponse);
}
// get data from wiflySerial and (optionally) print on debugSerial.
void MinimalWiflyConfig::readResponse(Stream &in, Stream &out,bool showResponse){
  TRACE_WIFLY_R(F("<-\n"));
  waitForChar(in,initialTimeoutMillis ); // wait at most initialTimeoutMillis for anything at all to come.
  while(in.available()){
    // stream from in to out
    if(showResponse){
      out.write(in.read());
    }
    else{
      in.read(); //read anyway to clear buffer from response
    }
    waitForChar(in,interCharTimeoutMillis ); // wait at most interCharTimeoutMillis for the next char.
  }
}

// wait for a response and check if it matches the expected one
boolean MinimalWiflyConfig::checkForResponse(const __FlashStringHelper *cmdString){
  TRACE_WIFLY_S(F("Looking for:"));
  TRACELN_WIFLY_S(cmdString); 

  int cmdStringLength=strlen_P((const char PROGMEM *)cmdString);  
  const int bufLength=64;

  //receive data...
  waitForChar(wiflySerial,initialTimeoutMillis ); // wait at most initialTimeoutMillis for anything at all to come.
  char input[bufLength];
  wiflySerial.setTimeout(interCharTimeoutMillis);
  int nBytesReceived=wiflySerial.readBytes(input, bufLength-2); //read data...
  input[nBytesReceived]=0; // terminate string to make printing possible
  TRACE_WIFLY_R(F("<-\n")); //debug out: all that follows was received from the wifly
  TRACE_WIFLY_R(input);  
  if(nBytesReceived<cmdStringLength) return false;
  int foundPos=strncmp_P(input,(const char PROGMEM *)cmdString,cmdStringLength); // check for differences
  if(foundPos==0){
    TRACELN_WIFLY_S(F("found!"));
    return true; // no deviations found - success!
  }
  else {
    return false;
  }
} 

// try all known baudrates and issue a "change to" command.
// we do not care about what we receive because Software serial reception doesn't work at high baudrate (sending does).
void MinimalWiflyConfig::changeBaudRateTo(unsigned long newRate){
  //we try one baudrate after another and see if we managed to enter command mode.
  unsigned long serialSpeeds[]={9600, 19200, 28800,38400, 57600, 115200};
  int nSpeeds=6;
  for(int i=0;i<nSpeeds;i++){
    TRACE_WIFLY_S("Trying to connect with baudrate: ");
    TRACELN_WIFLY_S(serialSpeeds[i]);
    wiflySerial.begin(serialSpeeds[i]);
	exitCmdMode(); // to make sure we can enter again...
    enterCmdMode(); //try to enter cmd mode
	sendCmdWaitAndRelay(F("set uart baud "),(long)newRate,(bool)DEBUG_WIFI_RESPONSE); // set speed
	sendCmdWaitAndRelay(F("save"),(bool)DEBUG_WIFI_RESPONSE);                       //reboot to apply changes
	sendCmdWaitAndRelay(F("reboot"),(bool)DEBUG_WIFI_RESPONSE);                       //reboot to apply changes
	delay(300); // wait for the reboot to be finished
  }
}
