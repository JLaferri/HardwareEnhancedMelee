#include "enhmelee.h"
#include "meleeids.h"

//**********************************************************************
//*                         ASM Event Codes
//**********************************************************************
#define EVENT_GAME_START 0x37
#define EVENT_UPDATE 0x38
#define EVENT_GAME_END 0x39

int asmEvents[256];

void asmEventsInitialize() {
  asmEvents[EVENT_GAME_START] = 0xA;
  asmEvents[EVENT_UPDATE] = 0x64;
  asmEvents[EVENT_GAME_END] = 0x0;
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
  
  //Wait until data is available in the queue
  do { rfifoReadPins(); } while(!rfifoDataAvailable());
  
  //There is now data in the queue, the first byte should always be the event code byte, read it
  Msg.eventCode = rfifoPopByte();
  
  //Get the expected message size for this event
  Msg.messageSize = asmEvents[Msg.eventCode];
  if (Msg.messageSize <= 0) return; //If message size for this event is zero, it is not a valid event, return a false success message

  for (int i = 0; i < Msg.messageSize; i++) {
    rfifoReadPins();
    
    //If there is no data available in the queue we can assume message transmission has completed and we missed at least one byte
	  //this works because the GC writes to the queue faster than we can read from it, by the time one byte is read there should be another ready
    if (!rfifoDataAvailable()) return;
    
    //Load byte into buffer
    Msg.data[i] = rfifoPopByte();
	
	  //Keep track of the number of bytes read (this is in case we miss reads we will know how many we did successfully read)
	  Msg.bytesRead = i + 1;
  }
  
  Msg.success = true;
}

//**********************************************************************
//*                         Event Handlers
//**********************************************************************
#define GAME_START_PLAYER_FRAME_BYTES 4;
#define UPDATE_PLAYER_FRAME_BYTES 46

Game CurrentGame;

void handleGameStart(uint8_t data[]) {
  //Reset CurrentGame variable
  CurrentGame = { };

  //Load stage ID
  CurrentGame.stage = *(uint16_t*)(&data[0]);

  for (int i = 0; i < PLAYER_COUNT; i++) {
    Player p = CurrentGame.players[i];
    int offset = i * GAME_START_PLAYER_FRAME_BYTES;

    //Load player data
    p.controllerPort = data[2 + offset];
    p.characterId = data[3 + offset];
    p.playerType = data[4 + offset];
    p.characterColor = data[5 + offset];
  }
}

void handleUpdate(uint8_t data[]) {
  //Check frame count and see if any frames were skipped
  uint32_t frameCount = *(uint32_t*)(&data[0]);
  int framesMissed = frameCount - CurrentGame.frameCounter - 1;
  CurrentGame.framesMissed += framesMissed;
  CurrentGame.frameCounter = frameCount;

  CurrentGame.randomSeed = *(uint32_t*)(&data[4]);

  for (int i = 0; i < PLAYER_COUNT; i++) {
    Player p = CurrentGame.players[i];
    int offset = i * UPDATE_PLAYER_FRAME_BYTES;

    //Change over previous frame data
    p.previousFrameData = p.currentFrameData;
    
    //Load player data
    p.currentFrameData = { };
    p.currentFrameData.internalCharacterId = data[8 + offset];
    p.currentFrameData.animation = *(uint16_t*)(&data[9 + offset]);
    p.currentFrameData.locationX = *(float*)(&data[11 + offset]);
    p.currentFrameData.locationY = *(float*)(&data[15 + offset]);

    //Controller information
    p.currentFrameData.joystickX = *(float*)(&data[19 + offset]);
    p.currentFrameData.joystickY = *(float*)(&data[23 + offset]);
    p.currentFrameData.cstickX = *(float*)(&data[27 + offset]);
    p.currentFrameData.cstickY = *(float*)(&data[31 + offset]);
    p.currentFrameData.trigger = *(float*)(&data[35 + offset]);
    p.currentFrameData.buttons = *(uint32_t*)(&data[39 + offset]);

    //More data
    p.currentFrameData.percent = *(float*)(&data[43 + offset]);
    p.currentFrameData.shieldSize = *(float*)(&data[47 + offset]);
    p.currentFrameData.lastMoveHitId = data[51 + offset];
    p.currentFrameData.comboCount = data[52 + offset];
    p.currentFrameData.lastHitBy = data[53 + offset];
  }
}

void handleGameEnd(uint8_t data[]) {
  //TODO: define
}

//**********************************************************************
//*                             Setup
//**********************************************************************
void setup() {
  Serial.begin(250000);
  
  asmEventsInitialize();
  rfifoInitialize();
}

//**********************************************************************
//*                             Debug
//**********************************************************************
void debugPrintMatchParams() {
  Serial.println("Player A:");
  Serial.print("Port: "); Serial.println(CurrentGame.players[0].controllerPort + 1, DEC);
  Serial.println("Character: " + String(externalCharacterNames[CurrentGame.players[0].characterId]));
}

void debugPrintGameInfo() {
  
}

//**********************************************************************
//*                           Main Loop
//**********************************************************************
void loop() {
  //read a message from the read fifo - this function doesn't return until data has been read
  rfifoReadMessage();
  
  if (Msg.success) {
    switch (Msg.eventCode) {
      case EVENT_GAME_START:
        handleGameStart(Msg.data);
        debugPrintMatchParams();
        break;
      case EVENT_UPDATE:
        handleUpdate(Msg.data);
        debugPrintGameInfo();
        break;
      case EVENT_GAME_END:
        handleGameEnd(Msg.data);
        break;
    }
  }
}
