/*
 * main.c
 *
 * This is programmed for C2000 launchpad (TMS320F28027)
 * Also uses a nokia 5110 LCD
 *
 * 7/13/13
 * ADC1 to read 6 buttons in series with resistor ladder
 * resistor values r1 = 1k , r2 , r3, r4 , r5, r6 = 2.2k
 * display value, button #, ect. on nokia 5110
 * button is attched at J5_5 (ADCINA1)
 *
 * 9/27/13
 * added pot on ADCINA3 for variable PWM output
 * pot is attached to pin j5_4 (ADCINA3)
 * removed ADC3 pot is too noisy
 *
 * 12/18/13
 * added truth table and gpio for motor control VNH5091A-E
 *
 * 8/27/14
 * added encoder for PWM adjustment maybe? instead of pot
 *
 * 8/29/14
 * added timer with interrupt for cycling motor
 *
 * 12/20/14
 * removed ADC3 and pot unnecessary code
 * cleaned up encoder code
 * added switch case for PWM adjustment
 *
 * 4/4/15
 * added uart for logging results on a pc
 *
 * 4/6/15
 * readded ADC3 for monitoring relay from w127 load cell
 * added jog function
 * added cycle function
 * added change direction function
 *
 * 4/11/15
 * added ChaN's xprintf for easy uart AWESOME!! thanks ChaN!
 * added support for mode select on ADC1 button 1 --not good --rework
 *
 * 5/21/15
 * Timer interrupt code is now working during cycle return function
 * ADC buttons bounce too much!
 * todo add switch for mode select instead of button, use ADC6 to read switch
 */


#include "DSP28x_Project.h" // Device Header file and Examples Include File

#include <math.h>
#include <stdbool.h>

#include "f2802x_common/include/adc.h"
#include "f2802x_common/include/clk.h"
#include "f2802x_common/include/flash.h"
#include "f2802x_common/include/gpio.h"
#include "f2802x_common/include/pie.h"
#include "f2802x_common/include/pll.h"
#include "f2802x_common/include/wdog.h"
#include "f2802x_common/include/spi.h"
#include "f2802x_common/include/pwm.h"
#include "f2802x_common/include/timer.h"
#include "f2802x_common/include/sci.h"

// AIO library
#include "AIO.h"

// nokia5110 libraries
#include "nokia5110.h"

// for easy uart thanks ChaN!!
#include "xprintf.h"

//GPIO defines for  VNH5019A-E motor controller chip
#define VNH5019AE_inA               GPIO_Number_1
#define VNH5019AE_inA_Hi            GPIO_setHigh(myGpio, VNH5019AE_inA);
#define VNH5019AE_inA_Lo            GPIO_setLow(myGpio, VNH5019AE_inA);
#define VNH5019AE_inA_Toggle        GPIO_toggle(myGpio, VNH5019AE_inA);

#define VNH5019AE_inB               GPIO_Number_2
#define VNH5019AE_inB_Hi            GPIO_setHigh(myGpio, VNH5019AE_inB);
#define VNH5019AE_inB_Lo            GPIO_setLow(myGpio, VNH5019AE_inB);
#define VNH5019AE_inB_Toggle        GPIO_toggle(myGpio, VNH5019AE_inB);

#define VNH5019AE_DiagA             GPIO_Number_3
#define VNH5019AE_DiagA_Hi          GPIO_setHigh(myGpio, VNH5019AE_DiagA);
#define VNH5019AE_DiagA_Lo          GPIO_setLow(myGpio, VNH5019AE_DiagA);

#define VNH5019AE_DiagB             GPIO_Number_4
#define VNH5019AE_DiagB_Hi          GPIO_setHigh(myGpio, VNH5019AE_DiagB);
#define VNH5019AE_DiagB_Lo          GPIO_setLow(myGpio, VNH5019AE_DiagB);

// For figuring the PWM frequency
#define TPWM 0.133

// encoder stuff
#define enc1       GPIO_Number_6  // connect to J2_8 on launchpad
#define enc1High   GPIO_setHigh(myGpio, enc1);
#define enc1Low    GPIO_setLow(myGpio, enc1);

#define enc2       GPIO_Number_5    // connect to J6_6 on launchpad
#define enc2High   GPIO_setHigh(myGpio, enc2);
#define enc2Low    GPIO_setLow(myGpio, enc2);

//#define checkEnc1   GpioDataRegs.GPADAT.bit.enc1;    // data register for enc1 bit 6 0x6
//#define checkEnc2   GpioDataRegs.GPADAT.bit.enc2;    // data register for enc2 bit 5 0x5
#define portA       GpioDataRegs.GPADAT.all

#define debugLed        GPIO_Number_12    // connect to J2_3 on launchpad
#define debugLedHigh    GPIO_setHigh(myGpio, debugLed);
#define debugLedLow     GPIO_setLow(myGpio, debugLed);
#define toggleDebugLed  GPIO_toggle(myGpio, debugLed);

#define pin32           GPIO_Number_32    // connect to J6_7 on launchpad
#define pin32High       GPIO_setHigh(myGpio, pin32);
#define pin32Low        GPIO_setLow(myGpio, pin32);
#define pin32Toggle     GPIO_toggle(myGpio, pin32);
/*
#define pin33           GPIO_Number_33    // connect to J6_8 on launchpad
#define pin33High       GPIO_setHigh(myGpio, pin33);
#define pin33Low        GPIO_setLow(myGpio, pin33);
#define pin33Toggle     GPIO_toggle(myGpio, pin33);
*/

// Configure the period for timer
#define EPWM1_TIMER_TBPRD_MAX  64500           // Period register
#define EPWM1_TIMER_TBPRD_MIN  190
#define EPWM1_MAX_CMPA         32600           // compare period
#define EPWM1_MIN_CMPA         95              // compare period
#define EPWM1_START_TBPRD      3760           // 1Khz
#define EPWM1_START_CMPA       1880           // 1Khz

/// pwm stuff
typedef struct{
    PWM_Handle myPwmHandle;
    uint16_t EPwmMaxCMPA;
    uint16_t EPwmMinCMPA;
    uint16_t EPwm1_CMPA;
   // uint16_t EPwm1_CMPB;
    uint16_t EPwm1_TBPRD;
}EPWM_INFO;

EPWM_INFO epwm1_info;

/////////// Global variables //////////////////////////
int16_t pressCount = 0;               // for button presses to toggle mode
int16_t volts;                        // converted adc voltage
char voltsToSend[8];
char adcAsString[4];
char freqToSend[6];
char stringToSend[4];
int16_t ADC3[7];                      // raw ADCINA3 reading take 7 readings and do an average of
int16_t ADC1[7];                      // raw ADCINA1 reading take 7 readings and do an average of
int16_t ADC4[7];                      // raw ADCINA4 reading take 7 readings and do an average of
int16_t ADC3_average = 0;             // averaged ADC data for relay from W127 unit
int16_t ADC1_average = 0;             // averaged ADC data for buttons
int16_t ADC4_average = 0;             // averaged ADC data for current sense on VNH5091AE
int16_t ADC3_value = 0;
int16_t ADC1_value = 0;
int16_t ADC4_value = 0;
volatile int16_t c = 0;              // global counter for adc isr's taken place
volatile uint16_t secCounter = 0;    // second counter for end of cycle actuator return
bool jogFlag = false;                // flag for jogging the actuator
bool cycleFlag = false;              // flag for cycling actuator

