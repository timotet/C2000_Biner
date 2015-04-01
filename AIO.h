/*
 * AIO.h
 *
 *  Created on: Dec 29, 2012
 *      Author: Timotet
 */

#ifndef AIO_H_
#define AIO_H_

// AIO pin defines

/*
// ADCINA2 = AIO2     pin J1_8 on C2000 launchpad
// set AIO2 low
#define AIO2_LOW     GpioDataRegs.AIOCLEAR.bit.AIO2 = 0
// set AIO2 high
#define AIO2_HIGH    GpioDataRegs.AIOSET.bit.AIO2 = 1
// toggle AIO2
#define AIO2_toggle  GpioDataRegs.AIOTOGGLE.bit.AIO2 = 1

// Dont use this is used by adc for current sense on tne VNH5091AE
// ADCINA4 = AIO4      pin J1_6 on C2000 launchpad
// set AIO4 low
#define AIO4_LOW     GpioDataRegs.AIOCLEAR.bit.AIO4 = 0
// set AIO4 high
#define AIO4_HIGH    GpioDataRegs.AIOSET.bit.AIO4 = 1
// toggle AIO4
#define AIO4_toggle  GpioDataRegs.AIOTOGGLE.bit.AIO4 = 1

// ADCINA6 = AIO6     pin J1_2 on C2000 launchpad
// set AIO6 low
#define AIO6_LOW     GpioDataRegs.AIOCLEAR.bit.AIO6 = 1
// set AIO6 high
#define AIO6_HIGH    GpioDataRegs.AIOSET.bit.AIO6 = 1
// toggle AIO6
#define AIO6_toggle  GpioDataRegs.AIOTOGGLE.bit.AIO6 = 1

// ADCINB2 = AI010    pin J1_9 on C2000 launchpad
// set AIO10 low
#define AIO10_LOW     GpioDataRegs.AIOCLEAR.bit.AIO10 = 1
// set AIO10 high
#define AIO10_HIGH    GpioDataRegs.AIOSET.bit.AIO10 = 1
// toggle AIO10
#define AIO10_toggle  GpioDataRegs.AIOTOGGLE.bit.AIO10 = 1

// ADCINB4 = AI012    pin J1_10 on C2000 launchpad
// set AIO12 low
#define AIO12_LOW     GpioDataRegs.AIOCLEAR.bit.AIO12 = 1
// set AIO12 high
#define AIO12_HIGH    GpioDataRegs.AIOSET.bit.AIO12 = 1
// toggle AIO12
#define AIO12_toggle  GpioDataRegs.AIOTOGGLE.bit.AIO12 =
*/
// ADCINB6 = AI014    pin J2_10 on C2000 launchpad
// set AIO14 low
#define AIO14_LOW     GpioDataRegs.AIOCLEAR.bit.AIO14 = 1
// set AIO14 high
#define AIO14_HIGH    GpioDataRegs.AIOSET.bit.AIO14 = 1
// toggle AIO14
#define AIO14_toggle  GpioDataRegs.AIOTOGGLE.bit.AIO14 = 1

// function prototypes
void AIO_init(void);


#endif /* AIO_H_ */
