/*
 * nokia5110.c
 * 9/22/12
 */

#include "DSP28x_Project.h" // Device Header file and Examples Include File

#include "nokia5110.h"
#include "font8x8.h"
//#include "font6x8.h"
#include "f2802x_common/include/gpio.h"
#include "f2802x_common/include/spi.h"

#define LCDCOLMAX	84
#define LCDROWMAX	6
#define LCDPIXELROWMAX	48

extern SPI_Handle mySpi;
extern GPIO_Handle myGpio;

// current cursor position
static unsigned char cursor_row = 0; /* 0-5 */
static unsigned char cursor_col = 0; /* 0-83 */

static unsigned char lcd_buffer[LCDROWMAX][LCDCOLMAX] = { { 0 }, { 0 } };

/*
 * Name         : init
 * Description  : Main LCD initialization function
 * Argument(s)  : None
 * Return value : None
 */
void nokia_init(void) {

	GPIO_setHigh(myGpio, nokiaVcc ); //power to LCD
	GPIO_setHigh(myGpio, nokiaRst ); //set RESET high
	GPIO_setLow(myGpio, nokiaRst ); //set RESET low
	DSP28x_usDelay(5000000);
	GPIO_setHigh(myGpio, nokiaRst ); //set RESET high
	GPIO_setHigh(myGpio, nokiaSce ); //set Sce high
	writeCommand(0x21); // LCD Extended instruction set
	writeCommand(0xF0); // Set LCD Vop (Contrast). //0xE0 - BF  may have to play with
	writeCommand(0x07); // Set Temp coefficient. //0x04 =t0 //0x05=t1 // 0x06=t2 // 0x07=t3
	writeCommand(0x13); // LCD bias mode 1:100 0x10 //1:48 0x13
	writeCommand(0x20); // LCD basic instruction set
	writeCommand(BonW); // LCD  0x0C for black on white //0x0d for inverse
	GPIO_setHigh(myGpio, nokiaBlight ); //power to back light
	clear();
}

/*
 * Name         : writeCommand
 * Description  : Sends command to display controller
 * Argument(s)  : command - The command to be sent
 * Return value : none
 */
void writeCommand(unsigned char command) {

	GPIO_setLow(myGpio, nokiaDc ); //set LCD to command mode dc low
	//GPIO_setLow(myGpio, nokiaSce ); //SCE pin low     for software chip enable
	SPI_write8(mySpi, command); // shift out 8 bits

}

/*
 * Name         : writeData
 * Description  : Sends data to the display controller
 * Argument(s)  : Data - Data to be sent
 * Return value : none
 */
void writeData(unsigned char data) {

	GPIO_setHigh(myGpio, nokiaDc ); //set LCD to data mode dc high
	//GPIO_setLow(myGpio, nokiaSce ); //SCE pin low     for software chip enable
	SPI_write8(mySpi, data); // shift out 8 bits

}

/*
 * Name         : drawBitmap
 * Description  : Sends a bitmap image to the display
 * Argument(s)  : x, y - Position on screen, x 0-83, y 1-6
 *                map - pointer to data
 *                size_x,size_y - Size of the image in pixels,
 *                size_y is multiple of 8
 * Return value : none
 */

void drawBitmap(unsigned char x, unsigned char y, const unsigned char * map,
		unsigned char w, unsigned char h) {
	unsigned char i, n;
	unsigned char row;

	row = (h % 8 == 0) ? h / 8 : h / 8 + 1;
	for (n = 0; n < row; n++) {
		gotoXY(x, y);
		for (i = 0; i < w; i++) {
			writeData(map[i + n * w]);
		}
	y++;
  }
}

/*
 * Name         : clearBitmap
 * Description  : Clear an area of the screen, usually to blank out a
 * 		  previously drawn image or part of image.
 * Argument(s)  : x, y - Position on screen, x 0-83, y 1-6
 *                size_x,size_y - Size of the image in pixels,
 *                size_y is multiple of 8
 * Return value : none
 */
