#include <SPI.h>
#include <Ethernet.h>
#include <ArduinoJson.h>

#include "enhmelee.h"

//**********************************************************************
//*                         ASM Event Codes
//**********************************************************************
#define EVENT_GAME_START 0x37
#define EVENT_UPDATE 0x38
#define EVENT_GAME_END 0x39

int asmEvents[256];

void asmEventsInitialize() {
  asmEvents[EVENT_GAME_START] = 0xA;
  asmEvents[EVENT_UPDATE] = 0x66;
  asmEvents[EVENT_GAME_END] = 0x1;
}

//**********************************************************************
//*               FPGA Read FIFO Communication Functions
//**********************************************************************
#define EMPTY_BIT_MASK 0x2000
#define FULL_BIT_MASK 0x1000
#define ACK_BIT_MASK 0x4000
#define ACK_PIN 49
#define EMPTY_PIN 50
#define FULL_PIN 51
#define READ_DATA_START_PIN 33
#define READ_DATA_END_PIN 40

word PortC;
RfifoMessage Msg; //Keep an RfifoMessage variable as a global variable to prevent memory leak? Does arduino do garbage collection?

void rfifoInitialize() {
  //Set up fifo data pins as inputs
  for (int i = READ_DATA_START_PIN; i <= READ_DATA_END_PIN; i++) pinMode(i, INPUT);

  //Set up communication pins
  pinMode(ACK_PIN, OUTPUT);
  pinMode(EMPTY_PIN, INPUT);
  pinMode(FULL_PIN, INPUT);

  //Ensure ack pin starts low
  digitalWrite(ACK_PIN, LOW);

  //Empty the FPGA buffer on startup
  while((PIOC->PIO_PDSR & EMPTY_BIT_MASK) == 0) {
    digitalWrite(ACK_PIN, HIGH);
    digitalWrite(ACK_PIN, LOW);
  }
}

//Reads the pins corresponding with the FIFO communication
void rfifoReadPins() {
  PortC = PIOC->PIO_PDSR;
}

//Uses values read in previous read to determine if data is available
bool rfifoDataAvailable() {
  return (PortC & EMPTY_BIT_MASK) == 0;
}

//Pops a byte off of the queue. This should only be called if data is available
uint8_t rfifoPopByte() {
  uint8_t read = (uint8_t)(PortC >> 1);

  //Acknowledge the read
  digitalWrite(ACK_PIN, HIGH);
  digitalWrite(ACK_PIN, LOW);

  return read;
}

void rfifoReadMessage() {
  Msg = { false, 0, 0, 0, 0 };
  uint8_t byteNum;
  
  do {
    //Wait until data is available in the queue
    do { rfifoReadPins(); } while(!rfifoDataAvailable());
    byteNum = rfifoPopByte(); //First byte popped is the num
  
    //Wait until data is available in the queue
    do { rfifoReadPins(); } while(!rfifoDataAvailable());
    Msg.eventCode = rfifoPopByte(); //if byteNum is zero, this will be the event code
  } while(byteNum != 0);
  
  //Get the expected message size for this event
  Msg.messageSize = asmEvents[Msg.eventCode];
  if (Msg.messageSize <= 0) return; //If message size for this event is zero, it is not a valid event, return a false success message

  for (int i = 0; i < Msg.messageSize; i++) {
    //Wait until data is available in the queue
    do { rfifoReadPins(); } while(!rfifoDataAvailable());
    uint8_t newByteNum = rfifoPopByte(); //Get byte index
    
    //Wait until data is available in the queue
    do { rfifoReadPins(); } while(!rfifoDataAvailable());
    uint8_t readValue = rfifoPopByte(); //Get byte value

    //Given one of these conditions, something went wrong
    if (newByteNum == 0 || newByteNum < byteNum || i != (newByteNum - 1)) return;

    byteNum = newByteNum;

    //Load byte into buffer
    Msg.data[i] = readValue;
	
	  //Keep track of the number of bytes read (this is in case we miss reads we will know how many we did successfully read)
	  Msg.bytesRead = i + 1;
  }
  
  Msg.success = true;
}

