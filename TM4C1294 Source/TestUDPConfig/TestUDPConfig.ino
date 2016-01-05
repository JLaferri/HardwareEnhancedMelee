#include <Ethernet.h>
#include <EEPROM.h>
#include <EthernetUdp.h>
#include <ArduinoJson.h>

#include "Flash.h"

#define UDP_MAX_PACKET_SIZE 100
#define RECONNECT_TIME_MS 15000

#define MSG_TYPE_DISCOVERY 1
#define MSG_TYPE_FLASH_ERASE 2
#define MSG_TYPE_LOG_MESSAGE 3
#define MSG_TYPE_SET_TARGET 4

#define EEPROM_SCHEMA 0x1

//MAC address. This address will be overwritten by MAC configured in USERREG0 and USERREG1 during ethernet initialization
byte mac[] = { 0x00, 0x1A, 0xB6, 0x02, 0xF5, 0x8C };
//byte mac[] = { 0x00, 0x1A, 0xB6, 0x02, 0xFA, 0xF8 };

IPAddress serverIp(192, 168, 0, 3);
int serverPort = 3636;
long timeOfLastFailedConnection = 0;
EthernetClient client;

int udpPort = 3637;
IPAddress lastBroadcastIp(192, 168, 0, 4);
int lastBroadcastPort = 0;
EthernetUDP udp;

bool ethernetInitialized = false;

bool sendUdpDebugMessages = true;
bool sendSerialDebugMessages = true;

String ipPortToString(IPAddress ip, int port) {
  char ipAddressString[30];
  sprintf(ipAddressString, "%d.%d.%d.%d:%d", ip[0], ip[1], ip[2], ip[3], port);
  return String(ipAddressString);
}

