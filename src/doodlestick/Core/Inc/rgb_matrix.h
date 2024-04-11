/*
 * rgb_matrix.h
 *
 *  Created on: Dec 7, 2023
 *      Author: jackkrammer
 */

#ifndef INC_RGB_MATRIX_H_
#define INC_RGB_MATRIX_H_


// defines
#define NUM_ROWS 16 // changed for testing, trying to only have the 1st and 9th row be on //16
#define NUM_COLS 32 // changed for testing, trying to only have the left two cols be on //32
#define HIGH 1
#define LOW 0

// typedefs
typedef struct color
{
	uint8_t r,g,b;
} color;

// function declarations
void matrix_init(); // initializes the Adafruit 32x16 RGB LED matrix
void matrix_begin(); // initializes the matrix values to not latch data and disable output

void update_display(); // updates the display with the matrix buffer
void clear_matrix(); // clears the matrix buffer and updates the display
void fill_matrix(color c); // fills the matrix with the selected color

void draw_pixel(uint8_t x, uint8_t y, color c); // draws the selected color at the input coordinate then updates display
void clear_pixel(uint8_t x, uint8_t y); // removes the pixel at the input coordinates from the buffer then updates the display

void make_smiley(color c); // makes a 'relief' smiley face with the background color as the input
void make_hi(color c); // makes a relief hi
void make_logo(color c); // makes the doodlestick name with logo

void set_matrix_section(uint8_t row); // sets the row select bits of the matrix to the desired value
void set_LAT(uint8_t val); // sets the latch pin HIGH if val > 0 and LOW if val == 0
void set_OE(uint8_t val); // sets the output enable pin HIGH if val > 0 and LOW if val == 0
void drive_matrix_clk(); // drives the matrix clock high then low, for a idle-low rising-edge clock
void set_RGB_val(color upper, color lower); // sets the RGB values according to the input colors
void clear_RGB_val(); // clears the values in the RGB pins' ODR




// makes the doodlestick name with logo
void make_logo(color c)
{
	// draw doodle
	// d
	draw_pixel(3,1,c);
	draw_pixel(3,2,c);
	draw_pixel(3,3,c);
	draw_pixel(3,4,c);
	draw_pixel(3,5,c);
	draw_pixel(2,5,c);
	draw_pixel(1,5,c);
	draw_pixel(1,4,c);
	draw_pixel(1,3,c);
	draw_pixel(2,4,c);
	// o
	draw_pixel(5,5,c);
	draw_pixel(5,4,c);
	draw_pixel(5,3,c);
	draw_pixel(6,3,c);
	draw_pixel(7,3,c);
	draw_pixel(7,4,c);
	draw_pixel(7,5,c);
	draw_pixel(6,5,c);

	// draw stick

	// draw logo

}

// makes a relief hi
void make_hi(color c)
{
	fill_matrix(c);

	clear_pixel(1,1);
	clear_pixel(1,2);
	clear_pixel(1,3);
	clear_pixel(1,4);
	clear_pixel(1,5);

	clear_pixel(2,3);

	clear_pixel(3,1);
	clear_pixel(3,2);
	clear_pixel(3,3);
	clear_pixel(3,4);
	clear_pixel(3,5);

	clear_pixel(5,2);
	clear_pixel(5,4);
	clear_pixel(5,5);
}

// makes a 'relief' smiley face with the background color as the input
void make_smiley(color c)
{
	fill_matrix(c);

	clear_pixel(3,2);
	clear_pixel(5,2);
	clear_pixel(2,4);
	clear_pixel(3,5);
	clear_pixel(4,5);
	clear_pixel(5,5);
	clear_pixel(6,4);
}

// sets the row select bits of the matrix to the desired value
void set_matrix_section(uint8_t row)
{
	/*
	 * sets the appropriate value to the row select pins below
	 * 		A(LSB)  B 	 C(MSB)
	 * 		PC7     PC8  PC9
	 */

	// variables
//	uint8_t a = row & 0x01; // LSB of row select
//	uint8_t b = row & 0x02; //
//	uint8_t c = row & 0x04; // MSB of row select

	// clears pins
	GPIOC->ODR &= ~((1 << 7) | (1 << 8) | (1 << 9));

	// sets pins to value
//	GPIOC->ODR |= (((!!c) << 9) | ((!!b) << 8) | ((!!a) << 7)); // works
//	GPIOC->ODR |= (((row & 0x04) << 9) | ((row & 0x02) << 8) | ((row & 0x01) << 7)); // doesn't work

	if(row & 0x01) GPIOC->ODR |= (1 << 7); // set LSB of ADDR bits
	if(row & 0x02) GPIOC->ODR |= (1 << 8);
	if(row & 0x04) GPIOC->ODR |= (1 << 9); // set MSB of ADDR bits
}

