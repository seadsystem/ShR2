/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Pavlo Milo Manovi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/**
 * @file	
 * @author 	Pavlo Milo Manovi
 * @date	April, 2014
 * @brief 	This library provides implementation of methods to init and read the MCP391x.
 *
 */

#include "MCP391x.h"
#include "ADCModuleBoard.h"
#include "ParallelIO.h"
#include <stdint.h>
#include <stdlib.h>
#include <plib.h>
#include "DMA_Transfer.h"

void SPI_Write_Register(uint8_t registerAddress, uint32_t registerData);
void SPI_Read_Register(uint8_t registerAddress);

static MCP391x_Info *passedInfoStruct;

/**
 * @brief Sets up the MCP391x.
 * @param MCP391x_Params A pointer to a MCP391x_Info struct which will be updated.
 * @return Returns EXIT_SUCCESS if the device responds with the set configuration.
 */
uint8_t MCP391x_Init(MCP391x_Info *MCP391xInfo)
{
	MCP391x_Info MCP;

	//Setting bits for configuration of the MCP3912;
	MCP.config0Reg.BOOST = 0b11;
	MCP.config0Reg.DITHER = 0b11;
	MCP.config0Reg.EN_GAINCAL = 0;
	MCP.config0Reg.EN_OFFCAL = 0;
	MCP.config0Reg.OSR = 0b010;
	MCP.config0Reg.PRE = 0b10;
	MCP.config0Reg.VREFCAL = 64;

	MCP.config1Reg.CLKEXT = 1; //This should be set to one for oscillator :: TESTING
	MCP.config1Reg.RESET = 0b0000;
	MCP.config1Reg.SHUTDOWN = 0b0000;
	MCP.config1Reg.VREFEXT = 0;

	MCP.phaseReg.PHASEA = 0;
	MCP.phaseReg.PHASEB = 0;

	MCP.statusReg.WIDTH_DATA = 0b00;
	MCP.statusReg.DR_HIZ = 0;
	MCP.statusReg.DR_LINK = 1;
	MCP.statusReg.EN_CRCCOM = 0;
	MCP.statusReg.EN_INT = 0;
	MCP.statusReg.READ = 0b10;
	MCP.statusReg.WIDTH_CRC = 0;
	MCP.statusReg.WRITE = 0;

	//Testing Reading from the device.

	SPI_Read_Register(CONFIG0_ADDR);
	SPI_Read_Register(CONFIG1_ADDR);
	SPI_Read_Register(PHASE_ADDR);
	SPI_Read_Register(STATUSCOM_ADDR);


	//These calls are using DMA, but they'll be blocking to make sure this
	//happens properly on startup.  TODO: Implement initialization as a
	//single DMA transfer.  This will save up to 20 uS on initialization.

	SPI_Write_Register(CONFIG0_ADDR, MCP.config0Reg.wholeRegister);
	SPI_Write_Register(CONFIG1_ADDR, MCP.config1Reg.wholeRegister);
	SPI_Write_Register(PHASE_ADDR, MCP.phaseReg.wholeRegister);
	SPI_Write_Register(STATUSCOM_ADDR, MCP.statusReg.wholeRegister);

	//Testing Reading from the device.

	SPI_Read_Register(CONFIG0_ADDR);
	SPI_Read_Register(CONFIG1_ADDR);
	SPI_Read_Register(PHASE_ADDR);
	SPI_Read_Register(STATUSCOM_ADDR);

	passedInfoStruct = MCP391xInfo;
	*passedInfoStruct = MCP;

	return(EXIT_SUCCESS);
}

/****************************** PRIVATE FCNS **********************************/

/**
 * The MCP3912 has a read/write sequence which is 32 bits long.  The first 8
 * bits make up the control byte with form:
 *
 * - A 7:6 Device Address:	Default 01
 * - A 5:1 Register Address:	See XXXXX_ADDR defines
 * - A 0 Read/Write		0 = Write, 1 = Read
 *
 * @param registerAddress
 * @param registerData
 */
void SPI_Write_Register(uint8_t registerAddress, uint32_t registerData)
{
	static uint8_t txBuf[4];
	//Toggle SS pin.
	int i;
	SPI_SS_LAT = 0;

	txBuf[0] = 0x40 | (registerAddress << 1);
	txBuf[0] &= ~(1 << 0); //Clear R/W* bit for Write operation.
	txBuf[1] = (registerData & 0x000000FF);
	txBuf[2] = (registerData & 0x0000FF00) >> 8;
	txBuf[3] = (registerData & 0x00FF0000) >> 16;


	DmaChnClrEvFlags(DMA_CHANNEL1, DMA_EV_BLOCK_DONE);
	BufferToSpi_Transfer(txBuf, 4);


	while (!(DmaChnGetEvFlags(DMA_CHANNEL1) & DMA_EV_BLOCK_DONE)
		|| !(SpiChnTxBuffEmpty(SPI_CHANNEL1)));
	SPI_SS_LAT = 1;
}

void SPI_Read_Register(uint8_t registerAddress)
{
	static uint8_t txBuf[5];
	//Toggle SS pin.
	int i;
	SPI_SS_LAT = 0;

	txBuf[0] = 0x40 | (registerAddress << 1);
	txBuf[0] |= 1 << 0; //Clear R/W* bit for Write operation.
	txBuf[1] = 0;
	txBuf[2] = 0;
	txBuf[3] = 0;

	DmaChnClrEvFlags(DMA_CHANNEL1, DMA_EV_BLOCK_DONE);
	BufferToSpi_Transfer(txBuf, 4);


	while (!(DmaChnGetEvFlags(DMA_CHANNEL1) & DMA_EV_BLOCK_DONE)
		|| !(SpiChnTxBuffEmpty(SPI_CHANNEL1)));
	SPI_SS_LAT = 1;
}