//**********************************************************************
//*                         Event Handlers
//**********************************************************************
#define GAME_START_PLAYER_FRAME_BYTES 4;
#define UPDATE_PLAYER_FRAME_BYTES 47

Game CurrentGame = { };

uint16_t readHalf(uint8_t* a) {
  return a[0] << 8 | a[1];
}

uint32_t readWord(uint8_t* a) {
  return a[0] << 24 | a[1] << 16 | a[2] << 8 | a[3];
}

float readFloat(uint8_t* a) {
  uint32_t bytes = readWord(a);
  return *(float*)(&bytes);
}

void handleGameStart() {
  uint8_t* data = Msg.data;
  
  //Reset CurrentGame variable
  CurrentGame = { };

//  Serial.println("Hello");
//  //Debug
//  for (int i = 0; i < 0xA; i++) {
//    Serial.print(Msg.data[i], HEX);
//    Serial.print(" ");
//  }
  
  //Load stage ID
  CurrentGame.stage = readHalf(&data[0]);;

  for (int i = 0; i < PLAYER_COUNT; i++) {
    Player* p = &CurrentGame.players[i];
    int offset = i * GAME_START_PLAYER_FRAME_BYTES;

    //Load player data
    p->controllerPort = data[2 + offset];
    p->characterId = data[3 + offset];
    p->playerType = data[4 + offset];
    p->characterColor = data[5 + offset];
  }
}

void handleUpdate() {
  uint8_t* data = Msg.data;
  
  //Check frame count and see if any frames were skipped
  uint32_t frameCount = readWord(&data[0]);
  int framesMissed = frameCount - CurrentGame.frameCounter - 1;
  CurrentGame.framesMissed += framesMissed;
  CurrentGame.frameCounter = frameCount;

  CurrentGame.randomSeed = readWord(&data[4]);

  for (int i = 0; i < PLAYER_COUNT; i++) {
    Player* p = &CurrentGame.players[i];
    int offset = i * UPDATE_PLAYER_FRAME_BYTES;

    //Change over previous frame data
    p->previousFrameData = p->currentFrameData;

    PlayerFrameData* pfd = &p->currentFrameData;

    //Load player data
    *pfd = { };
    pfd->internalCharacterId = data[8 + offset];
    pfd->animation = readHalf(&data[9 + offset]);
    pfd->locationX = readFloat(&data[11 + offset]);
    pfd->locationY = readFloat(&data[15 + offset]);

    //Controller information
    pfd->joystickX = readFloat(&data[19 + offset]);
    pfd->joystickY = readFloat(&data[23 + offset]);
    pfd->cstickX = readFloat(&data[27 + offset]);
    pfd->cstickY = readFloat(&data[31 + offset]);
    pfd->trigger = readFloat(&data[35 + offset]);
    pfd->buttons = readWord(&data[39 + offset]);

    //More data
    pfd->percent = readFloat(&data[43 + offset]);
    pfd->shieldSize = readFloat(&data[47 + offset]);
    pfd->lastMoveHitId = data[51 + offset];
    pfd->comboCount = data[52 + offset];
    pfd->lastHitBy = data[53 + offset];
    pfd->stocks = data[54 + offset];
  }
}

void handleGameEnd() {
  uint8_t* data = Msg.data;

  CurrentGame.winCondition = data[0];
}

//**********************************************************************
//*                           Ethernet
//**********************************************************************
#define LED_PIN 13
#define DELAY_TIME 10000

// the media access control (ethernet hardware) address for the shield:
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0F, 0x70, 0x6C };
IPAddress server(192, 168, 0, 2);
int port = 3636;

EthernetClient client;