void clearBitmap(unsigned char x, unsigned char y, unsigned char size_x,
		unsigned char size_y) {
	unsigned int i, n;
	unsigned char row;

	row = (size_y % 8 == 0) ? size_y / 8 : size_y  / 8;
	for (n = 0; n < row; n++) {
		gotoXY(x, y);
		for (i = 0; i < size_x; i++) {
			writeData(0x00);
		}
    y++;
  }
}

/*
 * Name         : writeString
 * Description  : Write a string to the LCD from current position
 * Argument(s)  : x - Column to start at, 0-83
 * 		  y - Row to start, 0-6
 * 		  s - Pointer to start of string
 *
 * Return value : none
 */
void writeString(unsigned char x, unsigned char y, char *string) {
	gotoXY(x, y);
	while (*string)
		writeChar(*string++);
}

/*
 * Name         : writeChar
 * Description  : Write a single normal font character to screen
 * 		  at current cursor position
 * Argument(s)  : ch - character to display
 *
 * Return value : none
 */

/*
// this is for 6x8 font
void writeChar(unsigned char ch) {
	unsigned char j;

	if (cursor_col > LCDCOLMAX - 6)
		cursor_col = LCDCOLMAX - 6; // ensure space is available for the character
	if (cursor_row > LCDROWMAX - 1)
		cursor_row = LCDROWMAX - 1; // ensure space is available for the character

	lcd_buffer[cursor_row][cursor_col] = 0x00;
	for (j = 0; j < 5; j++) {
		lcd_buffer[cursor_row][cursor_col + j] = (font6x8[ch - 0x20][j]);
	}

	lcd_buffer[cursor_row][cursor_col + 5] = 0x00;

	for (j = 0; j < 5; j++) {
		writeData(lcd_buffer[cursor_row][cursor_col++]);
		if (cursor_col >= LCDCOLMAX) {
			cursor_col = 0;
			cursor_row++;
			if (cursor_row >= LCDROWMAX)
				cursor_row = 0;
		}
	}
}
*/
// this is for 8x8 font
void writeChar(unsigned char ch) {
	unsigned char j;

	if (cursor_col > LCDCOLMAX - 6)
		cursor_col = LCDCOLMAX - 6; // ensure space is available for the character
	if (cursor_row > LCDROWMAX - 1)
		cursor_row = LCDROWMAX - 1; // ensure space is available for the character
	lcd_buffer[cursor_row][cursor_col] = 0x00;
	for (j = 0; j < 7; j++) {
		lcd_buffer[cursor_row][cursor_col + j] = (font8x8[ch][j]);
	}

	lcd_buffer[cursor_row][cursor_col + 6] = 0x00;

	for (j = 0; j < 7; j++) {
		writeData(lcd_buffer[cursor_row][cursor_col++]);
		if (cursor_col >= LCDCOLMAX) {
			cursor_col = 0;
			cursor_row++;
			if (cursor_row >= LCDROWMAX)
				cursor_row = 0;
		}
	}
}

/*
 * Name         : gotoXY
 * Description  : Move text position cursor to specified position
 * Argument(s)  : x, y - Position, x = 0-83, y = 0-6
 * Return value : none
 */
void gotoXY(unsigned char x, unsigned char y) {
	if (x > LCDCOLMAX - 1)
		x = LCDCOLMAX - 1; // ensure within limits
	if (y > LCDROWMAX - 1)
		y = LCDROWMAX - 1; // ensure within limits

	writeCommand(0x80 | x); //column
	writeCommand(0x40 | y); //row

	cursor_row = y;
	cursor_col = x;
}

/*
 * Name         : clear
 * Description  : Clear the screen and display buffer
 * Argument(s)  : none
 * Return value : none
 */