float pwmFreq = 0;                   // for figuring the PWM frequency

/// Encoder stuff ///
char encCntToSend[2];  //for encoder count on lcd
const int16_t table[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};  // increment of angle for all possible bit codes
//bool flag = false;
int16_t encPos = 0;
int16_t angle = 0;
int16_t oldAngle = 0;
uint16_t encUpdate(int16_t value);

/// for uart ///
//char * msg;
bool transmit = true;     // start off true to transmit beginning data

/// actator stuff ///
bool CW = true;        // default to CW
enum Mode{
	jog,cycle,stop
};
enum Mode mode = stop;  // default stopped on startup

//// for timer ///
//uint32_t  timerPeriod = 0;    // default to 1 sec interrupt

/// interrupts ///
interrupt void spiRxFifoIsr(void);
interrupt void adc_Isr(void);
interrupt void xint1_isr(void);  //// these are for the encoder
interrupt void xint2_isr(void);
interrupt void timer0_isr(void);
interrupt void timer1_isr(void);
//interrupt void epwm1_isr(void);

/// function prototypes ///
void gpio_init(void);
void spi_init(void);
void adc_init(void);
void epwm_init(void);
void timer0_init(void);
void scia_init(void);
void scia_xmit(char a);
//void scia_msg(char * msg);
void itoa(unsigned int val, char *str);
void ftoa(float f, char *buf, int decPlaces);
int round(float);
float figureFreq(uint16_t TBPRD);
long map(long x, long in_min, long in_max, long out_min, long out_max);
void update_compare(EPWM_INFO*);

/// functions for VNH5091AE ///
void VNH5091AE_changeDir(void);
void VNH5091AE_CCW(void);
void VNH5091AE_CW(void);
void VNH5091AE_STOP(void);
void VNH5091AE_CYCLE(void);
void VNH5091AE_JOG(void);
void VNH5091AE_BRAKE_TO_VCC(void);
void VNH5091AE_BRAKE_TO_GND(void);
void VNH5091AE_FAULT_FIX(void);
void VNH5091AE_RETURN(void);

ADC_Handle myAdc;
CLK_Handle myClk;
FLASH_Handle myFlash;
GPIO_Handle myGpio;
PIE_Handle myPie;
SPI_Handle mySpi;
CPU_Handle myCpu;
PLL_Handle myPll;
WDOG_Handle myWDog;
PWM_Handle myPwm1;
TIMER_Handle myTimer;   //, myTimer1;
SCI_Handle mySci;

void main(void) {

    // Initialize all the handles needed for this application
	myPwm1 = PWM_init((void *)PWM_ePWM1_BASE_ADDR, sizeof(PWM_Obj));
    myAdc = ADC_init((void *)ADC_BASE_ADDR, sizeof(ADC_Obj));
    myClk = CLK_init((void *)CLK_BASE_ADDR, sizeof(CLK_Obj));
    myCpu = CPU_init((void *)NULL, sizeof(CPU_Obj));
    myFlash = FLASH_init((void *)FLASH_BASE_ADDR, sizeof(FLASH_Obj));
    myGpio = GPIO_init((void *)GPIO_BASE_ADDR, sizeof(GPIO_Obj));
    myPie = PIE_init((void *)PIE_BASE_ADDR, sizeof(PIE_Obj));
    myPll = PLL_init((void *)PLL_BASE_ADDR, sizeof(PLL_Obj));
    myWDog = WDOG_init((void *)WDOG_BASE_ADDR, sizeof(WDOG_Obj));
    mySpi = SPI_init((void *)SPIA_BASE_ADDR, sizeof(SPI_Obj));
    mySci = SCI_init((void *)SCIA_BASE_ADDR, sizeof(SCI_Obj));
    myTimer = TIMER_init((void *)TIMER0_BASE_ADDR, sizeof(TIMER_Obj));
    //myTimer1 = TIMER_init((void *)TIMER1_BASE_ADDR, sizeof(TIMER_Obj));

    // Perform basic system initialization
    WDOG_disable(myWDog);
    CLK_enableAdcClock(myClk);
    (*Device_cal)();
    CLK_disableAdcClock(myClk);

    //Select the internal oscillator 1 as the clock source
    CLK_setOscSrc(myClk, CLK_OscSrc_Internal);

    // Setup the PLL for x12 /2 which will yield 60Mhz = 10Mhz * 12 / 2
    PLL_setup(myPll, PLL_Multiplier_12, PLL_DivideSelect_ClkIn_by_2);

    // If running from flash copy RAM only functions to RAM
#ifdef _FLASH
    memcpy(&RamfuncsRunStart, &RamfuncsLoadStart, (size_t)&RamfuncsLoadSize);
#endif

   // Enable XCLOCKOUT to allow monitoring of oscillator 1
   // GPIO_setMode(myGpio, GPIO_Number_18, GPIO_18_Mode_XCLKOUT);
   // CLK_setClkOutPreScaler(myClk, CLK_ClkOutPreScaler_SysClkOut_by_1);

    gpio_init();
    AIO_init();
    spi_init();
    nokia_init();
    adc_init();
    timer0_init();
    scia_init();

    CLK_disableTbClockSync(myClk);  // disable sync clock
    epwm_init();
    CLK_enableTbClockSync(myClk);  // enable sync clock

    // Disable the PIE and all interrupts
    // PIE_setDebugIntVectorTable(myPie);
	PIE_enable(myPie);
	CPU_disableGlobalInts(myCpu);
	CPU_clearIntFlags(myCpu);
	CPU_enableGlobalInts(myCpu);
	CPU_enableDebugInt(myCpu);

    xputs("\nCarabiner Testing Machine!\n");

    // Main program loop - continually sample ADC inputs
        for(;;) {

        	//Force start of conversion on SOC0, SOC1, SOC3, SOC4
        	ADC_forceConversion(myAdc, ADC_SocNumber_0);     // discard this conversion per errata
        	ADC_forceConversion(myAdc, ADC_SocNumber_1);     // ADCina1 reading
        	ADC_forceConversion(myAdc, ADC_SocNumber_3);     // ADCina3 reading
        	ADC_forceConversion(myAdc, ADC_SocNumber_4);     // ADCina4 reading for current reading on VNH5019A-E


            switch(encPos){
            case 1:
            	epwm1_info.EPwm1_TBPRD = 1878;   //set the TBPRD to 2Khz
                update_compare(&epwm1_info);     // update the EPwm registers
                break;
            case 2:
            	epwm1_info.EPwm1_TBPRD = 1252;   //set the TBPRD to 3Khz
            	update_compare(&epwm1_info);     // update the EPwm registers
            	break;
            case 3:
                epwm1_info.EPwm1_TBPRD = 938;    //set the TBPRD to 4Khz
                update_compare(&epwm1_info);     // update the EPwm registers
                break;
            case 4:
                epwm1_info.EPwm1_TBPRD = 750;    //set the TBPRD to 5Khz
                update_compare(&epwm1_info);     // update the EPwm registers
                break;
            case 5:
                epwm1_info.EPwm1_TBPRD = 626;    //set the TBPRD to 6Khz
                update_compare(&epwm1_info);     // update the EPwm registers
                break;
            case 6:
				epwm1_info.EPwm1_TBPRD = 536;   //set the TBPRD to 7Khz
				update_compare(&epwm1_info);     // update the EPwm registers
				break;
			case 7:
				epwm1_info.EPwm1_TBPRD = 470;   //set the TBPRD to 8Khz
				update_compare(&epwm1_info);     // update the EPwm registers
				break;
			case 8:
				epwm1_info.EPwm1_TBPRD = 418;    //set the TBPRD to 9Khz
				update_compare(&epwm1_info);     // update the EPwm registers
				break;
			case 9:
				epwm1_info.EPwm1_TBPRD = 376;    //set the TBPRD to 10Khz
				update_compare(&epwm1_info);     // update the EPwm registers
				break;
			case 10:
				epwm1_info.EPwm1_TBPRD = 342;    //set the TBPRD to 11Khz
				update_compare(&epwm1_info);     // update the EPwm registers
				break;
			case 11:
				epwm1_info.EPwm1_TBPRD = 312;   //set the TBPRD to 12Khz
				update_compare(&epwm1_info);     // update the EPwm registers
				break;
			case 12:
				epwm1_info.EPwm1_TBPRD = 288;   //set the TBPRD to 13Khz
				update_compare(&epwm1_info);     // update the EPwm registers
				break;
			case 13:
				epwm1_info.EPwm1_TBPRD = 268;    //set the TBPRD to 14Khz
				update_compare(&epwm1_info);     // update the EPwm registers
				break;
			case 14:
				epwm1_info.EPwm1_TBPRD = 250;    //set the TBPRD to 15Khz
				update_compare(&epwm1_info);     // update the EPwm registers
				break;
			case 15:
				epwm1_info.EPwm1_TBPRD = 234;    //set the TBPRD to 16Khz
				update_compare(&epwm1_info);     // update the EPwm registers
				break;
			case 16:
				epwm1_info.EPwm1_TBPRD = 222;   //set the TBPRD to 17Khz
				update_compare(&epwm1_info);     // update the EPwm registers
				break;
			case 17:
				epwm1_info.EPwm1_TBPRD = 208;   //set the TBPRD to 18Khz
				update_compare(&epwm1_info);     // update the EPwm registers
				break;
			case 18:
				epwm1_info.EPwm1_TBPRD = 198;    //set the TBPRD to 19Khz
				update_compare(&epwm1_info);     // update the EPwm registers
				break;
			case 19:
				epwm1_info.EPwm1_TBPRD = 188;    //set the TBPRD to 20Khz
				update_compare(&epwm1_info);     // update the EPwm registers
				break;

            default:
            	epwm1_info.EPwm1_TBPRD = EPWM1_START_TBPRD;   //set the TBPRD to 1Khz
            	update_compare(&epwm1_info);                  // update the EPwm registers
            	break;
            }

            writeString(0,0,">> PWM");
		    writeString(0,1,"Enc Count");
		    writeString(0,3,"ADC1 Buttons");
		    writeString(35,4,"Pressed");
            itoa(encPos, encCntToSend);
            writeString(68,1,"  ");
            writeString(68,1,encCntToSend);

            pwmFreq = figureFreq(epwm1_info.EPwm1_TBPRD);

            ftoa(pwmFreq, freqToSend, 4);
            writeString(0,2,freqToSend);

            if (pwmFreq >= 1.0) {
            	writeString(48,2,"Khz");
            } else {
                clearSome(47,16,54,24);
            }

            if (jogFlag)           // Jog
            	VNH5091AE_JOG();

            if (cycleFlag)
            	VNH5091AE_CYCLE();  // Cycle


            if (transmit) {

            	xprintf("Encoder count = %d\n", encPos);
            	ftoa(volts, voltsToSend, 2);
            	xputs("ADC3 Voltage = ");
				xputs(voltsToSend);
				xprintf("\nPress count = %d\n", pressCount);
                xprintf("Mode is %d\n", mode);

                if (CW){
                	xputs("Direction = CW\n");
                }else{
                	xputs("Direction = CWW\n");
                }

				transmit = false;
            }
      }
}

