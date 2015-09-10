#define PLAYER_COUNT 2
#define MAX_FRAMES 28800

typedef struct {
  uint8_t code;
  int byteCount;
} AsmEvent;

AsmEvent eventGameStart = { 0x0, 50 };
AsmEvent eventUpdate = { 0x1, 50 };
AsmEvent eventGameEnd = { 0x2, 50 };

typedef struct {
  uint8_t buttons1;
  uint8_t buttons2;
  uint8_t joystickx;
  uint8_t joysticky;
  uint8_t cstickx;
  uint8_t csticky;
  uint8_t ltrigger;
  uint8_t rtrigger;
} Inputs;

typedef PlayerFrameData {
  //Volatile data - this data will change throughout the course of the match
  Inputs inputs;
  uint16_t animation;
  uint16_t locationX;
  uint16_t locationY;
}

typedef struct {
  //Static data
  uint8_t characterId;
  uint8_t characterColor;
  uint8_t playerType;
  uint8_t controllerPort;
  
} Player;

typedef struct {
  Player players[PLAYER_COUNT]; //Contains all information relevant to individual players
  uint16_t stage; //Stage ID
  
  //From OnGameEnd event
  uint8_t winCondition;
} Game;

//**********************************************************************
//*                         Global Variables
//**********************************************************************
Game* CurrentGame;

void setup() {
  Serial.begin(250000);
}

//**********************************************************************
//*                          SPI Functions
//**********************************************************************
bool spiDataAvailable() {
  //TODO: define
}

uint8_t spiReadByte() {
  //TODO: define
}

uint8_t[] spiReadBytes(int count) {
  //TODO: define
}

//**********************************************************************
//*                         Event Handlers
//**********************************************************************
Game* handleGameStart(uint8_t[] data) {
  //TODO: define
}

void handleUpdate(uint8_t[] data) {
  //TODO: define
}

void handleGameEnd(uint8_t[] data) {
  //TODO: define
}

void loop() {
  //Wait for SPI data
  if (spiDataAvailable()) {
    uint8_t event = spiReadByte(); //Read command byte

    //Check the command type and respond accordingly
    switch (event) {
    case eventGameStart.code:
      uint8_t data[] = spiReadBytes(eventGameStart.byteCount);
      CurrentGame = handleGameStart(data);
      break;
    case eventUpdate.code:
      uint8_t data[] = spiReadBytes(eventUpdate.byteCount);
      handleUpdate(data);
      break;
    case eventGameEnd.code:
      uint8_t data[] = spiReadBytes(eventGameEnd.byteCount);
      handleGameEnd(data);
      break;
    }
  }
}