void clear(void) {
	int i, j;

	gotoXY(0, 0); //start with (0,0) home position

	for (i = 0; i < LCDROWMAX; i++) {
		for (j = 0; j < LCDCOLMAX; j++) {
			writeData(0x00);
			lcd_buffer[i][j] = 0x00;
		}
	}

	gotoXY(0, 0); //bring the XY position back to (0,0)
}

/*
 * Name         : clearSome
 * Description  : Clear part of the screen
 * Argument(s)  : xs, ys are x and y start points for top left corner of rectangle,
 * xe, ye are end points for lower left corner of rectangle
 * Return value : none
 */
void clearSome(unsigned char xs, unsigned char ys, unsigned char xe, unsigned char ye) {

   drawFilledRectangle(xs,ys,xe,ye,0); // PIXEL_OFF hard coded to 0
}

/*
 * Name   : update
 * Description  : Write the screen buffer to the display memory
 * Argument(s)  : none
 * Return value : none
 */
void update(void) {
	int i, j;

	for (i = 0; i < LCDROWMAX; i++) {
		gotoXY(0, i);
		for (j = 0; j < LCDCOLMAX; j++) {
			writeData(lcd_buffer[i][j]);
		}
	}
	gotoXY(0, 0); //bring the XY position back to (0,0)
}

/*
 * Name         : setPixel
 * Description  : Set a single pixel either on or off, update display buffer.
 * Argument(s)  : x,y - position, x = 0-83, y = 0-6
 *                c - colour, either PIXEL_ON, PIXEL_OFF or PIXEL_XOR
 * Return value : none
 */
void setPixel(unsigned char x, unsigned char y, unsigned char c) {
	unsigned char value;
	unsigned char row;

	//if (x < 0 || x >= LCDCOLMAX || y < 0 || y >= LCDPIXELROWMAX)
		//return;

	row = y / 8;

	value = lcd_buffer[row][x];
	if (c == PIXEL_ON) {
		value |= (1 << (y % 8));
	} else if (c == PIXEL_XOR) {
		value ^= (1 << (y % 8));
	} else {
		value &= ~(1 << (y % 8));
	}

	lcd_buffer[row][x] = value;

	gotoXY(x, row);
	writeData(value);
}

/*
 * Name         : drawLine
 * Description  : Draws a line between two points on the display.
 * Argument(s)  : x1, y1 - Absolute pixel coordinates for line origin.
 *                x2, y2 - Absolute pixel coordinates for line end.
 *                c - either PIXEL_ON, PIXEL_OFF or PIXEL_XOR
 * Return value : none
 */
void drawLine(unsigned char x1, unsigned char y1, unsigned char x2,
		unsigned char y2, unsigned char c) {
	int dx, dy, stepx, stepy, fraction;

	/* Calculate differential form */
	/* dy   y2 - y1 */
	/* -- = ------- */
	/* dx   x2 - x1 */

	/* Take differences */
	dy = y2 - y1;
	dx = x2 - x1;

	/* dy is negative */
	if (dy < 0) {
		dy = -dy;
		stepy = -1;
	} else {
		stepy = 1;
	}

	/* dx is negative */
	if (dx < 0) {
		dx = -dx;
		stepx = -1;
	} else {
		stepx = 1;
	}

	dx <<= 1;
	dy <<= 1;

	/* Draw initial position */
	setPixel(x1, y1, c);

	/* Draw next positions until end */
	if (dx > dy) {
		/* Take fraction */
		fraction = dy - (dx >> 1);
		while (x1 != x2) {
			if (fraction >= 0) {
				y1 += stepy;
				fraction -= dx;
			}
			x1 += stepx;
			fraction += dy;

			/* Draw calculated point */
			setPixel(x1, y1, c);
		}
	} else {
		/* Take fraction */
		fraction = dx - (dy >> 1);
		while (y1 != y2) {
			if (fraction >= 0) {
				x1 += stepx;
				fraction -= dy;
			}
			y1 += stepy;
			fraction += dx;

			/* Draw calculated point */
			setPixel(x1, y1, c);
		}
	}
}

