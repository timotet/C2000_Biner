/*
 * AIO.c
 *
 *  Created on: Dec 30, 2014
 *      Author: Tim
 *
 *      init function for using AIO's as GPIO's on C2000 launchpad
 */

#include "DSP28x_Project.h" // Device Header file and Examples Include File
#include "f2802x_common/include/gpio.h"
#include "AIO.h"

void AIO_init(void) {

	EALLOW;  // protected register write enable

	// pin AIO2 for GPIO booster J1_8 on c2000 LP board
	//GpioCtrlRegs.AIOMUX1.bit.AIO2 = 0;      //set as digital i/o
	//GpioCtrlRegs.AIODIR.bit.AIO2 = 1;       //set direction as output
	//GpioDataRegs.AIODAT.bit.AIO2 = 0;       //set bit low

	// pin AIO4 for GPIO booster J1_6 on c2000 LP board
	//GpioCtrlRegs.AIOMUX1.bit.AIO4 = 0;      //set as digital i/o
	//GpioCtrlRegs.AIODIR.bit.AIO4 = 1;       //set direction as output
	//GpioDataRegs.AIODAT.bit.AIO4 = 0;       //set bit low

	// pin AIO6 for GPIO booster J1_2 on c2000 LP board
	//GpioCtrlRegs.AIOMUX1.bit.AIO6 = 0;      //set as digital i/o
	//GpioCtrlRegs.AIODIR.bit.AIO6 = 1;       //set direction as output
	//GpioDataRegs.AIODAT.bit.AIO6 = 0;       //set bit low

	// pin AIO10 for GPIO booster J1_9 on c2000 LP board
	//GpioCtrlRegs.AIOMUX1.bit.AIO10 = 0;      //set as digital i/o
	//GpioCtrlRegs.AIODIR.bit.AIO10 = 1;       //set direction as output
	//GpioDataRegs.AIODAT.bit.AIO10 = 0;       //set bit low

	// pin AIO12 for GPIO booster J1_10 on c2000 LP board
	//GpioCtrlRegs.AIOMUX1.bit.AIO12 = 0;      //set as digital i/o
	//GpioCtrlRegs.AIODIR.bit.AIO12 = 1;       //set direction as output
	//GpioDataRegs.AIODAT.bit.AIO12 = 0;       //set bit low

	// pin AIO14 for GPIO booster J2_10 on c2000 LP board
	GpioCtrlRegs.AIOMUX1.bit.AIO14 = 0;      //set as digital i/o
	GpioCtrlRegs.AIODIR.bit.AIO14 = 1;       //set direction as output
	GpioDataRegs.AIODAT.bit.AIO14 = 0;       //set bit low

	EDIS;  // protected register write disable

}


