/*
 * Copyright (c) 2010 by Cristian Maglie <c.maglie@bug.st>
 * SSI3DMASlave Master library for arduino.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#ifndef _SSI3DMASlave_H_INCLUDED
#define _SSI3DMASlave_H_INCLUDED

#include <stdio.h>
#include <Energia.h>

class SSI3DMASlaveClass {

private:

  void configureSSI3(void);
  void configureDMA(void);
  void configureCSInterrupt(void);

public:

  SSI3DMASlaveClass(void);
  void begin(); // Default
  void end();
  bool isMessageAvailable(void);
  uint32_t getMessageSize(void);
  uint8_t* popMessage(void);
  void queueResponse(uint8_t* data, int length);
  
};

extern SSI3DMASlaveClass SSI3DMASlave;

#endif


