#include <stdint.h>
#include <stdio.h>
#include "meleeids.h"

#define PLAYER_COUNT 2
#define STOCK_COUNT 4
#define MAX_FRAMES 28800
#define MSG_BUFFER_SIZE 1024

typedef struct {
  //Volatile data - this data will change throughout the course of the match
  uint8_t internalCharacterId;
  uint16_t animation;
  float locationX;
  float locationY;
  uint8_t stocks;
  float percent;
  float shieldSize;
  uint8_t lastMoveHitId;
  uint8_t comboCount;
  uint8_t lastHitBy;
  
  //Controller information
  float joystickX;
  float joystickY;
  float cstickX;
  float cstickY;
  float trigger;
  uint32_t buttons;
  
} PlayerFrameData;

typedef struct {
  //Recovery
  bool isRecovering = false;
  uint8_t framesSinceLanding;
  
  uint32_t framesWithoutDamage;
} PlayerFlags;

typedef struct {
	uint32_t frame;
	float percent;
	uint8_t lastHitBy;
	uint8_t deathDirection;
  bool isStockUsed;
} StockStatistics;

typedef struct {
  //Positional
  uint32_t framesAboveOthers; //Assuming if you are higher, you are in a worse position
  uint32_t framesClosestCenter; //Assuming if you are closer to center, you are controlling the stage
  float averageDistanceFromCenter;
  
  uint32_t framesInShield; //Amount of frames spent shielding
  uint32_t mostFramesWithoutDamage; //Amount of frames without being hit
  
  //Defensive option selection
  uint16_t rollCount;
  uint16_t spotDodgeCount;
  uint16_t airDodgeCount;
  
  //Recovery
  uint16_t recoveryAttempts;
  uint16_t successfulRecoveries;
  
  StockStatistics stocks[STOCK_COUNT];
} PlayerStatistics;

typedef struct {
  //Static data
  uint8_t characterId;
  uint8_t characterColor;
  uint8_t playerType;
  uint8_t controllerPort;

  //Update data
  PlayerFrameData currentFrameData;

  //Used for statistic calculation
  PlayerFrameData previousFrameData;
  PlayerFlags flags;
  PlayerStatistics stats;
} Player;

typedef struct {
  Player players[PLAYER_COUNT]; //Contains all information relevant to individual players
  uint16_t stage; //Stage ID

  //Fromt Update event
  uint32_t frameCounter; //Frame count
  uint32_t framesMissed;
  uint32_t randomSeed;
  
  //From OnGameEnd event
  uint8_t winCondition;

  bool matchReported;
} Game;

typedef struct {
  bool success;
  uint8_t eventCode;
  int messageSize;
  int bytesRead;
  uint8_t data[MSG_BUFFER_SIZE]; //No event should pass more than 1024 bytes
} RfifoMessage;

bool checkIfOffstage(uint16_t stage, float x, float y) {
  switch(stage) {
    case STAGE_FOD:
    case STAGE_POKEMON:
    case STAGE_YOSHIS:
    case STAGE_DREAM_LAND:
    case STAGE_BATTLEFIELD:
    case STAGE_FD:
    default:
      return false;
  }
}
