#include <stdint.h>
#include <stdio.h>
#include "meleeids.h"

#define PLAYER_COUNT 2
#define STOCK_COUNT 4
#define MAX_FRAMES 28800
#define MSG_BUFFER_SIZE 1024

#define JOYSTICK_NE 1
#define JOYSTICK_SE 2
#define JOYSTICK_SW 3
#define JOYSTICK_NW 4
#define JOYSTICK_N 5
#define JOYSTICK_E 6
#define JOYSTICK_S 7
#define JOYSTICK_W 8
#define JOYSTICK_DZ 9

//For statistics
#define FRAMES_LANDED_RECOVERY 60
#define COMBO_STRING_TIMEOUT 60

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
  uint32_t buttons; //This will include multiple "buttons" pressed on special buttons. For example I think pressing z sets 3 bits

  //This is extra controller information
  uint16_t physicalButtons; //A better representation of what a player is actually pressing
  float lTrigger;
  float rTrigger;
} PlayerFrameData;

typedef struct {
  //Recovery
  bool isRecovering = false;
  bool isHitOffStage = false;
  bool isLandedOnStage = false;
  uint8_t framesSinceLanding;
  
  //Combo String
  float stringStartPercent = 0;
  uint32_t stringStartFrame = 0;
  uint16_t stringCount = 0;
  uint8_t stringResetCounter = 0;
  
  uint32_t framesWithoutDamage;
} PlayerFlags;

void resetRecoveryFlags(PlayerFlags& flags) {
  flags.isRecovering = false;
  flags.isHitOffStage = false;
  flags.isLandedOnStage = false;
  flags.framesSinceLanding = 0;
}

typedef struct {
	uint32_t frame;
	float percent;
	uint8_t lastHitBy;
	uint16_t lastAnimation;
  //uint16_t deathAnimation; //to implement
  bool isStockUsed;
  bool isStockLost;
  
  //Combo String
  uint16_t killedInOpenings;
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
  uint16_t edgeguardChances;
  uint16_t edgeguardConversions;
  
  //APM
  uint16_t actionCount;
  
  //Combo Strings
  float mostDamageString;
  uint32_t mostTimeString; //longest amount of frame for a combo string
  uint16_t mostHitsString; //most amount of hits in a combo string
  uint16_t numberOfOpenings; //this is the number of time a player started a combo string
  float averageDamagePerString;
  float averageTimePerString;
  float averageHitsPerString;
  
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
} Game;

typedef struct {
  bool success;
  uint8_t eventCode;
  int messageSize;
  int bytesRead;
  uint8_t data[MSG_BUFFER_SIZE]; //No event should pass more than 1024 bytes
} RfifoMessage;

bool checkIfOffStage(uint16_t stage, float x, float y) {
  switch(stage) {
    case STAGE_FOD:
      return x < -63.35 || x > 63.35 || y < 0;
    case STAGE_POKEMON:
      return x < -87.75 || x > 87.75 || y < 0;
    case STAGE_YOSHIS:
      return x < -56 || x > 56 || y < 0;
    case STAGE_DREAM_LAND:
      return x < -77.27 || x > 77.27 || y < 0;
    case STAGE_BATTLEFIELD:
      return x < -68.4 || x > 68.4 || y < 0;
    case STAGE_FD:
      return x < -85.5606 || x > 85.5606 || y < 0;
    default:
      return false;
  }
}

//Return joystick region
uint8_t getJoystickRegion(uint32_t x, uint32_t y) {
  if(x >= 0.2875 & y >= 0.2875) return JOYSTICK_NE;
  else if(x >= 0.2875 & y <= -0.2875) return JOYSTICK_SE; 
  else if(x <= -0.2875 & y <= -0.2875) return JOYSTICK_SW;
  else if(x <= -0.2875 & y >= 0.2875) return JOYSTICK_NW;
  else if(y >= 0.2875) return JOYSTICK_N;
  else if(x >= 0.2875) return JOYSTICK_E;
  else if(y <= -0.2875) return JOYSTICK_S;
  else if(x <= -0.2875) return JOYSTICK_W;
  else return JOYSTICK_DZ;
}