void ethernetInitialize() {
  pinMode(LED_PIN, OUTPUT);

  Serial.println("Attempting to obtain IP address from DHCP");
  
  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    delay(DELAY_TIME);
    return;
  }

  Serial.print("Obtained IP address: "); Serial.println(Ethernet.localIP());

  ethernetConnect();
}

void ethernetConnect() {
  Serial.print("Trying to connect to: "); Serial.println(server);
  int code = client.connect(server, port);
  if (code) {
    Serial.println("Connected to server!");
    postConnectedMessage();
  } else {
    Serial.print("Failed to connect to server. Code: "); Serial.println(code);
    delay(DELAY_TIME);
  }
}

int ethernetCheckConnection() {
  //Checks if connection is working. If it isn't, attempt to reconnect.
  if(!client) {
	  client.stop();
    ethernetInitialize();
    return -1;
  }

  if (!client.connected()) {
    client.stop();
    Ethernet.maintain();
    ethernetConnect();
    return -2;
  }

  Ethernet.maintain();
  return 1;
}

void blinkEndlessly(uint32_t msDelay) {
  for(;;) {
     digitalWrite(LED_PIN, HIGH);
     delay(msDelay);
     digitalWrite(LED_PIN, LOW);
     delay(msDelay);
  }
}

//**********************************************************************
//*                           JSON
//**********************************************************************
void postMatchParameters() {
  if (client.connected()) {
    //Create JSON
    StaticJsonBuffer<400> jsonBuffer;
  
    JsonObject& root = jsonBuffer.createObject();
    root["stage"] = CurrentGame.stage;
  
    JsonArray& data = root.createNestedArray("players");
    for (int i = 0; i < PLAYER_COUNT; i++) {
      Player* p = &CurrentGame.players[i];
      JsonObject& item = jsonBuffer.createObject();
      
      item["port"] = p->controllerPort + 1;
      item["character"] = p->characterId;
      item["color"] = p->characterColor;
      item["type"] = p->playerType;
  
      data.add(item);
    }
  
    root.printTo(client);
    client.println();
  }
}

void postConnectedMessage() {
  if (client.connected()) {    
    //Create JSON
    StaticJsonBuffer<200> jsonBuffer;
  
    JsonObject& root = jsonBuffer.createObject();
    JsonArray& macBytes = root.createNestedArray("mac");
    for (int i = 0; i < sizeof(mac); i++) macBytes.add(mac[i]);
  
    root.printTo(client);
    client.println();
  }
}

void postGameEndMessage() {
  if (client.connected()) {
    StaticJsonBuffer<4000> jsonBuffer;
    
    JsonObject& root = jsonBuffer.createObject();
    root["frames"] = CurrentGame.frameCounter;
    root["winCondition"] = CurrentGame.winCondition;

    float totalActiveGameFrames = float(CurrentGame.frameCounter);
    
    JsonArray& data = root.createNestedArray("players");
    for (int i = 0; i < PLAYER_COUNT; i++) {
      //Serial.println(String("Writing out player ") + i);
      PlayerStatistics& ps = CurrentGame.players[i].stats;
      JsonObject& item = jsonBuffer.createObject();

      item["stocksRemaining"] = CurrentGame.players[i].currentFrameData.stocks;
      
      item["averageDistanceFromCenter"] = ps.averageDistanceFromCenter;
      item["percentTimeClosestCenter"] = 100 * (ps.framesClosestCenter / totalActiveGameFrames);
      item["percentTimeAboveOthers"] = 100 * (ps.framesAboveOthers / totalActiveGameFrames);
      item["percentTimeInShield"] = 100 * (ps.framesInShield / totalActiveGameFrames);
      item["secondsWithoutDamage"] = float(ps.mostFramesWithoutDamage) / 60;

      item["rollCount"] = ps.rollCount;
      item["spotDodgeCount"] = ps.spotDodgeCount;
      item["airDodgeCount"] = ps.airDodgeCount;
      
      item["recoveryAttempts"] = ps.recoveryAttempts;
      item["successfulRecoveries"] = ps.successfulRecoveries;
      
      JsonArray& stocks = item.createNestedArray("stocks");
      for (int j = 0; j < STOCK_COUNT; j++) {
        //Serial.println(String("Writing out player ") + i + String(". Stock: ") + j);
        StockStatistics& ss = ps.stocks[j];
        
        //Only log the stock if the player actually played that stock
        if (ss.isStockUsed) {
          JsonObject& stock = jsonBuffer.createObject();
        
          uint32_t stockFrames = ss.frame;
          if (j > 0) stockFrames -= ps.stocks[j - 1].frame;
          
          stock["timeSeconds"] = float(stockFrames) / 60; 
          stock["percent"] = ss.percent;
          stock["moveLastHitBy"] = ss.lastHitBy;
        
          stocks.add(stock);
        }
      }
      
      data.add(item);
    }

    root.printTo(client);
    client.println();
  }
}

