#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <ArduinoJson.h>

#include "SSI3DMASlave.h"
#include "enhmelee.h"
#include "Flash.h"
#include "serverConfig.h"

//**********************************************************************
//*                         ASM Event Codes
//**********************************************************************
#define EVENT_GAME_START 0x37
#define EVENT_UPDATE 0x38
#define EVENT_GAME_END 0x39

int asmEvents[256];

void asmEventsInitialize() {
  asmEvents[EVENT_GAME_START] = 0xA;
  asmEvents[EVENT_UPDATE] = 0x7A;
  asmEvents[EVENT_GAME_END] = 0x1;
}

//**********************************************************************
//*                          Global Variables
//**********************************************************************
bool connectAttempted = false;
char debugStrBuf[1024];

//**********************************************************************
//*               SPI Slave Communication Functions
//**********************************************************************
RfifoMessage Msg; //Keep an RfifoMessage variable as a global variable to prevent memory leak?

void spiSlaveInitialize() {
  SSI3DMASlave.begin();
}

void spiReadMessage() {
  Msg = { false, 0, 0, 0, 0 };
  
  //Wait until a message is available
  if(!SSI3DMASlave.isMessageAvailable()) return;
  
  uint32_t messageSize = SSI3DMASlave.getMessageSize();
  uint8_t* bytes = SSI3DMASlave.popMessage();
  
  Msg.eventCode = bytes[0];
  
  //Copy all bytes from receive buffer to msg
  for (int i = 1; i < messageSize; i++) Msg.data[i-1] = bytes[i];
  
  Msg.messageSize = messageSize - 1;
  
  //If message size does not match expected size, return without flagging success
  if (Msg.messageSize != asmEvents[Msg.eventCode]) return;
  
  Msg.success = true; 
}

//**********************************************************************
//*                         Event Handlers
//**********************************************************************
Game CurrentGame = { };
bool gameInProgress = false;

//The read operators will read a value and increment the index so the next read will read in the correct location
uint8_t readByte(uint8_t* a, int& idx) {
  return a[idx++];
}

uint16_t readHalf(uint8_t* a, int& idx) {
  uint16_t value = a[idx] << 8 | a[idx + 1];
  idx += 2;
  return value;
}

uint32_t readWord(uint8_t* a, int& idx) {
  uint32_t value = a[idx] << 24 | a[idx + 1] << 16 | a[idx + 2] << 8 | a[idx + 3];
  idx += 4;
  return value;
}

float readFloat(uint8_t* a, int& idx) {
  uint32_t bytes = readWord(a, idx);
  return *(float*)(&bytes);
}

void handleGameStart() {
  uint8_t* data = Msg.data;
  int idx = 0;
  
  //Reset CurrentGame variable
  CurrentGame = { };
  gameInProgress = true;
  
  //Load stage ID
  CurrentGame.stage = readHalf(data, idx);

  for (int i = 0; i < PLAYER_COUNT; i++) {
    Player& p = CurrentGame.players[i];
    
    //Load player data
    p.controllerPort = readByte(data, idx);
    p.characterId = readByte(data, idx);
    p.playerType = readByte(data, idx);
    p.characterColor = readByte(data, idx);
  }
}

void handleUpdate() {
  uint8_t* data = Msg.data;
  int idx = 0;
  
  //Check frame count and see if any frames were skipped
  uint32_t frameCount = readWord(data, idx);
  int framesMissed = frameCount - CurrentGame.frameCounter - 1;
  CurrentGame.framesMissed += framesMissed;
  CurrentGame.frameCounter = frameCount;

  CurrentGame.randomSeed = readWord(data, idx);

  for (int i = 0; i < PLAYER_COUNT; i++) {
    Player& p = CurrentGame.players[i];

    //Change over previous frame data
    p.previousFrameData = p.currentFrameData;

    PlayerFrameData& pfd = p.currentFrameData;

    //Load player data
    pfd = { };
    pfd.internalCharacterId = readByte(data, idx);
    pfd.animation = readHalf(data, idx);
    pfd.locationX = readFloat(data, idx);
    pfd.locationY = readFloat(data, idx);

    //Controller information
    pfd.joystickX = readFloat(data, idx);
    pfd.joystickY = readFloat(data, idx);
    pfd.cstickX = readFloat(data, idx);
    pfd.cstickY = readFloat(data, idx);
    pfd.trigger = readFloat(data, idx);
    pfd.buttons = readWord(data, idx);

    //More data
    pfd.percent = readFloat(data, idx);
    pfd.shieldSize = readFloat(data, idx);
    pfd.lastMoveHitId = readByte(data, idx);
    pfd.comboCount = readByte(data, idx);
    pfd.lastHitBy = readByte(data, idx);
    pfd.stocks = readByte(data, idx);

    //Raw controller information
    pfd.physicalButtons = readHalf(data, idx);
    pfd.lTrigger = readFloat(data, idx);
    pfd.rTrigger = readFloat(data, idx);
  }
}