///////////////////  functions /////////////////////////////////////////////////////////

void gpio_init(void){

	// set up gpio pins for uart/sci
	GPIO_setPullUp(myGpio, GPIO_Number_28, GPIO_PullUp_Enable);
	GPIO_setPullUp(myGpio, GPIO_Number_29, GPIO_PullUp_Disable);
    GPIO_setQualification(myGpio, GPIO_Number_28, GPIO_Qual_ASync);
	GPIO_setMode(myGpio, GPIO_Number_28, GPIO_28_Mode_SCIRXDA);
	GPIO_setMode(myGpio, GPIO_Number_29, GPIO_29_Mode_SCITXDA);

	// Set up pins 32 as output
	GPIO_setMode(myGpio, pin32, GPIO_32_Mode_GeneralPurpose);
	GPIO_setDirection(myGpio, pin32, GPIO_Direction_Output);
	GPIO_setPullUp(myGpio, pin32, GPIO_PullUp_Disable);

	// debug led GPIO 12
	GPIO_setMode(myGpio, debugLed, GPIO_12_Mode_GeneralPurpose);
	GPIO_setDirection(myGpio, debugLed, GPIO_Direction_Output);
	GPIO_setPullUp(myGpio, debugLed, GPIO_PullUp_Disable);

	// set up GPIO's for nokia
	GPIO_setMode(myGpio, nokiaVcc, GPIO_33_Mode_GeneralPurpose);
	GPIO_setMode(myGpio, nokiaRst, GPIO_17_Mode_GeneralPurpose);
	GPIO_setMode(myGpio, nokiaDc, GPIO_7_Mode_GeneralPurpose);
	GPIO_setMode(myGpio, nokiaBlight, GPIO_34_Mode_GeneralPurpose);
	GPIO_setMode(myGpio, nokiaSce, GPIO_19_Mode_GeneralPurpose);

	GPIO_setDirection(myGpio, nokiaVcc, GPIO_Direction_Output);
	GPIO_setDirection(myGpio, nokiaRst, GPIO_Direction_Output);
	GPIO_setDirection(myGpio, nokiaDc, GPIO_Direction_Output);
	GPIO_setDirection(myGpio, nokiaBlight, GPIO_Direction_Output);
	GPIO_setDirection(myGpio, nokiaSce, GPIO_Direction_Output);

	GPIO_setPullUp(myGpio, nokiaVcc, GPIO_PullUp_Disable);
	GPIO_setPullUp(myGpio, nokiaRst, GPIO_PullUp_Disable);
	GPIO_setPullUp(myGpio, nokiaDc, GPIO_PullUp_Disable);
	GPIO_setPullUp(myGpio, nokiaBlight, GPIO_PullUp_Disable);
	GPIO_setPullUp(myGpio, nokiaSce, GPIO_PullUp_Disable);

	// Configure GPIO #'s 1-4 for VHN5019AE
	GPIO_setMode(myGpio, VNH5019AE_inA, GPIO_1_Mode_GeneralPurpose);
	GPIO_setMode(myGpio, VNH5019AE_inB, GPIO_2_Mode_GeneralPurpose);
	GPIO_setMode(myGpio, VNH5019AE_DiagA, GPIO_3_Mode_GeneralPurpose);
	GPIO_setMode(myGpio, VNH5019AE_DiagB, GPIO_4_Mode_GeneralPurpose);

	GPIO_setDirection(myGpio, VNH5019AE_inA, GPIO_Direction_Output);
	GPIO_setDirection(myGpio, VNH5019AE_inB, GPIO_Direction_Output);
	GPIO_setDirection(myGpio, VNH5019AE_DiagA, GPIO_Direction_Output);
	GPIO_setDirection(myGpio, VNH5019AE_DiagB, GPIO_Direction_Output);

	GPIO_setPullUp(myGpio, VNH5019AE_inA, GPIO_PullUp_Disable);
	GPIO_setPullUp(myGpio, VNH5019AE_inB, GPIO_PullUp_Disable);
	GPIO_setPullUp(myGpio, VNH5019AE_DiagA, GPIO_PullUp_Disable);
	GPIO_setPullUp(myGpio, VNH5019AE_DiagB, GPIO_PullUp_Disable);

	VNH5019AE_inA_Lo;         // start out with both inA and inB low (brake to Gnd)
	VNH5019AE_inB_Lo;
	//VNH5019AE_DiagA_Hi ;    // these are pulled up in hardware
	//VNH5019AE_DiagB_Hi ;

	//set up encoder on GPIO#'s 5&6
	GPIO_setMode(myGpio, enc1, GPIO_6_Mode_GeneralPurpose);
	GPIO_setMode(myGpio, enc2, GPIO_5_Mode_GeneralPurpose);

	GPIO_setDirection(myGpio, enc1, GPIO_Direction_Input);
	GPIO_setDirection(myGpio, enc2, GPIO_Direction_Input);

	GPIO_setPullUp(myGpio, enc1, GPIO_PullUp_Enable);
	GPIO_setPullUp(myGpio, enc2, GPIO_PullUp_Enable);

	GPIO_setQualification(myGpio, enc1, GPIO_Qual_Sample_6);
	GPIO_setQualification(myGpio, enc2, GPIO_Qual_Sample_6);

	GPIO_setQualificationPeriod(myGpio, enc1, 0xFF);
	GPIO_setQualificationPeriod(myGpio, enc2, 0xFF);


	//enc1 is  XINT1
	GPIO_setExtInt(myGpio, enc1, CPU_ExtIntNumber_1);
	//enc1 is  XINT3
	//GPIO_setExtInt(myGpio, enc1, CPU_ExtIntNumber_3);
    //enc2 is  XINT2
    GPIO_setExtInt(myGpio, enc2, CPU_ExtIntNumber_2);

	/////// this sets up encoder interrupts ////////

    //Configure XINT1 to trigger on falling edge
    PIE_setExtIntPolarity(myPie, CPU_ExtIntNumber_1, PIE_ExtIntPolarity_RisingAndFallingEdge);
	//Configure XINT3 to trigger on falling edge
	//PIE_setExtIntPolarity(myPie, CPU_ExtIntNumber_3, PIE_ExtIntPolarity_RisingEdge);
	//Configure XINT2 to trigger on falling edge
	PIE_setExtIntPolarity(myPie, CPU_ExtIntNumber_2, PIE_ExtIntPolarity_RisingAndFallingEdge);
	//Register XINT1 interrupt handler in the PIE vector table
	PIE_registerPieIntHandler(myPie, PIE_GroupNumber_1, PIE_SubGroupNumber_4, (intVec_t)&xint1_isr);
	//Register XINT3 interrupt handler in the PIE vector table
	//PIE_registerPieIntHandler(myPie, PIE_GroupNumber_12, PIE_SubGroupNumber_1, (intVec_t)&xint1_isr);
	//Register XINT2 interrupt handler in the PIE vector table
	PIE_registerPieIntHandler(myPie, PIE_GroupNumber_1, PIE_SubGroupNumber_5, (intVec_t)&xint2_isr);

	PIE_enableInt(myPie, PIE_GroupNumber_1, PIE_InterruptSource_XINT_1);
	//PIE_enableInt(myPie, PIE_GroupNumber_12, PIE_InterruptSource_XINT_3);
	PIE_enableInt(myPie, PIE_GroupNumber_1, PIE_InterruptSource_XINT_2);

	//Enable XINT1
	PIE_enableExtInt(myPie, CPU_ExtIntNumber_1);
    //Enable XINT3
	//PIE_enableExtInt(myPie, CPU_ExtIntNumber_3);
    //Enable XINT2
	PIE_enableExtInt(myPie, CPU_ExtIntNumber_2);

    //CPU_enableInt(myCpu, CPU_IntNumber_12);
    CPU_enableInt(myCpu, CPU_IntNumber_1);   //enable the enc1 and enc2 interrupt
}

