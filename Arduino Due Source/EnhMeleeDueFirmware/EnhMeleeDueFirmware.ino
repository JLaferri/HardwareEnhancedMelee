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
#define UPDATE_PLAYER_FRAME_BYTES 46

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
    Player p = CurrentGame.players[i];
    int offset = i * UPDATE_PLAYER_FRAME_BYTES;

    //Change over previous frame data
    p.previousFrameData = p.currentFrameData;
    
    //Load player data
    p.currentFrameData = { };
    p.currentFrameData.internalCharacterId = data[8 + offset];
    p.currentFrameData.animation = readHalf(&data[9 + offset]);
    p.currentFrameData.locationX = readFloat(&data[11 + offset]);
    p.currentFrameData.locationY = readFloat(&data[15 + offset]);

    //Controller information
    p.currentFrameData.joystickX = readFloat(&data[19 + offset]);
    p.currentFrameData.joystickY = readFloat(&data[23 + offset]);
    p.currentFrameData.cstickX = readFloat(&data[27 + offset]);
    p.currentFrameData.cstickY = readFloat(&data[31 + offset]);
    p.currentFrameData.trigger = readFloat(&data[35 + offset]);
    p.currentFrameData.buttons = readWord(&data[39 + offset]);

    //More data
    p.currentFrameData.percent = readFloat(&data[43 + offset]);
    p.currentFrameData.shieldSize = readFloat(&data[47 + offset]);
    p.currentFrameData.lastMoveHitId = data[51 + offset];
    p.currentFrameData.comboCount = data[52 + offset];
    p.currentFrameData.lastHitBy = data[53 + offset];
  }
}

void handleGameEnd() {
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
  Serial.println(String("Stage: (") + CurrentGame.stage + String(") ") + String(stages[CurrentGame.stage]));
  for (int i = 0; i < PLAYER_COUNT; i++) {
    Serial.println(String("Player ") + (char)(65 + i));
    Serial.print("Port: "); Serial.println(CurrentGame.players[i].controllerPort + 1, DEC);
    Serial.println(String("Character: (") + CurrentGame.players[i].characterId + String(") ") + String(externalCharacterNames[CurrentGame.players[i].characterId]));
    Serial.println(String("Color: (") + CurrentGame.players[i].characterColor + String(") ") + String(colors[CurrentGame.players[i].characterColor]));
  }
}

void debugPrintGameInfo() {
  if (CurrentGame.frameCounter % 600 == 0) {
    Serial.println(String("Frame: ") + CurrentGame.frameCounter);
    Serial.println(String("Frames missed: ") + CurrentGame.framesMissed);
  }
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
        handleGameStart();
        debugPrintMatchParams();
        break;
      case EVENT_UPDATE:
        handleUpdate();
        debugPrintGameInfo();
        break;
      case EVENT_GAME_END:
        handleGameEnd();
        break;
    }
  } else {
    Serial.print("Failed to read message. Bytes read: ");
    Serial.println(Msg.bytesRead);
  }
}