//**********************************************************************
//*                            Statistics
//**********************************************************************
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
    
    //Check current action states, although many of these conditions check previous frame data, it shouldn't matter for frame = 1 that there is no previous
    if (cp.currentFrameData.animation >= GUARD_START && cp.currentFrameData.animation <= GUARD_END) cp.stats.framesInShield++;
    else if ((cp.currentFrameData.animation == ROLL_FORWARD && cp.previousFrameData.animation != ROLL_FORWARD) ||
             (cp.currentFrameData.animation == ROLL_BACKWARD && cp.previousFrameData.animation != ROLL_BACKWARD)) cp.stats.rollCount++;
    else if (cp.currentFrameData.animation == SPOT_DODGE && cp.previousFrameData.animation != SPOT_DODGE) cp.stats.spotDodgeCount++;
    else if (cp.currentFrameData.animation == AIR_DODGE && cp.previousFrameData.animation != AIR_DODGE) cp.stats.airDodgeCount++;
    
    bool beingDamaged = cp.currentFrameData.animation >= DAMAGE_START && cp.currentFrameData.animation <= DAMAGE_END;
    
    //Check if we are getting damaged
    if (beingDamaged) {
      cp.flags.framesWithoutDamage = 0;
    } else {
      cp.flags.framesWithoutDamage++; //Increment count of frames without taking damage

      //If frames without being hit is greater than previous, set new record
      if (cp.flags.framesWithoutDamage > cp.stats.mostFramesWithoutDamage) cp.stats.mostFramesWithoutDamage = cp.flags.framesWithoutDamage; 
    }
    
    //Recovery detection
    bool isOffStage = checkIfOffStage(CurrentGame.stage, cp.currentFrameData.locationX, cp.currentFrameData.locationY);
    bool isInControl = cp.currentFrameData.animation == ACTION_WAIT || cp.currentFrameData.animation == ACTION_DASH || cp.currentFrameData.animation == ACTION_KNEE_BEND;
    if (!cp.flags.isRecovering && !cp.flags.isHitOffStage && beingDamaged && isOffStage) {
      //If player took a hit off stage
      cp.flags.isHitOffStage = true;
    }
    else if (!cp.flags.isRecovering && cp.flags.isHitOffStage && !beingDamaged && isOffStage) {
      //If player exited damage state off stage
      cp.flags.isRecovering = true;
    }
    else if ((cp.flags.isRecovering || cp.flags.isHitOffStage) && isInControl) {
      //If a player is in control of his character after recovering flag as landed
      cp.flags.isLandedOnStage = true;
    }
    else if (cp.flags.isLandedOnStage && isOffStage) {
      //If player landed but is sent back off stage, continue recovery process
      cp.flags.framesSinceLanding = 0;
      cp.flags.isLandedOnStage = false;
    }
    else if (cp.flags.isLandedOnStage && !isOffStage) {
      //If player landed and is still on stage, increment frame counter
      cp.flags.framesSinceLanding++;
      
      //If frame counter while on stage passes threshold, consider it a successful recovery
      if (cp.flags.framesSinceLanding > FRAMES_LANDED_RECOVERY) {
        cp.stats.recoveryAttempts++;
        cp.stats.successfulRecoveries++;
        resetRecoveryFlags(cp.flags);
      }
    }
    else if (cp.flags.isRecovering && lostStock) {
      //TODO: Check if this function gets called on last stock lost
      //If player dies while recovering, consider it a failed recovery
      cp.stats.recoveryAttempts++;
      resetRecoveryFlags(cp.flags);
    }
    
    //Stock specific stuff
    int stockIndex = STOCK_COUNT - cp.currentFrameData.stocks;
    if (stockIndex >= 0 && stockIndex < STOCK_COUNT) {
      StockStatistics& s = cp.stats.stocks[stockIndex];
      s.frame = CurrentGame.frameCounter;
      s.percent = cp.currentFrameData.percent;
      s.lastHitBy = op.currentFrameData.lastMoveHitId;
      s.isStockUsed = true;
    }
  }
}