void spi_init(void){

	// enable Spia clock
	CLK_enableSpiaClock(myClk);
	//set up GPIO's for SPI
	GPIO_setPullUp(myGpio, nokiaMosi, GPIO_PullUp_Disable);
    GPIO_setPullUp(myGpio, nokiaClk, GPIO_PullUp_Disable);
    GPIO_setPullUp(myGpio, nokiaSce, GPIO_PullUp_Disable);

    GPIO_setQualification(myGpio, nokiaMosi, GPIO_Qual_ASync);
    GPIO_setQualification(myGpio, nokiaClk, GPIO_Qual_ASync);
    GPIO_setQualification(myGpio, nokiaSce, GPIO_Qual_ASync);

    GPIO_setMode(myGpio, nokiaMosi, GPIO_16_Mode_SPISIMOA);
    GPIO_setMode(myGpio, nokiaClk, GPIO_18_Mode_SPICLKA);
    GPIO_setMode(myGpio, nokiaSce, GPIO_19_Mode_SPISTEA_NOT);

    // Resets the serial peripheral interface (SPI)
    SPI_reset(mySpi);
    // Initializes the serial peripheral interface (SPI) object handle
    SPI_enable(mySpi);
    // Enables the serial peripheral interface (SPI) transmit and receive channels
    SPI_enableChannels(mySpi);
    // Enable master mode
    SPI_setMode(mySpi, SPI_Mode_Master);
    // Enables the serial peripheral interface (SPI) masater/slave transmit mode
    SPI_enableTx(mySpi);
    // Sets the serial peripheral interface (SPI) character length
    SPI_setCharLength(mySpi, SPI_CharLength_8_Bits);
    //SPI_enableLoopBack(mySpi);
    // Sets the serial peripheral interface (SPI) clock phase
    SPI_setClkPhase(mySpi, SPI_ClkPhase_Normal);
    // Sets the serial peripheral interface (SPI) clock polarity
    SPI_setClkPolarity(mySpi, SPI_ClkPolarity_OutputFallingEdge_InputRisingEdge);
    // Enables the serial peripheral interface (SPI) over-run interrupt
    // SPI_enableOverRunInt(mySpi);
    SPI_enableInt(mySpi);
    // Sets the serial peripheral interface (SPI) baud rate
    SPI_setBaudRate(mySpi, (SPI_BaudRate_e)0x00);
    //SPI_setBaudRate(mySpi, 0x00);
    // Controls pin inversion of STE pin
    // SPI_setSteInv(mySpi, SPI_SteInv_ActiveLow);

    // Register interrupt handlers in the PIE vector table
    PIE_registerPieIntHandler(myPie, PIE_GroupNumber_6, PIE_SubGroupNumber_1, (intVec_t)&spiRxFifoIsr);

    PIE_enableInt(myPie, PIE_GroupNumber_6, PIE_InterruptSource_SPIARX);
    //PIE_enableInt(myPie, PIE_GroupNumber_6, PIE_InterruptSource_SPIATX);
    CPU_enableInt(myCpu, CPU_IntNumber_6);

    // Set so breakpoints don't disturb xmission
    SPI_setPriority(mySpi, SPI_Priority_FreeRun);
    }