String ipToString(IPAddress ip) {
  char ipAddressString[20];
  sprintf(ipAddressString, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  return String(ipAddressString);
}

IPAddress stringToIp(String ipString) {
  
}

String macToString() {
  char macString[20];
  sprintf(macString, "%02X-%02X-%02X-%02X-%02X-%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(macString);
}

void ethernetInitialize() {
  debugPrintln("Checking EEPROM Schema");
  
  int i = 0;
  byte schemaRead = EEPROM.read(i++);
  
  //Ensure that EEPROM has been written to with the proper schema before reading
  if (schemaRead == EEPROM_SCHEMA) {
    debugPrintln("Loading settings from EEPROM");
    
    //Load target IP address
    serverIp[0] = EEPROM.read(i++);
    serverIp[1] = EEPROM.read(i++);
    serverIp[2] = EEPROM.read(i++);
    serverIp[3] = EEPROM.read(i++);
    
    //Load port
    serverPort = EEPROM.read(i++) << 8 | EEPROM.read(i++);
    
    //Load debug flags
    byte flags = EEPROM.read(i++);
    sendUdpDebugMessages = flags & 0x1;
    sendSerialDebugMessages = flags & 0x2;
  }
  
  debugPrintln("Getting MAC Address from registers.");
  loadMacAddress(mac);
  debugPrintln("Read MAC from registers: " + macToString());
  
  debugPrintln("Attempting to obtain IP address from DHCP");
  
  // start the Ethernet connection:
  if (Ethernet.begin(mac)) {
    debugPrintln("Obtained IP address: " + ipToString(Ethernet.localIP()));
  } else {
    debugPrintln("Failed to configure Ethernet using DHCP");
  }
  
  if (udp.begin(udpPort)) {
    debugPrintln("Initialized UDP");
  } else {
    debugPrintln("Failed to initialize UDP");
  }
  
  ethernetInitialized = true;
}

void maintainClientConnection() {
  //If client is connected, nothing to do
  if (client.connected()) return;
  
  //If connection attempt was recently failed, don't attempt to connect
  if (timeOfLastFailedConnection != 0 && millis() - timeOfLastFailedConnection < RECONNECT_TIME_MS) return;
  
  //Attempt to connect to server
  debugPrintln("Attempting to connect to server...");
  
  if (client.connect(serverIp, serverPort)) {
    debugPrintln("Connection to server successful.");
  } else {
    timeOfLastFailedConnection = millis();
    debugPrintln("Connection to server failed."); 
  }
}

void listenForUdpPacket() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    if (packetSize > UDP_MAX_PACKET_SIZE) return;
  
    //Get IP and port of the UDP sender
    lastBroadcastIp = udp.remoteIP();
    lastBroadcastPort = udp.remotePort();
    debugPrintln("Received UDP packet from: " + ipPortToString(lastBroadcastIp, lastBroadcastPort));
    
    //Read UDP packet into buffer
    char udpPacketBuffer[UDP_MAX_PACKET_SIZE];
    udp.read(udpPacketBuffer, UDP_MAX_PACKET_SIZE);
    
    //Create JSON root from buffer
    StaticJsonBuffer<UDP_MAX_PACKET_SIZE> jsonReadBuffer;
    JsonObject& root = jsonReadBuffer.parseObject(udpPacketBuffer);
    
    //Parse json for command
    int command = root["type"];
    debugPrintln("UDP packet contains command: " + String(command));
    
    //Prepare to write response
    StaticJsonBuffer<2048> jsonWriteBuffer;
    JsonObject& resp = jsonWriteBuffer.createObject();
    
    //TODO: Test what happens when a UDP message is sent that doesn't contain a type node
    
    //Handle command received
    switch(command) {
      case MSG_TYPE_DISCOVERY:
        debugPrintln("Responding to discovery request. " + macToString());
        //Add command and mac elements to JSON
        resp["type"] = MSG_TYPE_DISCOVERY;
        resp["mac"] = macToString();
//        resp["targetIp"] = ipToString(serverIp);
//        resp["targetPort"] = serverPort;
//        resp["debugSerial"] = sendSerialDebugMessages;
//        resp["debugUdp"] = sendUdpDebugMessages;

        debugPrintln("Writing UDP message...");
        //Send udp packet back
        udp.beginPacket(lastBroadcastIp, lastBroadcastPort);
        char buffer[2048];
        resp.printTo(buffer, sizeof(buffer));
        udp.write(buffer);
        udp.endPacket();
        
        debugPrintln("Discovery message written.");
        
        break;
      case MSG_TYPE_SET_TARGET:
//        String targetIpString = root["targetIp"];
//        int targetPort = root["targetPort"];
//        sendSerialDebugMessages = root["debugSerial"];
//        sendUdpDebugMessages = root["debugUdp"];
//        
//        //If IP or port as changed, disconnect client
//        if (!ipPortToString(serverIp, serverPort).equals(targetIpString + String(":") + String(targetPort))) {
//          debugPrintln("Disconnecting from old client.");
//          client.stop();
//        }
        
        //TODO: Write new settings to EEPROM
        
        break;
      case MSG_TYPE_FLASH_ERASE:
        debugPrintln("Erasing flash.");
        delay(200);
        eraseFlash();
        debugPrintln("Flash should be erased and program should be restarted. This message should not show up.");
    }     
  }
}

//This is the function that should be called every loop of the application
int ethernetExecute() {
  listenForUdpPacket();
  maintainClientConnection();
  
  Ethernet.maintain();
  return 1;
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  
  ethernetInitialize();
}

void debugPrintln(String s) {
   debugPrint(s + String("\r\n"));
}

void debugPrint(String s) {
  if (sendSerialDebugMessages) {
    //Print serial message
    Serial.print(s);
  }
  
  if (sendUdpDebugMessages && ethernetInitialized) {
    //Prepare to write response
    int jsonBufSize = s.length() + 50;
    StaticJsonBuffer<512> jsonWriteBuffer;
    JsonObject& json = jsonWriteBuffer.createObject();
  
    //Populate response JSON
    json["type"] = MSG_TYPE_LOG_MESSAGE;
    json["message"] = s;
    
    //Write JSON to 
    udp.beginPacket(lastBroadcastIp, lastBroadcastPort);
    char buffer[jsonBufSize];
    json.printTo(buffer, sizeof(buffer));
    udp.write(buffer);
    udp.endPacket();
  }
}

void loop()
{
  // put your main code here, to run repeatedly:
  ethernetExecute();
  delay(200);
}