//**********************************************************************
//*                             Setup
//**********************************************************************
void setup() {
  Serial.begin(250000);

  ethernetInitialize();
  asmEventsInitialize();
  rfifoInitialize();
}

//**********************************************************************
//*                             Debug
//**********************************************************************
void debugPrintMatchParams() {
  Serial.println(String("Stage: (") + CurrentGame.stage + String(") ") + String(stages[CurrentGame.stage]));
  for (int i = 0; i < PLAYER_COUNT; i++) {
    Player* p = &CurrentGame.players[i];
    Serial.println(String("Player ") + (char)(65 + i));
    Serial.print("Port: "); Serial.println(p->controllerPort + 1, DEC);
    Serial.println(String("Character: (") + p->characterId + String(") ") + String(externalCharacterNames[p->characterId]));
    Serial.println(String("Color: (") + p->characterColor + String(") ") + String(colors[p->characterColor]));
  }
}

void debugPrintGameInfo() {
  if (CurrentGame.frameCounter % 600 == 0) {
    Serial.println(String("Frame: ") + CurrentGame.frameCounter);
    Serial.println(String("Frames missed: ") + CurrentGame.framesMissed);
    Serial.print("Random seed: "); Serial.println(CurrentGame.randomSeed, HEX);
    for (int i = 0; i < PLAYER_COUNT; i++) {
      Player* p = &CurrentGame.players[i];
      PlayerFrameData* pfd = &p->currentFrameData;
      Serial.println(String("Player ") + (char)(65 + i));
      Serial.print("Location X: "); Serial.println(pfd->locationX);
      Serial.print("Location Y: "); Serial.println(pfd->locationY);
      Serial.print("Stocks: "); Serial.println(pfd->stocks);
      Serial.print("Percent: "); Serial.println(pfd->percent);
    }
  }
}

//**********************************************************************
//*                           Main Loop
//**********************************************************************
void loop() {
  //If ethernet client not working, attempt to re-establish
  if (ethernetCheckConnection()) {
    //read a message from the read fifo - this function doesn't return until data has been read
    rfifoReadMessage();
    
    if (Msg.success) {
      switch (Msg.eventCode) {
        case EVENT_GAME_START:
		  //TODO: Check stock count and number of players here
          handleGameStart();
          debugPrintMatchParams();
          postMatchParameters();
          break;
        case EVENT_UPDATE:
          handleUpdate();
          debugPrintGameInfo();
          computeStatistics();
          break;
        case EVENT_GAME_END:
          if(!CurrentGame.matchReported) {
            handleGameEnd();
            postGameEndMessage();
            CurrentGame.matchReported = true;
          }
          break;
      }
    } else {
      Serial.print("Failed to read message. Bytes read: ");
      Serial.println(Msg.bytesRead);
    }
  }
}