void adc_init(void){

	ADC_reset(myAdc);

	CLK_enableAdcClock(myClk);

	// Initialize the ADC
    ADC_enableBandGap(myAdc);
    ADC_enableRefBuffers(myAdc);
    ADC_powerUp(myAdc);
    ADC_enable(myAdc);
    ADC_setVoltRefSrc(myAdc, ADC_VoltageRefSrc_Int);

    ADC_setIntPulseGenMode(myAdc, ADC_IntPulseGenMode_During);      //ADCINT1 trips 1 cycle after AdcResults latch
    ADC_enableInt(myAdc, ADC_IntNumber_1);                                          //Enable ADCINT1
    ADC_setIntSrc(myAdc, ADC_IntNumber_1, ADC_IntSrc_EOC1);                         //Connect ADCINT1 to EOC1

    ADC_setSocTrigSrc(myAdc, ADC_SocNumber_0,ADC_SocTrigSrc_Sw);                    // software trigger
    ADC_setSocTrigSrc(myAdc, ADC_SocNumber_1,ADC_SocTrigSrc_Sw);                    // software trigger
    ADC_setSocTrigSrc(myAdc, ADC_SocNumber_3,ADC_SocTrigSrc_Sw);                    // software trigger
    ADC_setSocTrigSrc(myAdc, ADC_SocNumber_4,ADC_SocTrigSrc_Sw);                    // software trigger

    ADC_setSocChanNumber (myAdc, ADC_SocNumber_0, ADC_SocChanNumber_A0);             //Set SOC0 channel select to ADCINA0
    ADC_setSocChanNumber (myAdc, ADC_SocNumber_1, ADC_SocChanNumber_A1);             //Set SOC1 channel select to ADCINA1
    ADC_setSocChanNumber (myAdc, ADC_SocNumber_3, ADC_SocChanNumber_A3);             //Set SOC3 channel select to ADCINA3
    ADC_setSocChanNumber (myAdc, ADC_SocNumber_4, ADC_SocChanNumber_A4);             //Set SOC4 channel select to ADCINA4

    ADC_setSocSampleWindow(myAdc, ADC_SocNumber_0, ADC_SocSampleWindow_7_cycles);     //Set SOC0 acquisition period to 7 ADCCLK
    ADC_setSocSampleWindow(myAdc, ADC_SocNumber_1, ADC_SocSampleWindow_20_cycles);    //Set SOC1 acquisition period to 20 ADCCLK
    ADC_setSocSampleWindow(myAdc, ADC_SocNumber_3, ADC_SocSampleWindow_20_cycles);    //Set SOC3 acquisition period to 20 ADCCLK
    ADC_setSocSampleWindow(myAdc, ADC_SocNumber_4, ADC_SocSampleWindow_20_cycles);    //Set SOC4 acquisition period to 20 ADCCLK

    ADC_setIntMode(myAdc,ADC_IntNumber_1 ,ADC_IntMode_ClearFlag);                  //new interrupt will not happen until the flag is cleared
    //ADC_setIntMode(myAdc,ADC_IntNumber_1 ,ADC_IntMode_EOC);                        //new interrupt will happen at the EOC regardless of flag

    PIE_registerPieIntHandler(myPie, PIE_GroupNumber_10, PIE_SubGroupNumber_1, (intVec_t)&adc_Isr);
    PIE_enableAdcInt(myPie, ADC_IntNumber_1);

    CPU_enableInt(myCpu, CPU_IntNumber_10);

    // Set the flash OTP wait-states to minimum. This is important
    // for the performance of the temperature conversion function.
    FLASH_setup(myFlash);
}

// Epwm
void epwm_init(void){

	CLK_enablePwmClock(myClk, PWM_Number_1);

	// Set up GPIO#'s 0 & 1 for EPWM1
	GPIO_setPullUp(myGpio, GPIO_Number_0, GPIO_PullUp_Disable);
	GPIO_setMode(myGpio, GPIO_Number_0, GPIO_0_Mode_EPWM1A);
	//GPIO_setPullUp(myGpio, GPIO_Number_1, GPIO_PullUp_Disable);
	//GPIO_setMode(myGpio, GPIO_Number_1, GPIO_1_Mode_EPWM1B);

	// Setup TBCLK
	PWM_setPeriod(myPwm1, EPWM1_START_TBPRD);         // Start out at 1Khz
	PWM_setPhase(myPwm1, 0x0000);                     // Phase is 0
	PWM_setCount(myPwm1, 0x0000);                     // Clear counter

	// Set Compare values
	PWM_setCmpA(myPwm1,EPWM1_START_CMPA);      // Set compare A value to half of TBPRD (1880) for 50% duty cycle 1Khz square wave
    //PWM_setCmpB(myPwm1, EPWM1_MAX_CMPA);            // for CMPB

	// Setup counter mode
	//PWM_setCounterMode(myPwm1, PWM_CounterMode_UpDown);        // Count up/down
	PWM_setCounterMode(myPwm1, PWM_CounterMode_Stop);            // Count mode is stopped durind startup
	PWM_disableCounterLoad(myPwm1);                              // Disable phase loading
	PWM_setHighSpeedClkDiv(myPwm1, PWM_HspClkDiv_by_8);          // Clock ratio to SYSCLKOUT/8
	PWM_setClkDiv(myPwm1, PWM_ClkDiv_by_1);

	// Setup shadowing
	PWM_setShadowMode_CmpA(myPwm1, PWM_ShadowMode_Shadow);
	PWM_setLoadMode_CmpA(myPwm1, PWM_LoadMode_Either);
	//PWM_setShadowMode_CmpB(myPwm1, PWM_ShadowMode_Shadow);
	//PWM_setLoadMode_CmpB(myPwm1, PWM_LoadMode_Either);

	// Set actions
	PWM_setActionQual_CntUp_CmpA_PwmA(myPwm1, PWM_ActionQual_Set);     // Set PWM1A on event A, up count
	PWM_setActionQual_CntDown_CmpA_PwmA(myPwm1, PWM_ActionQual_Clear); // Clear PWM1A on event A, down count
    //PWM_setActionQual_CntUp_CmpB_PwmB(myPwm1, PWM_ActionQual_Set);
    //PWM_setActionQual_CntDown_CmpB_PwmB(myPwm1, PWM_ActionQual_Clear);

	// Set up interrupts
	//PWM_setIntMode(myPwm1, PWM_IntMode_CounterEqualZeroOrPeriod); // Select INT on Zero or period event
	//PWM_enableInt(myPwm1);                               // Enable INT
	//PWM_setIntPeriod(myPwm1, PWM_IntPeriod_ThirdEvent); // Generate INT on 3rd event

	// Register interrupt handlers in the PIE vector table
	//PIE_registerPieIntHandler(myPie, PIE_GroupNumber_3, PIE_SubGroupNumber_1,
	//(intVec_t) &epwm1_isr);
	//Enable EPWM INTn in the PIE: Group 3 interrupt 1
	//PIE_enablePwmInt(myPie, PWM_Number_1);
	//Enable CPU INT3 which is connected to EPWM1  INT:
	//CPU_enableInt(myCpu, CPU_IntNumber_3);

	epwm1_info.myPwmHandle = myPwm1;                         // Set the pointer to the ePWM module
	}