bool handleGameEnd() {
  uint8_t* data = Msg.data;
  int idx = 0;
  
  CurrentGame.winCondition = readByte(data, idx);
  
  bool monitoredSinceStart = gameInProgress;
  
  // Clear flags used to control when connection attempts are made
  gameInProgress = false;
  connectAttempted = false;
  
  return monitoredSinceStart;
}

//**********************************************************************
//*                        Ethernet
//**********************************************************************
#define UDP_MAX_PACKET_SIZE 1024
#define RECONNECT_TIME_MS 15000
#define WAIT_FOR_RESPONSE_MS 20000

#define MSG_TYPE_DISCOVERY 1
#define MSG_TYPE_FLASH_ERASE 2
#define MSG_TYPE_LOG_MESSAGE 3
#define MSG_TYPE_SET_TARGET 4

#define GAME_BUFFER_COUNT 2

//MAC address. This address will be overwritten by MAC configured in USERREG0 and USERREG1 during ethernet initialization
byte mac[] = { 0x00, 0x1A, 0xB6, 0x02, 0xF5, 0x8C };
//byte mac[] = { 0x00, 0x1A, 0xB6, 0x02, 0xFA, 0xF8 };
char macString[20];

Game completedGamesBuffer[GAME_BUFFER_COUNT];

//***** The following server information should be defined in serverConfig.h ******
//char serverName[] = "google.com";
//int serverPort = 80;
//char page[] = "/page";
//
//#define PARAM_PATH_LENGTH 0
//
//char jsonTemplate[] = "{}";
//char templateParamsPath[PARAM_PATH_LENGTH][10] = {};
//*********************************************************************************

long timeSentPost = 0;
bool didSendPost = false;
EthernetClient client;

int udpPort = 3637;
IPAddress lastBroadcastIp(192, 168, 0, 4);
char ipString[20];
char ipPortString[30];
int lastBroadcastPort = 0;
EthernetUDP udp;

bool ethernetInitialized = false;

char* ipPortToString(IPAddress ip, int port) {
  sprintf(ipPortString, "%d.%d.%d.%d:%d", ip[0], ip[1], ip[2], ip[3], port);
  return ipPortString;
}

