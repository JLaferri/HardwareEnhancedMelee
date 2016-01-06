#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_flash.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"

void eraseFlash() {
  ROM_FlashErase(0);
  ROM_SysCtlReset();
}

void loadMacAddress(byte* mac) {
  uint32_t userReg0 = HWREG(FLASH_USERREG0);
  uint32_t userReg1 = HWREG(FLASH_USERREG1);
  
  mac[0] = userReg0 & 0xFF;
  userReg0 = userReg0 >> 8;
  mac[1] = userReg0 & 0xFF;
  userReg0 = userReg0 >> 8;
  mac[2] = userReg0 & 0xFF;
  
  mac[3] = userReg1 & 0xFF;
  userReg1 = userReg1 >> 8;
  mac[4] = userReg1 & 0xFF;
  userReg1 = userReg1 >> 8;
  mac[5] = userReg1 & 0xFF;
}