void timer0_init(void){

	// Configure CPU-Timer0 to interrupt every 1 second:
	// 60MHz CPU Freq, 50 millisecond Period (in uSeconds)
	// ConfigCpuTimer(&CpuTimer0, 60, 500000);    // 1/2 second interrupt
	TIMER_stop(myTimer);
	TIMER_setPeriod(myTimer,  50000000);   // 1 second interrupt
	TIMER_setPreScaler(myTimer, 0);
	TIMER_reload(myTimer);
	TIMER_setEmulationMode(myTimer, TIMER_EmulationMode_StopAfterNextDecrement);
	//TIMER_setEmulationMode(myTimer, TIMER_EmulationMode_RunFree);
	//TIMER_setEmulationMode(myTimer, TIMER_EmulationMode_StopAtZero);
	TIMER_enableInt(myTimer);

	// Register interrupt handlers in the PIE vector table
	PIE_registerPieIntHandler(myPie, PIE_GroupNumber_1, PIE_SubGroupNumber_7,(intVec_t) &timer0_isr);

	// Enable TINT0 in the PIE: Group 1 interrupt 7
	PIE_enableTimer0Int(myPie);

	// Enable CPU int1 which is connected to CPU-Timer 0.
	CPU_enableInt(myCpu, CPU_IntNumber_1);

	/*
	TIMER_stop(myTimer1);
	TIMER_setPeriod(myTimer1, 50000000);   // 1 second interrupt
	TIMER_setPreScaler(myTimer1, 0);
	TIMER_reload(myTimer1);
	//TIMER_setEmulationMode(myTimer, TIMER_EmulationMode_StopAfterNextDecrement);
	TIMER_setEmulationMode(myTimer1, TIMER_EmulationMode_RunFree);
	TIMER_enableInt(myTimer1);

	// Register interrupt handlers in the PIE vector table
	PIE_registerSystemIntHandler(myPie, PIE_SystemInterrupts_TINT1, (intVec_t)& timer1_isr);

	// Enable CPU int13 which is connected to CPU-Timer 1.
	CPU_enableInt(myCpu, CPU_IntNumber_13);
    */
}
// ftoa for converting a float to a string
//arguments: f = float to convert, buf = an array to write to, decPlaces = # of decimal places
void ftoa(float f, char *buf, int decPlaces) {

	//int pos = 0, i, dp, num;
	int pos = 0;
	int i = 0;
	int dp = 0;
	int num = 0;

	if (f < 0) {
		buf[pos++] = '-';
		f = -f;
	}

	dp = 0;
	while (f >= 10.0) {
		f = f / 10.0;
		dp++;
	}

	unsigned int total = dp + decPlaces;

	for (i = 0; i < total; i++) {
		num = f;
		f = f - num;
		if (num > 9) {
			buf[pos++] = '#';
		} else {
			buf[pos++] = '0' + num;
		}
		if (dp == 0) {
			buf[pos++] = '.';
		}
		f = f * 10.0;
		dp--;
	} // loop

	buf[pos++] = 0;  // null
}

// base-10 itoa for positive numbers. Populates str with a null-terminated string.
// arguments: val = interger to be changed , str = an array to write to
void itoa(unsigned int val, char *str) {

	int temploc = 0;
	int digit = 0;
	int strloc = 0;
	char tempstr[5];    //16-bit number can be at most 5 ASCII digits;

	// Get the number of digits
	if (val >= 1000)
		digit = 4;
	else if (val >= 100)
		digit = 3;
	else if (val >= 10)
		digit = 2;
	else
		digit = 1;

	do {
		digit = val % 10;
		tempstr[temploc++] = digit + '0';
		val /= 10;
	} while (val > 0);
	// reverse the digits back into the output string
	while (temploc > 0)
		str[strloc++] = tempstr[--temploc];
	str[strloc] = 0;
}

