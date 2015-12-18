#include <Ethernet.h>
#include <EthernetUdp.h>
#include <ArduinoJson.h>

#define UDP_MAX_PACKET_SIZE 100
#define RECONNECT_TIME_MS 15000

byte mac[] = { 0x00, 0x1A, 0xB6, 0x02, 0xF5, 0x8C };

IPAddress server(192, 168, 0, 3);
int port = 3636;
long timeOfLastFailedConnection = 0;
EthernetClient client;

IPAddress udpServer(192, 168, 0, 3);
int udpPort = 3637;
IPAddress lastBroadcastIp(192, 168, 0, 4);
int lastBroadcastPort = 0;
EthernetUDP udp;

bool ethernetInitialized = false;

void ethernetInitialize() {
  debugPrintln("Attempting to obtain IP address from DHCP");
  
  // start the Ethernet connection:
  if (Ethernet.begin(mac)) {
    debugPrint("Obtained IP address: "); debugPrintln(String(Ethernet.localIP()));
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
  
  if (client.connect(server, port)) {
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
    char ipAddressString[30];
    sprintf(ipAddressString, "%d.%d.%d.%d:%d", lastBroadcastIp[0], lastBroadcastIp[1], lastBroadcastIp[2], lastBroadcastIp[3], lastBroadcastPort);
    debugPrintln("Received UDP packet from: " + String(ipAddressString));
    
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
    StaticJsonBuffer<200> jsonWriteBuffer;
    JsonObject& resp = jsonWriteBuffer.createObject();
        
    //Handle command received
    switch(command) {
      case 1:
        //Add command and mac elements to JSON
        resp["type"] = 1;
        char macString[20];
        sprintf(macString, "%02X-%02X-%02X-%02X-%02X-%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        resp["mac"] = macString;

        //Send udp packet back
        udp.beginPacket(lastBroadcastIp, lastBroadcastPort);
        char buffer[256];
        resp.printTo(buffer, sizeof(buffer));
        udp.write(buffer);
        udp.endPacket();
        
        break;
    }     
  }
}

//This is the function that should be called every loop of the application
int ethernetCheckConnection() {
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
  //Print serial message
  Serial.print(s);
   
  if (ethernetInitialized) {
    //Prepare to write response
    int jsonBufSize = s.length() + 50;
    StaticJsonBuffer<512> jsonWriteBuffer;
    JsonObject& json = jsonWriteBuffer.createObject();
  
    json["type"] = 3;
    json["message"] = s;
    
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
  ethernetCheckConnection();
  delay(200);
}
