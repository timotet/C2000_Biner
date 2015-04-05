/*
 * nokia5110.h
 * 9/22/12
 */

#ifndef NOKIA5110_H_
#define NOKIA5110_H_


#define nokiaVcc     GPIO_Number_33     //connect to pin  J6_8 on c2000 LP board
#define nokiaRst     GPIO_Number_17     //connect to pin  J2_7 on c2000 LP board
#define nokiaSce     GPIO_Number_19     //connect to pin  J2_2 on c2000 LP board     //hardware chip enable
//#define nokiaSce     GPIO_Number_34   //connect to pin  J1_5 on c2000 LP board    //software chip enable
#define nokiaClk     GPIO_Number_18     //connect to pin  J1_7 on on c2000 LP board SPICLK
#define nokiaDc      GPIO_Number_7      //connect to pin  J2_9 on c2000 LP board
#define nokiaBlight  GPIO_Number_34     //connect to pin  J1_5 on on c2000 LP board
#define nokiaMosi    GPIO_Number_16     //connect to pin  J6_7 on on c2000 LP board SPISIMOA



#define inverse     0x0d   //display control inverse mode
#define BonW        0x0c   //display control black on white
#define vAddress    0x22
#define hAddress    0x20

#define PIXEL_OFF 0
#define PIXEL_ON  1
#define PIXEL_XOR 2

	void writeCommand(unsigned char);
	void writeData(unsigned char);

	// Init/Clear/position functions
	void nokia_init(void);
	void clear(void);
	void clearSome(unsigned char xs, unsigned char ys, unsigned char xe, unsigned char ye);
	void gotoXY(unsigned char x, unsigned char y);
	void update(void);

	// String and character functions
	void writeString(unsigned char x, unsigned char y, char *s);
	void writeChar(unsigned char ch);
	void printV(unsigned char x, unsigned char y, unsigned char length,
			char *characters);

	// Bitmap functions
	void drawBitmap(unsigned char x, unsigned char y, const unsigned char *map,
			unsigned char w, unsigned char h);
	void clearBitmap(unsigned char x, unsigned char y, unsigned char size_x,
			unsigned char size_y);

	// Graphic functions
	void setPixel(unsigned char x, unsigned char y, unsigned char c);
	void drawLine(unsigned char x1, unsigned char y1, unsigned char x2,
			unsigned char y2, unsigned char c);
	void drawRectangle(unsigned char x1, unsigned char y1, unsigned char x2,
			unsigned char y2, unsigned char c);
	void drawFilledRectangle(unsigned char x1, unsigned char y1,
			unsigned char x2, unsigned char y2, unsigned char c);
	void drawCircle(unsigned char xc, unsigned char yc, unsigned char r,
			unsigned char c);
	void drawFilledCircle(unsigned char x, unsigned char y, unsigned char radius);

#endif /* NOKIA5110_H_ */