float figureFreq(uint16_t TBPRD) {  // for 7.5 Mhz clock (sysClk/8) .133us TBPRD

	float tFreq = 2 * TBPRD * TPWM;
	float freq = 1 / tFreq;

	return freq * 1000;  // for Mhz
}
/*
// rounding function
int round(float number) {

	return (number >= 0) ? (int) (number + 0.5) : (int) (number - 0.5);
}
*/
// mapping function
long map(long x, long in_min, long in_max, long out_min, long out_max) {

	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void update_compare(EPWM_INFO *epwm_info){

	// update TBPRD register
	PWM_setPeriod(epwm_info->myPwmHandle,epwm_info->EPwm1_TBPRD); //update TBPRD

	if (epwm1_info.EPwm1_TBPRD <= 190 ){            // 190 = 19.9 Khz

		epwm1_info.EPwm1_CMPA = EPWM1_MIN_CMPA ;         // make the CMPA MAX (95)  as to not divide 0
		PWM_setCmpA(epwm_info->myPwmHandle,epwm_info->EPwm1_CMPA); //update CMPA

	}else
		epwm1_info.EPwm1_CMPA = epwm1_info.EPwm1_TBPRD/2 ;      // divide TBPRD by 2 for 50% duty cycle
	    PWM_setCmpA(epwm_info->myPwmHandle,epwm_info->EPwm1_CMPA); //update CMPA


	// epwm1_info.EPwm1_CMPB= epwm1_info.EPwm1_TBPRD/2 ;      // divide TBPRD  by 2 for 50% duty cycle and shift 8 for EPWM
	//PWM_setCmpA(epwm_info->myPwmHandle,epwm_info->EPwm1_CMPA); //update CMPA

	// update TBPRD register
	//PWM_setPeriod(epwm_info->myPwmHandle,epwm_info->EPwm1_TBPRD); //update TBPRD

    // update CMPA register
	//PWM_setCmpA(epwm_info->myPwmHandle,epwm_info->EPwm1_CMPA); //update CMPA
	//PWM_setCmpB(epwm_info->myPwmHandle,epwm_info->EPwm1_CMPB); //update CMPB
}

// This code taken from Michael Kellett's
// Interfacing micro-controllers with incremental shaft encoders
uint16_t encUpdate(int16_t value){

	//static uint16_t angle = 0;           // encoder position
	//static int16_t oldAngle = 0;        // old position data

	oldAngle = oldAngle << 2;           // shift the old angle left 2 places
	oldAngle |= (value & 0x03);         // or in the new port value and
	angle += table[(oldAngle & 0xf)];   // index into table for the direction value

	if (angle <= 0) {                  // don't count past 0 or 19
		angle = 0;
	} else if (angle >= 19) {
		angle = 19;
	}

	return angle;
}

void scia_init(void)
{
    CLK_enableSciaClock(myClk);

    // 1 stop bit,  No loopback
    // No parity,8 char bits,
    // async mode, idle-line protocol
    SCI_disableParity(mySci);
	SCI_setNumStopBits(mySci, SCI_NumStopBits_One);
	SCI_setCharLength(mySci, SCI_CharLength_8_Bits);

	SCI_enableTx(mySci);
	SCI_enableRx(mySci);
	SCI_enableTxInt(mySci);
	SCI_enableRxInt(mySci);

	SCI_setBaudRate(mySci, SCI_BaudRate_115_2_kBaud);

	SCI_enableFifoEnh(mySci);
	SCI_resetTxFifo(mySci);
	SCI_clearTxFifoInt(mySci);
	SCI_resetChannels(mySci);
	SCI_setTxFifoIntLevel(mySci, SCI_FifoLevel_Empty);

	SCI_resetRxFifo(mySci);
	SCI_clearRxFifoInt(mySci);
	SCI_setRxFifoIntLevel(mySci, SCI_FifoLevel_4_Words);

	SCI_setPriority(mySci, SCI_Priority_FreeRun);

	SCI_enable(mySci);

	xdev_out(scia_xmit);	// Pointer to the output stream for ChaN's xprintf

}

// Transmit a character from the SCI
void scia_xmit(char a){

    while(SCI_getTxFifoStatus(mySci) != SCI_FifoStatus_Empty){}
    //SCI_putDataBlocking(mySci, a);
    //SCI_putDataNonBlocking(mySci, a);
    SCI_putData(mySci, a);
}

/*
void scia_msg(char * msg){
    int i;
    i = 0;
    while(msg[i] != '\0')
    {
        scia_xmit(msg[i]);
        i++;
    }
}
*/

////////////////// truth table ///////////////////////////////
/*
 *       InA       InB       DiagA/EnA    DiagB/EnB          Operating Mode
 *
 *        1         1            1           1                Brake to Vcc
 *        1         0            1           1                Clockwise
 *        0         1            1           1                Counter clockwise
 *        0         0            1           1                Brake to Gnd
 */

///////////////// functions for VNH5091AE //////////////////////////

void VNH5091AE_changeDir(void) {// this switches the actuator direction

	if (CW == true) {
        xputs("Direction = CCW\n");
		CW = false;
	} else {
		xputs("Direction = CW\n" );
		CW = true;
	}
}

void VNH5091AE_CCW(void){

	VNH5019AE_inA_Lo;
	VNH5019AE_inB_Hi;
	//VNH5019AE_DiagA_Hi;   // these are pulled high in hardware
	//VNH5019AE_DiagB_Hi;
}

void VNH5091AE_CW(void){

	VNH5019AE_inA_Hi;
	VNH5019AE_inB_Lo;
	//VNH5019AE_DiagA_Hi;   // these are pulled high in hardware
	//VNH5019AE_DiagB_Hi;
}

void VNH5091AE_BRAKE_TO_VCC(void){

	//CLK_disablePwmClock(myClk, PWM_Number_1 ); // disable the PWM1 clock
	//epwm1_info.EPwm1_TBPRD = 0;  //  set TBPRD to 0
	//PWM_setPeriod(epwm_info->myPwmHandle,epwm_info->EPwm1_TBPRD); // set the period to 0 for no PWM

	PWM_setCounterMode(myPwm1, PWM_CounterMode_Stop); // stop PWM
	VNH5019AE_inA_Hi;
	VNH5019AE_inB_Hi;
	//VNH5019AE_DiagA_Hi;   // these are pulled high in hardware
	//VNH5019AE_DiagB_Hi;
}

void VNH5091AE_BRAKE_TO_GND(void){

	//CLK_disablePwmClock(myClk, PWM_Number_1 ); // disable the PWM1 clock
	//epwm1_info.EPwm1_TBPRD = 0;  //  set TBPRD to 0
	//PWM_setPeriod(epwm_info->myPwmHandle,epwm_info->EPwm1_TBPRD); // set the period to 0 for no PWM

	PWM_setCounterMode(myPwm1, PWM_CounterMode_Stop); // stop PWM
	VNH5019AE_inA_Lo;
	VNH5019AE_inB_Lo;
	//VNH5019AE_DiagA_Hi;   // these are pulled high in hardware
	//VNH5019AE_DiagB_Hi;
}

void VNH5091AE_FAULT_FIX(void){

	//CLK_disablePwmClock(myClk, PWM_Number_1 ); // disable the PWM1 clock
	//epwm1_info.EPwm1_TBPRD = 0;  //  set TBPRD to 0
	//PWM_setPeriod(epwm_info->myPwmHandle,epwm_info->EPwm1_TBPRD); // set the period to 0 for no PWM

	PWM_setCounterMode(myPwm1, PWM_CounterMode_Stop); // stop the counter
	VNH5019AE_DiagA_Lo;   // these are pulled high in hardware
    VNH5019AE_DiagB_Lo;   // Pull low to clear fault

	VNH5019AE_inA_Toggle;
	VNH5019AE_inB_Toggle;
}

void VNH5091AE_STOP(void){

	VNH5091AE_BRAKE_TO_GND();  // brake
	pin32High;                 // red led on
	mode = stop;
	xputs("!!BRAKE!!\n");
}

void VNH5091AE_CYCLE(void){

	int16_t result = ADC3_value;
	if (mode == cycle) {
		PWM_setCounterMode(myPwm1, PWM_CounterMode_UpDown);        // Count up/down start PWM
		xputs("Pull Time\n");
		VNH5091AE_CW();            // go CW
		while (result > 3000) {    // wait till we get a signal from the W127
			xputs("pulling\n");
			ADC_forceConversion(myAdc, ADC_SocNumber_3);             // ADCina3 conversion
			result = ADC_readResult(myAdc, ADC_ResultNumber_3);      // ADCina3 reading
			xprintf("ADC = %d\n", result);
		}

		VNH5091AE_RETURN();            // return actuator
	}
}

void VNH5091AE_JOG(void) {

	if (mode == jog) {
		PWM_setCounterMode(myPwm1, PWM_CounterMode_UpDown); // Count up/down start PWM
		if (CW) {
			VNH5091AE_CW();
		    xputs("jogging CW\n");
		    jogFlag = false;
		} else {
			VNH5091AE_CCW();
            xputs("jogging CCW\n");
            jogFlag = false;
		}
	}
}

void VNH5091AE_RETURN(void) {

	TIMER_start(myTimer);          // Start  1 sec timer
	VNH5091AE_CCW();               // return actuator for 10sec
	//secCounter = 0;
	while (secCounter < 10) {
        xprintf("counter = %d\n", secCounter);
	}
	if (secCounter >= 10) {        // after 10 seconds stop actuator
		TIMER_stop(myTimer);
		VNH5091AE_STOP();
		secCounter = 0;
		xputs("Cycle Completed!\n");
		mode = stop;
		cycleFlag = false;
	}
}

/////////////////////// interrupt routines ///////////////////////////////////////////////////////////////
interrupt void timer0_isr(void){

    AIO14_toggle;
    //toggleDebugLed;
    secCounter++;

    // clear interrupt
    PIE_clearInt(myPie, PIE_GroupNumber_1);
}
/*
interrupt void timer1_isr(void){

	toggleDebugLed;
	secCounter++;
}
*/
// ADCINA1,3,4  interrupt service routine
interrupt void adc_Isr(void){

	int16_t j,k,l;

	ADC3[c] = ADC_readResult(myAdc, ADC_ResultNumber_3);  // ADCINA3 reading will be sampled 7 times relay from W127 unit
	ADC4[c] = ADC_readResult(myAdc, ADC_ResultNumber_4);  // ADCINA4 reading will be sampled 7 times motor current sense
	ADC1[c] = ADC_readResult(myAdc, ADC_ResultNumber_1);  // ADCINA1 reading will be sampled 7 times buttons

	if(c == 7){

	ADC3_average = ADC3[2];  //throw out the first 2 samples
	ADC4_average = ADC4[2];  //throw out the first 2 samples
	ADC1_average = ADC1[2];  //throw out the first 2 samples

	for(j=2; j<4; j++){ADC3_average += ADC3[j];}    //sum samples 2,3,4 and throw out the last2
    for(k=2; k<4; k++){ADC1_average += ADC1[k];}
    for(l=2; l<4; l++){ADC4_average += ADC4[l];}

	ADC3_value = ADC3_average/3;     //divide by 3
	ADC4_value = ADC4_average/3;     //divide by 3
	ADC1_value = ADC1_average/3;     //divide by 3

	//epwm1_info.EPwm1_TBPRD = map(ADC3_value,0,4095,190,65534); //map the pot readings to the PWM max and min values
	//update_compare(&epwm1_info); // update the EPwm registers

	// button values
    if((ADC1_value > 3450) &&(ADC1_value < 3550)){
    	clearSome(0,40,83,47);
    	writeString(0,4,"# 1");
    	drawFilledCircle(5,44,3);
    	pressCount++;

    	switch (pressCount){
			case 0:
				mode = stop;
				break;
			case 1:
				mode = jog;
				break;
			case 2:
				mode = cycle;
				break;
			default:
				mode = stop;
				pressCount = 0;
				break;
    	}

    	transmit = true;

    }else if((ADC1_value > 3600) &&(ADC1_value < 3700)){
    	clearSome(0,40,83,47);
    	writeString(0,4,"# 2");
    	drawFilledCircle(19,44,3);
    	//VNH5091AE_CYCLE();
    	cycleFlag = true;
    }else if((ADC1_value > 3700) &&(ADC1_value < 3770)){
    	clearSome(0,40,83,47);
    	writeString(0,4,"# 3");
    	drawFilledCircle(33,44,3);
    	VNH5091AE_changeDir();
    }else if((ADC1_value > 3270) &&(ADC1_value < 3340)){
    	clearSome(0,40,83,47);
       	writeString(0,4,"# 4");
       	drawFilledCircle(47,44,3);
       	//VNH5091AE_JOG();
       	jogFlag = true;
    }else if((ADC1_value > 2740) &&(ADC1_value < 2790)){
    	clearSome(0,40,83,47);
       	writeString(0,4,"# 5");
       	drawFilledCircle(61,44,3);
    }else if((ADC1_value > 0 ) &&(ADC1_value < 400)){
    	clearSome(0,40,83,47);
       	writeString(0,4,"# 6");
       	drawFilledCircle(75,44,3);
       	VNH5091AE_STOP();
     }

	//Convert ADC_average to volts
	//volts = (((float)ADC1_value/4095) * 3300);
    volts = (((float)ADC3_value/4095) * 3300);

	//itoa(ADC3_value,adcAsString);
	//writeString(0,2,"     ");
	//writeString(0,2,adcAsString);           // send ADCINA3 resolution value
	//ftoa(volts,voltsToSend,4);
	//writeString(0,2,"     ");
	//writeString(0,2,voltsToSend);          // send ADCINA3 in millivolts

	c = 0;                                   // reset cycle counter

	}else
		c++;                                // increment cycle counter

	// Clear ADCINT1 flag reinitialize for next SOC
	ADC_clearIntFlag(myAdc, ADC_IntNumber_1);
	// Acknowledge interrupt to PIE
	PIE_clearInt(myPie, PIE_GroupNumber_10);

}

interrupt void spiRxFifoIsr(void){

    //set Sce high
	//GPIO_setHigh(myGpio, nokiaSce);      // for software chip enable

	//read the rx register
    SPI_read(mySpi);

    // Issue PIE ack
    PIE_clearInt(myPie, PIE_GroupNumber_6);
}

///// encoder interrupts ////
interrupt void xint1_isr(void){

	PIE_disableInt(myPie, PIE_GroupNumber_1, PIE_InterruptSource_XINT_1);
	//PIE_disableInt(myPie, PIE_GroupNumber_12, PIE_InterruptSource_XINT_3);

    int16_t pCheck = (portA & 0x60) >> 5;   // read bits 5 and 6 on port A

	encPos = encUpdate(pCheck);        // update the encoder position


	PIE_clearInt(myPie, PIE_GroupNumber_1);       // Issue PIE ack
    //PIE_clearInt(myPie, PIE_GroupNumber_12);    // Issue PIE ack
    pin32Toggle;
    PIE_enableInt(myPie, PIE_GroupNumber_1, PIE_InterruptSource_XINT_1);
    //PIE_enableInt(myPie, PIE_GroupNumber_12, PIE_InterruptSource_XINT_3);
}

interrupt void xint2_isr(void){

	PIE_disableInt(myPie, PIE_GroupNumber_1, PIE_InterruptSource_XINT_2);

	int16_t pCheck = (portA & 0x60) >> 5;   // read bits 5 and 6 on port A

	encPos = encUpdate(pCheck);             // update the encoder position


    PIE_clearInt(myPie, PIE_GroupNumber_1);       // Issue PIE ack
    toggleDebugLed;
    PIE_enableInt(myPie, PIE_GroupNumber_1, PIE_InterruptSource_XINT_2);
}

/*
interrupt void epwm1_isr(void)
{

	epwm1_info.EPwm1_TBPRD = map(ADC3_value,0,4095,190,68000); //map the pot readings to the PWM max and min values
	// update the CMPA register
	update_compare(&epwm1_info);

    // Clear INT flag for this timer
    PWM_clearIntFlag(myPwm1);

    // Acknowledge this interrupt to receive more interrupts from group 3
    PIE_clearInt(myPie, PIE_GroupNumber_3);

    GPIO_toggle(myGpio,GPIO_Number_2);
}
*/