/*
 * Name         : drawRectangle
 * Description  : Draw a rectangle given the top left and bottom right points
 * Argument(s)  : x1, y1 - Absolute pixel coordinates for top left corner
 *                x2, y2 - Absolute pixel coordinates for bottom right corner
 *                c - either PIXEL_ON, PIXEL_OFF or PIXEL_XOR
 * Return value : none
 */
void drawRectangle(unsigned char x1, unsigned char y1, unsigned char x2,
		unsigned char y2, unsigned char c) {
	drawLine(x1, y1, x2, y1, c);
	drawLine(x1, y1, x1, y2, c);
	drawLine(x1, y2, x2, y2, c);
	drawLine(x2, y1, x2, y2, c);
}

/*
 * Name         : drawFilledRectangle
 * Description  : Draw a filled rectangle given the top left and bottom right points
 * 		  just simply draws horizontal lines where the rectangle would be
 * Argument(s)  : x1, y1 - Absolute pixel coordinates for top left corner
 *                x2, y2 - Absolute pixel coordinates for bottom right corner
 *                c - either PIXEL_ON, PIXEL_OFF or PIXEL_XOR
 * Return value : none
 */
void drawFilledRectangle(unsigned char x1, unsigned char y1, unsigned char x2,
		unsigned char y2, unsigned char c) {
	int i;
	for (i = y1; i <= y2; i++) {
		drawLine(x1, i, x2, i, c);
	}
}

/*
 * Name         : drawCircle
 * Description  : Draw a circle using Bresenham's algorithm.
 * 		  Some small circles will look like squares!!
 * Argument(s)  : xc, yc - Center of circle
 * 		  r - Radius
 * 		  c - either PIXEL_ON, PIXEL_OFF or PIXEL_XOR
 * Return value : None
 */
void drawCircle(unsigned char xc, unsigned char yc, unsigned char r,
		unsigned char c) {

	int ddF_x = 1;
	int ddF_y = -2 * r;
	int f = 1 - r;
	int x = 0;
	int y = r;

	setPixel(xc, yc + r, c);
	setPixel(xc, yc - r, c);
	setPixel(xc + r, yc, c);
	setPixel(xc - r, yc, c);

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		setPixel((xc + x), (yc + y), c);
		setPixel((xc - x), (yc + y), c);
		setPixel((xc + x), (yc - y), c);
		setPixel((xc - x), (yc - y), c);

		setPixel((xc + y), (yc + x), c);
		setPixel((xc - y), (yc + x), c);
		setPixel((xc + y), (yc - x), c);
		setPixel((xc - y), (yc - x), c);
	}

}

/*Name: drawFilledCircle
 * Description  : Draws a filled circle
 * Argument(s)  : x, y are center of circle, r is the radius
 * Returns : None
 */

void drawFilledCircle(unsigned char x, unsigned char y, unsigned char radius) {
	int dx = radius;
	int dy = 0;
	int xChange = 1 - 2 * radius;
	int yChange = 1;
	int radiusError = 0;
	while (dx >= dy) {
		drawLine(x + dy, y + dx, x - dy, y + dx, 1);    //hard coded PIXEL_ON
		drawLine(x - dy, y - dx, x + dy, y - dx, 1);
		drawLine(x - dx, y + dy, x + dx, y + dy, 1);
		drawLine(x - dx, y - dy, x + dx, y - dy, 1);
		dy++;
		radiusError += yChange;
		yChange += 2;
		if (2 * radiusError + xChange > 0) {
			dx--;
			radiusError += xChange;
			xChange += 2;
		}
	}
}

/*
 * Name         : printV
 * Description  : prints a word vertically
 * Argument(s)  : x , y , length of word (can't be longer than 6 letters)
 * Return value : None
 */

void printV(unsigned char x, unsigned char y, unsigned char length,
		char *characters) {
	for (length = 0; length <= 5; length++) {
		gotoXY(x, y);
		y++;
		writeChar(*characters++);
	}
}