// sets the latch pin HIGH if val > 0 and LOW if val == 0
void set_LAT(uint8_t val)
{
	/*
	 * sets the chosen latch value to the ODR
	 * 		LAT
	 * 		PC11
	 */

	// sets the pin low
	GPIOC->ODR &= ~(1 << 11);

	// sets the pin high if input is > 0
	if(val > 0)
		GPIOC->ODR |= (1 << 11);
}

// sets the output enable pin HIGH if val > 0 and LOW if val == 0
void set_OE(uint8_t val)
{
	/*
	 * sets the chosen output enable value to the ODR
	 * 		OE
	 * 		PC12
	 */

	// sets the pin low
	GPIOC->ODR &= ~(1 << 12);

	// sets the pin high if input is > 0
	if(val > 0)
		GPIOC->ODR |= (1 << 12);
}

// drives the matrix clock high then low, for a idle-low rising-edge clock
void drive_matrix_clk()
{
	GPIOC->BSRR |= (1 << 10); // sets clock HIGH
	GPIOC->BRR |= (1 << 10);  // sets clock LOW
}

// sets the RGB values according to the input colors
void set_RGB_val(color upper, color lower)
{
	/*
	 * RGB color pins
	 * 		|---upper---|  |---lower---|
	 * 		R1   G1   B1   R2   G2   B2
	 * 		PB0  PB1  PB2  PB3  PB4  PB5
	 */

	// clear the previous values in the ODR
//	GPIOB->ODR &= ~((1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5));

	// sets the upper and lower rgb values at the same time
	GPIOB->ODR |= ((upper.r << 0) | (upper.g << 1) | (upper.b << 2)
					| (lower.r << 3) | (lower.g << 4) | (lower.b << 5));
}

// clears the values in the RGB pins' ODR
void clear_RGB_val()
{
	/*
	 * RGB color pins
	 * 		|---upper---|  |---lower---|
	 * 		R1   G1   B1   R2   G2   B2
	 * 		PB0  PB1  PB2  PB3  PB4  PB5
	 */

	// clear the previous values in the ODR
	GPIOB->ODR &= ~((1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5));
}

// initializes the matrix values to not latch data and disable output
void matrix_begin()
{
	/*
	 * RGB pins to be zero
	 * ADDR pins to be zero
	 * CLK pin to be idle low
	 * LAT pin to be low b/c not latching any data
	 * OE pin to be high to disable output
	 */

	// RGB pins
	GPIOB->ODR &= ~((1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5));

	// ADDR pins, CLK pin, LAT pin
	GPIOC->ODR &= ~((1 << 7) | (1 << 8) | (1 << 9) | (1 << 10) | (1 << 11));

	// OE pin
	GPIOC->ODR |= (1 << 12);

	// addr pins
//	GPIOC->ODR |= ((0 << 9) | (0 << 8) | (1 << 7)); // sets address to 1

	// initializes the buffer to be all dark
	clear_matrix();
}

// initializes the Adafruit 32x16 RGB LED matrix
void matrix_init()
{
	/*
	 * initializes the matrix input pins
	 * color pins are the data pins that control the color of the current matrix section
	 * 		|---upper---|  |---lower---|
	 * 		R1   G1   B1   R2   G2   B2
	 * 		PB0  PB1  PB2  PB3  PB4  PB5
	 * row select pins are are the pins that control the current section of the matrix
	 * 		A(LSB)  B 	 C(MSB)
	 * 		PC7     PC8  PC9
	 * clock pin is the input clock to the matrix
	 * 		CLK
	 * 		PC10
	 * latch pin is the pin to signal the end of a row of data
	 * 		LAT
	 * 		PC11
	 * output enable pin is the pin to turn the LEDs off when moving from one row to next
	 * 		OE
	 * 		PC12
	 */

	// enable clock for port B and C
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;

	// set color pins as outputs
	GPIOB->MODER &= ~(GPIO_MODER_MODE0
			| GPIO_MODER_MODE1
			| GPIO_MODER_MODE2
			| GPIO_MODER_MODE3
			| GPIO_MODER_MODE4
			| GPIO_MODER_MODE5);
	GPIOB->MODER |= (GPIO_MODER_MODE0_0
			| GPIO_MODER_MODE1_0
			| GPIO_MODER_MODE2_0
			| GPIO_MODER_MODE3_0
			| GPIO_MODER_MODE4_0
			| GPIO_MODER_MODE5_0);

	// set row select pins as outputs
	GPIOC->MODER &= ~(GPIO_MODER_MODE7
				| GPIO_MODER_MODE8
				| GPIO_MODER_MODE9);
	GPIOC->MODER |= (GPIO_MODER_MODE7_0
				| GPIO_MODER_MODE8_0
				| GPIO_MODER_MODE9_0);

	// set clock, latch, output enable pins as output
	GPIOC->MODER &= ~(GPIO_MODER_MODE10
				| GPIO_MODER_MODE11
				| GPIO_MODER_MODE12);
	GPIOC->MODER |= (GPIO_MODER_MODE10_0
				| GPIO_MODER_MODE11_0
				| GPIO_MODER_MODE12_0);
}

#endif /* INC_RGB_MATRIX_H_ */
