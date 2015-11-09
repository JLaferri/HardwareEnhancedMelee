/*
 * Copyright (c) 2010 by Cristian Maglie <c.maglie@bug.st>
 * SSI3DMASlave Master library for arduino.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#include "wiring_private.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ssi.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/udma.h"
#include "driverlib/ssi.h"
#include "SSI3DMASlave.h"
#include "part.h"

//SPI Clock Speed
#define SPI_CLOCK 8000000

//*****************************************************************************
//
// The size of the memory transfer source and destination buffers (in words).
//
//*****************************************************************************
#define SSI_BUFFER_SIZE       1024
#define SSI_RX_BUFFER_COUNT   5

//*****************************************************************************
//
// The SSI3 Buffers
//
//*****************************************************************************
uint8_t g_ui8SSITxBuf[SSI_BUFFER_SIZE];
uint8_t g_ui8SSIRxBuf[SSI_RX_BUFFER_COUNT][SSI_BUFFER_SIZE];

//*****************************************************************************
//
// The Memory Control Variables
//
//*****************************************************************************
uint32_t g_ui32MessageSizes[SSI_RX_BUFFER_COUNT];
uint8_t g_ui8RxWriteIndex = 0;
uint8_t g_ui8RxReadIndex = 0;

//*****************************************************************************
//
// The count of uDMA errors.  This value is incremented by the uDMA error
// handler.
//
//*****************************************************************************
uint32_t g_ui32uDMAErrCount = 0;

//*****************************************************************************
//
// The count of SSI3 buffers filled/read
//
//*****************************************************************************
volatile uint32_t g_ui32SSIRxWriteCount = 0;
volatile uint32_t g_ui32SSIRxReadCount = 0;
uint32_t g_ui32SSITxCount = 0;

//*****************************************************************************
//
// The control table used by the uDMA controller.  This table must be aligned
// to a 1024 byte boundary.
//
//*****************************************************************************
#if defined(ewarm)
#pragma data_alignment=1024
uint8_t pui8ControlTable[1024];
#elif defined(ccs)
#pragma DATA_ALIGN(pui8ControlTable, 1024)
uint8_t pui8ControlTable[1024];
#else
uint8_t pui8ControlTable[1024] __attribute__ ((aligned(1024)));
#endif

void uDMAErrorHandler(void)
{
    uint32_t ui32Status;

    //
    // Check for uDMA error bit
    //
    ui32Status = ROM_uDMAErrorStatusGet();

    //
    // If there is a uDMA error, then clear the error and increment
    // the error counter.
    //
    if(ui32Status)
    {
        ROM_uDMAErrorStatusClear();
        g_ui32uDMAErrCount++;
    }
}

//*****************************************************************************
//
// CS Rising Edge Interrupt Handler
//
//*****************************************************************************
void gpioQ1IntHandler(void) {
    uint32_t ui32Status;

    ui32Status = ROM_GPIOIntStatus(GPIO_PORTQ_BASE, 1);
    ROM_GPIOIntClear(GPIO_PORTQ_BASE, ui32Status);

    uint32_t xferSize = ROM_uDMAChannelSizeGet(UDMA_CH14_SSI3RX | UDMA_PRI_SELECT);

	//Calculate next RxWriteIndex (buffer in which to write the next set of data)
	uint8_t nextIndex = g_ui8RxWriteIndex + 1;
	if(nextIndex >= SSI_RX_BUFFER_COUNT) nextIndex = 0;

	//Get the size of the completed transfer
	uint32_t msgSize = SSI_BUFFER_SIZE - xferSize;
	g_ui32MessageSizes[g_ui8RxWriteIndex] = msgSize;

	//Store index value of this transaction for print debug message
	uint8_t previousIndex = g_ui8RxWriteIndex;

	//Move current rx write index to the next buffer
	g_ui8RxWriteIndex = nextIndex;

	ROM_uDMAChannelDisable(UDMA_CH14_SSI3RX);

	//Enable DMA channel to write in the next buffer position
	ROM_uDMAChannelTransferSet(UDMA_CH14_SSI3RX | UDMA_PRI_SELECT,
							   UDMA_MODE_BASIC,
							   (void *)(SSI3_BASE + SSI_O_DR),
							   g_ui8SSIRxBuf[g_ui8RxWriteIndex], sizeof(g_ui8SSIRxBuf[g_ui8RxWriteIndex]));

	ROM_uDMAChannelEnable(UDMA_CH14_SSI3RX);

	//Increment receive count
	g_ui32SSIRxWriteCount++;

	//UARTprintf("%d\t%d\t%x\t%x\n", previousIndex, msgSize, g_ui8SSIRxBuf[previousIndex][0], g_ui8SSIRxBuf[previousIndex][msgSize - 1]);
}

SSI3DMASlaveClass::SSI3DMASlaveClass(void) {
  
}

void SSI3DMASlaveClass::configureSSI3() {
  //
  // Enable the SSI3 Peripheral.
  //
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI3);
  ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_SSI3);
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);

  // Configure GPIO Pins for SSI3 mode.
  //
  ROM_GPIOPinConfigure(GPIO_PQ0_SSI3CLK);
  ROM_GPIOPinConfigure(GPIO_PQ1_SSI3FSS);
  ROM_GPIOPinConfigure(GPIO_PQ2_SSI3XDAT0);
  ROM_GPIOPinConfigure(GPIO_PQ3_SSI3XDAT1);
  ROM_GPIOPinTypeSSI(GPIO_PORTQ_BASE, GPIO_PIN_3 | GPIO_PIN_2 | GPIO_PIN_1 | GPIO_PIN_0);

  ROM_SSIConfigSetExpClk(SSI3_BASE, F_CPU, SSI_FRF_MOTO_MODE_0,
          SSI_MODE_SLAVE, SPI_CLOCK / 12, 8);

  ROM_SSIEnable(SSI3_BASE);
  ROM_SSIDMAEnable(SSI3_BASE, SSI_DMA_RX | SSI_DMA_TX);
}

void SSI3DMASlaveClass::configureDMA() {
  //Enable uDMA
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
  ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_UDMA);

  //Register DMA interrupt to handler
  IntRegister(INT_UDMAERR, uDMAErrorHandler);
  
  //Enable interrupt
  ROM_IntEnable(INT_UDMAERR);

  // Enable the uDMA controller.
  ROM_uDMAEnable();

  // Point at the control table to use for channel control structures.
  ROM_uDMAControlBaseSet(pui8ControlTable);

  //Assign DMA channels to SSI3
  ROM_uDMAChannelAssign(UDMA_CH14_SSI3RX);
  ROM_uDMAChannelAssign(UDMA_CH15_SSI3TX);

  // Put the attributes in a known state for the uDMA SSI0RX channel.  These
  // should already be disabled by default.
  ROM_uDMAChannelAttributeDisable(UDMA_CH14_SSI3RX, UDMA_ATTR_USEBURST | UDMA_ATTR_ALTSELECT |
    (UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK));

  // Configure the control parameters for the primary control structure for
  // the SSIORX channel.
  ROM_uDMAChannelControlSet(UDMA_CH14_SSI3RX | UDMA_PRI_SELECT,
                            UDMA_SIZE_8 | UDMA_SRC_INC_NONE | UDMA_DST_INC_8 |
                            UDMA_ARB_4);


//Enable DMA channel to write in the next buffer position
ROM_uDMAChannelTransferSet(UDMA_CH14_SSI3RX | UDMA_PRI_SELECT,
               UDMA_MODE_BASIC,
               (void *)(SSI3_BASE + SSI_O_DR),
               g_ui8SSIRxBuf[g_ui8RxWriteIndex], sizeof(g_ui8SSIRxBuf[g_ui8RxWriteIndex]));

// Configure TX

//
// Put the attributes in a known state for the uDMA SSI0TX channel.  These
// should already be disabled by default.
//
ROM_uDMAChannelAttributeDisable(UDMA_CH15_SSI3TX,
                UDMA_ATTR_ALTSELECT |
                UDMA_ATTR_HIGH_PRIORITY |
                UDMA_ATTR_REQMASK);
//
//	//
//	// Set the USEBURST attribute for the uDMA SSI0TX channel.  This will
//	// force the controller to always use a burst when transferring data from
//	// the TX buffer to the SSI0.  This is somewhat more effecient bus usage
//	// than the default which allows single or burst transfers.
//	//
//	//ROM_uDMAChannelAttributeEnable(UDMA_CH15_SSI3TX, UDMA_ATTR_USEBURST);
//
//	//
//	// Configure the control parameters for the primary control structure for
//	// the SSIORX channel.
//	//
//	ROM_uDMAChannelControlSet(UDMA_CH15_SSI3TX | UDMA_PRI_SELECT,
//							  UDMA_SIZE_8 | UDMA_SRC_INC_NONE | UDMA_DST_INC_NONE |
//							  UDMA_ARB_4);
//
//	//Configure TX to always write 0xFF?
//	g_ui8SSITxBuf[0] = 0xFF;
//
//	//Enable DMA channel to write in the next buffer position
//	ROM_uDMAChannelTransferSet(UDMA_CH15_SSI3TX | UDMA_PRI_SELECT,
//							   UDMA_MODE_BASIC, g_ui8SSITxBuf,
//							   (void *)(SSI3_BASE + SSI_O_DR),
//							   sizeof(g_ui8SSITxBuf));

ROM_uDMAChannelEnable(UDMA_CH14_SSI3RX);
//	ROM_uDMAChannelEnable(UDMA_CH15_SSI3TX);

//	//
//	// Enable the SSI3 DMA TX/RX interrupts.
//	//
//	ROM_SSIIntEnable(SSI3_BASE, SSI_DMARX);
//
//	//
//	// Enable the SSI3 peripheral interrupts.
//	//
//	ROM_IntEnable(INT_SSI3);
}

void SSI3DMASlaveClass::configureCSInterrupt() {
	IntRegister(INT_GPIOQ1, gpioQ1IntHandler);
	ROM_GPIOIntTypeSet(GPIO_PORTQ_BASE, GPIO_PIN_1, GPIO_RISING_EDGE | GPIO_DISCRETE_INT);
	ROM_GPIOIntEnable(GPIO_PORTQ_BASE, GPIO_INT_PIN_1);
	ROM_IntEnable(INT_GPIOQ1);
}

void SSI3DMASlaveClass::begin() {
  ROM_SysCtlPeripheralClockGating(true);
  
  configureSSI3();
  configureDMA();
  configureCSInterrupt();
}

void SSI3DMASlaveClass::end() {
	ROM_SSIDisable(SSI3_BASE);
}

bool SSI3DMASlaveClass::isMessageAvailable() {
	return g_ui32SSIRxWriteCount > g_ui32SSIRxReadCount;
}

uint32_t SSI3DMASlaveClass::getMessageSize() {
	return g_ui32MessageSizes[g_ui8RxReadIndex];
}

uint8_t* SSI3DMASlaveClass::popMessage() {
	uint8_t readIndex = g_ui8RxReadIndex;

	//Increment read index, keep between 0 and buffer count
	g_ui8RxReadIndex++;
	if(g_ui8RxReadIndex >= SSI_RX_BUFFER_COUNT) g_ui8RxReadIndex = 0;

	g_ui32SSIRxReadCount++; //Increment read count

	//Return pointer to array containing the message to read
	return g_ui8SSIRxBuf[readIndex];
}

void SSI3DMASlaveClass::queueResponse(uint8_t* data, int length) {
  
} 

SSI3DMASlaveClass SSI3DMASlave;