char* ipToString(IPAddress ip) {
  sprintf(ipString, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  return ipString;
}

char* macToString() {
  sprintf(macString, "%02X-%02X-%02X-%02X-%02X-%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return macString;
}

void ethernetInitialize() {
  sprintf(debugStrBuf, "Getting MAC Address from registers."); debugPrintln();
  loadMacAddress(mac);
  sprintf(debugStrBuf, "Read MAC from registers: %s", macToString()); debugPrintln();

  sprintf(debugStrBuf, "Attempting to obtain IP address from DHCP"); debugPrintln();
  
  // start the Ethernet connection:
  if (Ethernet.begin(mac)) {
    sprintf(debugStrBuf, "Obtained IP address: %s", ipToString(Ethernet.localIP())); debugPrintln();
  } else {
    sprintf(debugStrBuf, "Failed to configure Ethernet using DHCP"); debugPrintln();
    ethernetInitialized = false;
    return;
  }
  
  ethernetInitialized = true;
}

void unshiftCurrentGame() {
  // Shift everything to the right
  for (int i = GAME_BUFFER_COUNT - 2; i >= 0; i--) {
    completedGamesBuffer[i + 1] = completedGamesBuffer[i];
  }
  
  // Add currentGame to the front
  completedGamesBuffer[0] = CurrentGame;
}

void clearGamesBuffer() {
  for (int i = 0; i < GAME_BUFFER_COUNT; i++) {
    completedGamesBuffer[i] = { };
  }
}

void handlePostResponse() {
  if (!client.connected()) {
    // If we have lost connection, let's just reset state
    sprintf(debugStrBuf, "Lost connection waiting for response."); debugPrintln();
    didSendPost = false;
    client.stop();
    return;
  }
  
  // If we are connected, let's first check if we've timed out
  long currentTime = millis();
  if (currentTime - timeSentPost > WAIT_FOR_RESPONSE_MS) {
    sprintf(debugStrBuf, "Timed out waiting for response."); debugPrintln();
    didSendPost = false;
    client.stop();
    return;
  }
  
  // If we received something from the server, let's just for now assume it was successful
  if (client.available()) {
    sprintf(debugStrBuf, "Received response from server!"); debugPrintln();

    // TODO: Check response error code and don't mark as successful if it's not a 200.
    // TODO: Turn on/off status LEDs to indicate what went wrong with transfer

    // Debug server response
//    while (client.available()) {
//      sprintf(debugStrBuf, "%c", client.read()); debugPrint();
//    }
    
    clearGamesBuffer();
    didSendPost = false;
    client.stop();
    return;
  }
}

void writeOutGames() {
  Game& firstGame = completedGamesBuffer[0];
  if (firstGame.frameCounter == 0) {
    // If we dont have any games to write, do nothing
    return;
  }
  
  if (didSendPost) {
    handlePostResponse();
    return;  
  }
  
  // Connecting and sending can take a long time which can cause us to miss SPI bus reads 
  // so only do it if we don't have a game in progress
  // connectAttempted is here such that we will only try connecting once after a game ends
  // this flag is cleared when a game ends
  if (gameInProgress || connectAttempted) {
    return;
  }

  client.stop();
  
  char outBuf[64];
  connectAttempted = true;
  long connectTime = millis();
  if (client.connect(serverName, serverPort)) {
    long successTime = millis();
    sprintf(debugStrBuf, "Connected! Took %d ms. Writing games...", successTime - connectTime); debugPrintln();
    
    // send the header
    sprintf(outBuf,"POST %s HTTP/1.1", page);
    client.println(outBuf);
    sprintf(outBuf,"Host: %s", serverName);
    client.println(outBuf);
    client.println(F("Connection: close\r\nContent-Type: application/json"));
    printGameSummaries();
    
    // Go into state that will wait for response
    timeSentPost = millis();
    didSendPost = true;
  } else {
    long failTime = millis();
    sprintf(debugStrBuf, "Failed to connect to %s on port %d. Took %d ms.", serverName, serverPort, failTime - connectTime); debugPrintln();
  }
}

//This is the function that should be called every loop of the application
int ethernetExecute() {
  Ethernet.maintain();
  return 1;
}

//**********************************************************************
//*                           JSON
//**********************************************************************
void printGameSummaries() {
  StaticJsonBuffer<25000> jsonBuffer;
  
  JsonObject& root = jsonBuffer.parseObject(jsonTemplate);
  JsonVariant iRoot = root;
  for (int i = 0; i < PARAM_PATH_LENGTH; i++) {
    char* path = templateParamsPath[i];
    JsonObject& current = iRoot.as<JsonObject&>();
    iRoot = current[path];
  }
//  JsonObject& requestsRoot = root.containsKey("requests")
//  JsonObject& paramsRoot = root.containsKey("params") ? root["params"] : root;

  JsonObject& paramsRoot = iRoot.as<JsonObject&>();
  paramsRoot["macAddress"] = macToString();

//  root.prettyPrintTo(debugStrBuf, sizeof(debugStrBuf)); debugPrintln();
  
  JsonArray& games = paramsRoot.createNestedArray("games");
  
  for (int k = (GAME_BUFFER_COUNT - 1); k >= 0; k--) {
    Game& kGame = completedGamesBuffer[k];
    if (kGame.frameCounter == 0) {
      // If this game element is empty, don't write it out
      continue;
    }
    
    JsonObject& game = jsonBuffer.createObject();
    game["frames"] = kGame.frameCounter;
    game["framesMissed"] = kGame.framesMissed;
    game["winCondition"] = kGame.winCondition;
    game["stage"] = kGame.stage;

    float totalActiveGameFrames = float(kGame.frameCounter);
    
    JsonArray& data = game.createNestedArray("players");
    for (int i = 0; i < PLAYER_COUNT; i++) {
      Player& currentPlayer = kGame.players[i];
      PlayerStatistics& ps = currentPlayer.stats;
      JsonObject& item = jsonBuffer.createObject();

      // Calculate played character
      uint8_t playedCharacterId = currentPlayer.characterId;
      if (playedCharacterId == EXTERNAL_ZELDA || playedCharacterId == EXTERNAL_SHEIK) {
        uint32_t zeldaFrames = ps.internalCharUsage[INTERNAL_ZELDA];
        uint32_t sheikFrames = ps.internalCharUsage[INTERNAL_SHEIK];

        playedCharacterId = zeldaFrames > sheikFrames ? EXTERNAL_ZELDA : EXTERNAL_SHEIK;
      }
      
      item["port"] = currentPlayer.controllerPort + 1;
      item["character"] = playedCharacterId;
      item["color"] = currentPlayer.characterColor;
      item["playerType"] = currentPlayer.playerType;
      
      item["stocksRemaining"] = currentPlayer.currentFrameData.stocks;
      
      item["apm"] = 3600 * (ps.actionCount / totalActiveGameFrames);
      
      item["averageDistanceFromCenter"] = ps.averageDistanceFromCenter;
      item["percentTimeClosestCenter"] = 100 * (ps.framesClosestCenter / totalActiveGameFrames);
      item["percentTimeAboveOthers"] = 100 * (ps.framesAboveOthers / totalActiveGameFrames);
      item["percentTimeInShield"] = 100 * (ps.framesInShield / totalActiveGameFrames);
      item["framesWithoutDamage"] = ps.mostFramesWithoutDamage;

      item["rollCount"] = ps.rollCount;
      item["spotDodgeCount"] = ps.spotDodgeCount;
      item["airDodgeCount"] = ps.airDodgeCount;
      
      JsonArray& stocks = item.createNestedArray("stocks");
      for (int j = 0; j < STOCK_COUNT; j++) {
        StockStatistics& ss = ps.stocks[j];
        
        //Only log the stock if the player actually played that stock
        if (ss.frameStart > 0 || j == 0) {
          JsonObject& stock = jsonBuffer.createObject();
          
          stock["frameStart"] = ss.frameStart;
          stock["frameEnd"] = ss.frameEnd; 
          stock["percent"] = ss.percent;
          stock["moveLastHitBy"] = ss.lastHitBy;
          stock["lastAnimation"] = ss.lastAnimation;
          stock["openingsAllowed"] = ss.killedInOpenings;
        
          stocks.add(stock);
        }
      }
      
      JsonArray& comboStrings = item.createNestedArray("comboStrings");
      for (int j = 0; j < COMBO_STRING_BUFFER_SIZE; j++) {
        ComboString& cs = ps.comboStrings[j];
       
        if (cs.frameEnd == 0) {
          break;
        } 
        
        JsonObject& comboString = jsonBuffer.createObject();
        comboString["frameStart"] = cs.frameStart;
        comboString["frameEnd"] = cs.frameEnd;
        comboString["percentStart"] = cs.percentStart;
        comboString["percentEnd"] = cs.percentEnd;
        comboString["hitCount"] = cs.hitCount;
        
        comboStrings.add(comboString);
      }
      
      JsonArray& recoveries = item.createNestedArray("recoveries");
      for (int j = 0; j < RECOVERY_BUFFER_SIZE; j++) {
        Recovery& r = ps.recoveries[j];
       
        if (r.frameEnd == 0) {
          break;
        } 
        
        JsonObject& recovery = jsonBuffer.createObject();
        recovery["frameStart"] = r.frameStart;
        recovery["frameEnd"] = r.frameEnd;
        recovery["percentStart"] = r.percentStart;
        recovery["percentEnd"] = r.percentEnd;
        recovery["isSuccessful"] = r.isSuccessful;
        
        recoveries.add(recovery);
      }

      JsonArray& punishes = item.createNestedArray("punishes");
      for (int j = 0; j < PUNISH_BUFFER_SIZE; j++) {
        Punish& p = ps.punishes[j];
       
        if (p.frameEnd == 0) {
          break;
        } 
        
        JsonObject& punish = jsonBuffer.createObject();
        punish["frameStart"] = p.frameStart;
        punish["frameEnd"] = p.frameEnd;
        punish["percentStart"] = p.percentStart;
        punish["percentEnd"] = p.percentEnd;
        punish["hitCount"] = p.hitCount;
        punish["isKill"] = p.isKill;
        
        punishes.add(punish);
      }
      
      data.add(item);
    }
    
    games.add(game);
  }

  char outBuf[64];
  int contentLength = root.measureLength();
  sprintf(debugStrBuf, "Length is %u", contentLength); debugPrintln();
  sprintf(outBuf, "Content-Length: %u\r\n", contentLength);
  client.println(outBuf);
  root.printTo(client);
  client.println();
  sprintf(debugStrBuf, "Done printing"); debugPrintln();
}

//**********************************************************************
//*                            Statistics
//**********************************************************************
int numberOfSetBits(uint16_t x) {
  //This function solves the Hamming Weight problem. Effectively it counts the number of bits in the input that are set to 1
  //This implementation is supposedly very efficient when most bits are zero. Found: https://en.wikipedia.org/wiki/Hamming_weight#Efficient_implementation
  int count;
  for (count=0; x; count++) x &= x-1;
  return count;
}

void computeStatistics() {
  //this function will only get called when frameCount >= 1
  uint32_t framesSinceStart = CurrentGame.frameCounter - 1;
  
  Player* p = CurrentGame.players;
  
  float p1CenterDistance = sqrt(pow(p[0].currentFrameData.locationX, 2) + pow(p[0].currentFrameData.locationY, 2));
  float p2CenterDistance = sqrt(pow(p[1].currentFrameData.locationX, 2) + pow(p[1].currentFrameData.locationY, 2));
  
  p[0].stats.averageDistanceFromCenter = (framesSinceStart*p[0].stats.averageDistanceFromCenter + p1CenterDistance) / (framesSinceStart + 1);
  p[1].stats.averageDistanceFromCenter = (framesSinceStart*p[1].stats.averageDistanceFromCenter + p2CenterDistance) / (framesSinceStart + 1);
  
  //Increment frame counter of person who is closest to center. If the players are even distances from the center, do not increment
  if (p1CenterDistance < p2CenterDistance) p[0].stats.framesClosestCenter++;
  else if (p2CenterDistance < p1CenterDistance) p[1].stats.framesClosestCenter++;

  //Increment frame counter of person who is highest;
  if (p[0].currentFrameData.locationY > p[1].currentFrameData.locationY) p[0].stats.framesAboveOthers++;
  else if (p[1].currentFrameData.locationY > p[0].currentFrameData.locationY) p[1].stats.framesAboveOthers++;
  
  for (int i = 0; i < PLAYER_COUNT; i++) {
    Player& cp = p[i]; //Current player
    Player& op = p[!i]; //Other player
    
    bool lostStock = cp.previousFrameData.stocks - cp.currentFrameData.stocks > 0;
    bool opntLostStock = op.previousFrameData.stocks - op.currentFrameData.stocks > 0;
    
    //Check current action states, although many of these conditions check previous frame data, it shouldn't matter for frame = 1 that there is no previous
    if (cp.currentFrameData.animation >= GUARD_START && cp.currentFrameData.animation <= GUARD_END) cp.stats.framesInShield++;
    else if ((cp.currentFrameData.animation == ROLL_FORWARD && cp.previousFrameData.animation != ROLL_FORWARD) ||
             (cp.currentFrameData.animation == ROLL_BACKWARD && cp.previousFrameData.animation != ROLL_BACKWARD)) cp.stats.rollCount++;
    else if (cp.currentFrameData.animation == SPOT_DODGE && cp.previousFrameData.animation != SPOT_DODGE) cp.stats.spotDodgeCount++;
    else if (cp.currentFrameData.animation == AIR_DODGE && cp.previousFrameData.animation != AIR_DODGE) cp.stats.airDodgeCount++;
    
    //Check if we are getting damaged
    bool tookPercent = cp.currentFrameData.percent - cp.previousFrameData.percent > 0;
    if (tookPercent) {
      cp.flags.framesWithoutDamage = 0;
    } else {
      cp.flags.framesWithoutDamage++; //Increment count of frames without taking damage

      //If frames without being hit is greater than previous, set new record
      if (cp.flags.framesWithoutDamage > cp.stats.mostFramesWithoutDamage) cp.stats.mostFramesWithoutDamage = cp.flags.framesWithoutDamage; 
    }

    //Modify internal character counter (sheik/zelda detection)
    uint8_t internalCharId = cp.currentFrameData.internalCharacterId;
    cp.stats.internalCharUsage[internalCharId]++;
    
    //------------------------------- Monitor Combo Strings -----------------------------------------
    bool opntTookDamage = op.currentFrameData.percent - op.previousFrameData.percent > 0;
    bool opntDamagedState = op.currentFrameData.animation >= DAMAGE_START && op.currentFrameData.animation <= DAMAGE_END;
    bool opntGrabbedState = op.currentFrameData.animation >= CAPTURE_START && op.currentFrameData.animation <= CAPTURE_END;
    bool opntTechState = (op.currentFrameData.animation >= TECH_START && op.currentFrameData.animation <= TECH_END) ||
      op.currentFrameData.animation == TECH_MISS_UP || op.currentFrameData.animation == TECH_MISS_DOWN;

    //By looking for percent changes we can increment counter even when a player gets true combo'd
    //The damage state requirement makes it so things like fox's lasers, grab pummels, pichu damaging self, etc don't increment count
    if (opntTookDamage && (opntDamagedState || opntGrabbedState)) {
      if (cp.flags.stringCount == 0) {
        cp.flags.stringStartPercent = op.previousFrameData.percent;
        cp.flags.stringStartFrame = CurrentGame.frameCounter;
      }
      
      cp.flags.stringCount++; //increment number of hits
    }

    //Reset combo string counter when somebody dies or doesn't get hit for too long
    if (opntDamagedState || opntGrabbedState || opntTechState) cp.flags.stringResetCounter = 0;
    else if (cp.flags.stringCount > 0) cp.flags.stringResetCounter++;

    //Mark combo completed if opponent lost his stock or if the counter is greater than threshold frames
    if (cp.flags.stringCount > 0 && (opntLostStock || lostStock || cp.flags.stringResetCounter > COMBO_STRING_TIMEOUT)) {
      ComboString& cs = cp.stats.comboStrings[cp.stats.comboStringIndex];
      cs.frameStart = cp.flags.stringStartFrame;
      cs.frameEnd = CurrentGame.frameCounter;
      cs.percentStart = cp.flags.stringStartPercent;
      cs.percentEnd = op.previousFrameData.percent;
      cs.hitCount = cp.flags.stringCount;
      
      if (cp.stats.comboStringIndex < COMBO_STRING_BUFFER_SIZE - 1) {
         cp.stats.comboStringIndex++;
      }
      
      //Reset string count
      cp.flags.stringCount = 0;
    }
    
    //------------------- Increment Action Count for APM Calculation --------------------------------
    //First count the number of buttons that go from 0 to 1
    uint16_t buttonChanges = (~cp.previousFrameData.physicalButtons & cp.currentFrameData.physicalButtons) & 0xFFF;
    cp.stats.actionCount += numberOfSetBits(buttonChanges); //Increment action count by amount of button presses
    
    //Increment action count when sticks change from one region to another. Don't increment when stick returns to deadzone
    uint8_t prevAnalogRegion = getJoystickRegion(cp.previousFrameData.joystickX, cp.previousFrameData.joystickY);
    uint8_t currentAnalogRegion = getJoystickRegion(cp.currentFrameData.joystickX, cp.currentFrameData.joystickY);
    if ((prevAnalogRegion != currentAnalogRegion) && (currentAnalogRegion != 0)) cp.stats.actionCount++;
    
    //Do the same for c-stick
    uint8_t prevCstickRegion = getJoystickRegion(cp.previousFrameData.cstickX, cp.previousFrameData.cstickY);
    uint8_t currentCstickRegion = getJoystickRegion(cp.currentFrameData.cstickX, cp.currentFrameData.cstickY);
    if ((prevCstickRegion != currentCstickRegion) && (currentCstickRegion != 0)) cp.stats.actionCount++;
    
    //Increment action on analog trigger... I'm not sure when. This needs revision
    if (cp.previousFrameData.lTrigger < 0.3 && cp.currentFrameData.lTrigger >= 0.3) cp.stats.actionCount++;
    if (cp.previousFrameData.rTrigger < 0.3 && cp.currentFrameData.rTrigger >= 0.3) cp.stats.actionCount++;
    
    //--------------------------- Recovery detection --------------------------------------------------
    bool isOffStage = checkIfOffStage(CurrentGame.stage, cp.currentFrameData.locationX, cp.currentFrameData.locationY);
    bool isInControl = cp.currentFrameData.animation >= GROUNDED_CONTROL_START && cp.currentFrameData.animation <= GROUNDED_CONTROL_END;
    bool beingDamaged = cp.currentFrameData.animation >= DAMAGE_START && cp.currentFrameData.animation <= DAMAGE_END;
    bool beingGrabbed = cp.currentFrameData.animation >= CAPTURE_START && cp.currentFrameData.animation <= CAPTURE_END;
    bool isDying = cp.currentFrameData.animation >= DYING_START && cp.currentFrameData.animation <= DYING_END;
    
    if (!cp.flags.isRecovering && !cp.flags.isHitOffStage && beingDamaged && isOffStage) {
      //If player took a hit off stage
      cp.flags.isHitOffStage = true;
    }
    else if (!cp.flags.isRecovering && cp.flags.isHitOffStage && !beingDamaged && !isDying && isOffStage) {
      //If player exited damage state off stage
      cp.flags.isRecovering = true;
      cp.flags.recoveryStartPercent = cp.currentFrameData.percent;
      cp.flags.recoveryStartFrame = CurrentGame.frameCounter;
    }
    else if (!cp.flags.isLandedOnStage && (cp.flags.isRecovering || cp.flags.isHitOffStage) && isInControl && !isOffStage) {
      //If a player is in control of his character after recovering flag as landed
      cp.flags.isLandedOnStage = true;
    }
    else if (cp.flags.isLandedOnStage && isOffStage) {
      //If player landed but is sent back off stage, continue recovery process
      cp.flags.framesSinceLanding = 0;
      cp.flags.isLandedOnStage = false;
    }
    else if (cp.flags.isLandedOnStage && !isOffStage && !beingDamaged && !beingGrabbed) {
      //If player landed, is still on stage, is not being hit, and is not grabbed, increment frame counter
      cp.flags.framesSinceLanding++;
      
      //If frame counter while on stage passes threshold, consider it a successful recovery
      if (cp.flags.framesSinceLanding > FRAMES_LANDED_RECOVERY) {
        appendRecovery(true, cp, CurrentGame.frameCounter);
        resetRecoveryFlags(cp.flags);
      }
    }
    
    if ((cp.flags.isRecovering || cp.flags.isHitOffStage) && lostStock) {
      //If player dies while recovering, consider it a failed recovery
      if (cp.flags.isRecovering) {
        appendRecovery(false, cp, CurrentGame.frameCounter);
      }
      
      resetRecoveryFlags(cp.flags);
    }

    //----------------------------- Punish detection --------------------------------------------------
    bool opntInControl = op.currentFrameData.animation >= GROUNDED_CONTROL_START && op.currentFrameData.animation <= GROUNDED_CONTROL_END;
    
    if (opntTookDamage && (opntDamagedState || opntGrabbedState)) {
      // Successfully hit opponent, check if we already have a punish going
      if (!cp.flags.isPunishing) {
        // If we didn't have a punish going, start a new one 
        cp.flags.punishStartPercent = op.previousFrameData.percent;
        cp.flags.punishStartFrame = CurrentGame.frameCounter;
        cp.stats.numberOfOpenings++;
        cp.flags.isPunishing = true;
      }

      cp.flags.punishHitCount++; //increment number of hits
    }

    if (opntDamagedState || opntGrabbedState) {
      // If opponent got grabbed or damaged, reset the punish reset counter
      cp.flags.framesSincePunishReset = 0; //reset the punish reset timer
    }

    bool startNeutralCount = cp.flags.framesSincePunishReset == 0 && opntInControl;
    bool continueNeutralCount = cp.flags.framesSincePunishReset > 0;
    if (startNeutralCount || continueNeutralCount) {
      // This will increment the reset timer under the following conditions:
      // 1) if we were punishing opponent but they have now entered an actionable state
      // 2) if counter has already started counting meaning opponent has entered actionable state
      cp.flags.framesSincePunishReset++;
    }

    // Termination condition 1 - we kill our opponent
    if (cp.flags.isPunishing && opntLostStock) {
      appendPunish(true, cp, op, CurrentGame.frameCounter);
      resetPunishFlags(cp.flags);
    }

    // Termination condition 2 - we have not re-hit our opponent in buffer amount
    if (cp.flags.isPunishing && cp.flags.framesSincePunishReset > FRAMES_LANDED_PUNISH) {
      appendPunish(false, cp, op, CurrentGame.frameCounter);
      resetPunishFlags(cp.flags);
    }
    
    //-------------------------- Stock specific stuff -------------------------------------------------
    int prevStockIndex = STOCK_COUNT - cp.previousFrameData.stocks;
    if (prevStockIndex >= 0 && prevStockIndex < STOCK_COUNT) {
      StockStatistics& s = cp.stats.stocks[prevStockIndex];
      
      if (s.frameStart == 0) s.frameStart = CurrentGame.frameCounter;
      s.percent = cp.currentFrameData.percent;
      s.lastHitBy = op.currentFrameData.lastMoveHitId; //This will indicate what this player was killed by
      s.lastAnimation = cp.currentFrameData.animation; //What was character doing before death
    }
    
    //Mark last stock as lost if lostStock is true
    if (lostStock && prevStockIndex >= 0 && prevStockIndex < STOCK_COUNT) {
      int16_t prevOpenings = 0;
      for (int i = prevStockIndex - 1; i >= 0; i--) prevOpenings += cp.stats.stocks[i].killedInOpenings;
      
      cp.stats.stocks[prevStockIndex].killedInOpenings = op.stats.numberOfOpenings - prevOpenings;
      cp.stats.stocks[prevStockIndex].frameEnd = CurrentGame.frameCounter;

      sprintf(debugStrBuf, "Player %c lost a stock. (%d, %d)", (char)(65 + i), cp.currentFrameData.animation, cp.previousFrameData.animation); debugPrintln();
    }
  }
}

//**********************************************************************
//*                             Setup
//**********************************************************************
void checkFlashErase() {
  pinMode(PJ_0, INPUT);
  
  if (digitalRead(PJ_0) == HIGH) {
    sprintf(debugStrBuf, "Erasing flash..."); debugPrintln();
    eraseFlash();
    sprintf(debugStrBuf, "Flash erased?"); debugPrintln();
  }
}

void setup() {
  Serial.begin(115200);

  sprintf(debugStrBuf, "Starting initialization."); debugPrintln();
  
  checkFlashErase();
  ethernetInitialize();
  asmEventsInitialize();
  spiSlaveInitialize();

  sprintf(debugStrBuf, "Initialization complete."); debugPrintln();
}

//**********************************************************************
//*                             Debug
//**********************************************************************
bool sendSerialDebugMessages = true;

void debugPrintln() {
  sprintf(debugStrBuf, "%s\r\n", debugStrBuf); debugPrint();
}

void debugPrint() {
  if (sendSerialDebugMessages) {
    //Print serial message
    Serial.print(debugStrBuf);
  }
}

//**********************************************************************
//*                           Main Loop
//**********************************************************************
void loop() {
  //If ethernet client not working, attempt to re-establish
  ethernetExecute();
  
  //read a message from the read fifo - this function returns if no message available
  spiReadMessage();
  
  if (Msg.success) {
    switch (Msg.eventCode) {
      case EVENT_GAME_START:
        sprintf(debugStrBuf, "Game started..."); debugPrintln();
        handleGameStart();
        break;
      case EVENT_UPDATE:
        handleUpdate();
        computeStatistics();
        break;
      case EVENT_GAME_END:
        sprintf(debugStrBuf, "Game ended..."); debugPrintln();
        bool monitoredSinceStart = handleGameEnd();
        if (monitoredSinceStart) unshiftCurrentGame();
        break;
    }
  }
  
  //this will write games out to the server if there are any
  writeOutGames();
}

