/*
 * keypad_12.h
 *
 *  Created on: Oct 17, 2023
 *      Author: jackkrammer
 *
 *  header file for the COM-14662 keypad
 */

#ifndef SRC_KEYPAD_12_H_
#define SRC_KEYPAD_12_H_

#define NUM_COL 	3
#define NUM_ROW 	4

const char keypad_chars[NUM_ROW * NUM_COL] = {
	'1', '2', '3',
	'4', '5', '6',
	'7', '8', '9',
	'*', '0', '#'
};

const uint8_t keypad_vals[NUM_ROW * NUM_COL] = {
	0x1, 0x2, 0x3,
	0x4, 0x5, 0x6,
	0x7, 0x8, 0x9,
	0xA, 0x0, 0xB
};

void keypad_init(); // initializes keypad
int8_t loop_keypad_once(); // returns index of button pressed or -1 if nothing is pressed
int8_t multiplex_keypad(); // returns index of button pressed by multiplexing keypad until button is pressed
char get_keypad_char(); // returns the char of the button pressed, null if invalid or error
int8_t get_keypad_value(); // returns the value of the button pressed, -1 if invalid or error


// returns the value of the button pressed, -1 if invalid or error
int8_t get_keypad_value()
{
	int8_t index = multiplex_keypad(); // get the index

	if(index != -1)
	{
		return keypad_vals[index];
	}

	return -1;
}

// returns the char of the button pressed
char get_keypad_char()
{
	int8_t index = multiplex_keypad(); // get the index

	if(index != -1) // there is a valid index
	{
		return keypad_chars[index];
	}

	return '\0';
}

// returns index of button pressed or -1 if nothing is pressed
int8_t loop_keypad_once()
{
	uint8_t col, row = 0;

	// clear column ODR
	GPIOC->ODR &= ~(GPIO_ODR_OD4 | GPIO_ODR_OD5 | GPIO_ODR_OD6);

	// drive each column once
	for(col = 0; col < NUM_COL; col++)
	{
		// drive column
		GPIOC->BSRR = (1 << NUM_ROW) << col;

		// check rows
		for(row = 0; row < NUM_ROW; row++)
		{
			if(GPIOC->IDR & (1 << row))
			{
				return NUM_COL * row + col;
			}
		}

		// disable column
		GPIOC->BSRR = ((1 << GPIO_BSRR_BR0_Pos) << NUM_ROW) << col;
	}

	return -1;
}


// returns index of button pressed by multiplexing keypad until button is pressed
int8_t multiplex_keypad()
{
	uint8_t col, row = 0;

	// clear column ODR
	GPIOC->ODR &= ~(GPIO_ODR_OD4 | GPIO_ODR_OD5 | GPIO_ODR_OD6);

	// multiplex columns
	while(1)
	{
		// drive column
		GPIOC->BSRR = (1 << NUM_ROW) << col;

		// check rows
		for(row = 0; row < NUM_ROW; row++)
		{
			if(GPIOC->IDR & (1 << row))
			{
				return NUM_COL * row + col;
			}
		}

		// disable column
		GPIOC->BSRR = ((1 << GPIO_BSRR_BR0_Pos) << NUM_ROW) << col;

		// go to next column, loops through
		col = (col + 1) % NUM_COL;
	}

	return -1;
}

void keypad_init()
{
	/*
	 * initializes pins to drive(output) columns and read(input) from rows
	 * columns are on pins PC4(col 1) PC5(col 2) PC6(col 3)
	 * rows are on pins PC0(row 1) PC1(row 2) PC2(row 3) PC3(row 4)
	 * sets pull down resistors on row pins for better reads
	 * */

	// enable clock for port C
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;

	// drive columns on pins PC4(col 1) PC5(col 2) PC6(col 3)
	GPIOC->MODER &= ~(GPIO_MODER_MODE4 | GPIO_MODER_MODE5 | GPIO_MODER_MODE6); // clear pins
	GPIOC->MODER |= ( (1 << GPIO_MODER_MODE4_Pos) |
						(1 << GPIO_MODER_MODE5_Pos) |
						(1 << GPIO_MODER_MODE6_Pos) ); // set pins to output

	// read from rows on pins PC0(row 1) PC1(row 2) PC2(row 3) PC3(row 4)
	GPIOC->MODER &= ~(GPIO_MODER_MODE0 | GPIO_MODER_MODE1 |
						GPIO_MODER_MODE2 | GPIO_MODER_MODE3); // clear pins, sets them to input

	// set pull-down resistors for rows to prevent floating voltages (better for reading)
	GPIOC->PUPDR &= ~(GPIO_PUPDR_PUPD0 | GPIO_PUPDR_PUPD1 |
						GPIO_PUPDR_PUPD2 | GPIO_PUPDR_PUPD3); // clear pins
	GPIOC->PUPDR |= ( (2 << GPIO_PUPDR_PUPD0_Pos) |
						(2 << GPIO_PUPDR_PUPD1_Pos) |
						(2 << GPIO_PUPDR_PUPD2_Pos) |
						(2 << GPIO_PUPDR_PUPD3_Pos) ); // pull pins low

}

#endif /* SRC_KEYPAD_12_H_ */
