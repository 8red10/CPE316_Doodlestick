/*
 * JACK and SRINI
 *
 * CPE 316 Final Project
 * 	John Oliver
 *
 *
 * DOODLESTICK
 * 		doodle with a joystick
 * 		draws on a LED matrix
 *
 *
 * 		DEPENDENCIES
 *
 *
 */

// includes
#include "main.h"
#include <stdio.h>
#include "joystick.h"
#include "uart.h"
#include "keypad_12.h"
#include "rgb_matrix.h"
#include "timer2.h"

// defines
#define X_NEUTRAL 2050
#define Y_NEUTRAL 1950
#define THRESHOLD  900
#define BLINK_THRESHOLD 5
#define BUFF_SIZE 16

// typedefs
typedef enum KP_MODE {
		COLOR 	= 0xA,
		FILL 	= 0x0,
		DRAW 	= 0xB,
		SPEED 	= 0x9  // do we need thickness ? speed instead
} KP_MODE;

typedef struct point{
	int8_t x, y;
} point;


// function declarations
void TIM2_IRQHandler(void); // interrupt handler for TIM2
void move_cursor(); // moves the cursor in the direction indicated by the joystick
uint8_t check_color(point pt, color c); // returns 1 if color at point in matrix is same as input, returns 0 if not
void int_to_str(int num, char* buff); // converts and int and returns a string of length BUFF_SIZE
void USART_print_int(int num); // prints an int to USART
void reset_shapes(point line[2], point square[2], point triangle[3]); // resets the shapes to their defaults, coordinates = -1
void reset_this_shape(point* shape, int size); // resets the vars of this shape
void add_pt_to_shape(point* shape, uint8_t size); // adds the current cursor position to the shape
int8_t get_shape_index(point* shape, int size); // returns -1 or the index of the shape that isn't set yet

uint8_t get_max(uint8_t a, uint8_t b); // gets the minimum of two uint8_t values
uint8_t get_min(uint8_t a, uint8_t b); // gets the maximum of two uint8_t values

void draw_horizontal_line(uint8_t xmin, uint8_t xmax, uint8_t yconst); // draws a horizontal line given the xmin, xmax and y values
void draw_vertical_line(uint8_t ymin, uint8_t ymax, uint8_t xconst); // draws a vertical line given ymin, ymax, and x values
void draw_rect(point p1, point p2); // draws a rectangle with the two points as opposite corners
void draw_shape(point* shape, int size); // draws the given shape
uint8_t same_point(point p1, point p2); // returns 1 if the points have the same coordinates and 0 if they don't
uint8_t pt_inbounds(point pt); // returns 1 if the point is within the bounds of the matrix and 0 if not


// colors
const color RED   	= {.r = 1, .g = 0, .b = 0};
const color GREEN 	= {.r = 0, .g = 1, .b = 0};
const color BLUE  	= {.r = 0, .g = 0, .b = 1};
const color WHITE   = {.r = 1, .g = 1, .b = 1};
const color BLACK 	= {.r = 0, .g = 0, .b = 0};
const color PURPLE	= {.r = 1, .g = 0, .b = 1};
const color YELLOW 	= {.r = 1, .g = 1, .b = 0};
const color CYAN	= {.r = 0, .g = 1, .b = 1};
//const color NONE 	= {.r = 2, .g = 2, .b = 2};

/*
 * matrix buffer is indexed as [x][y]
 *
 * convention (x,y):
 * 	top left is (0,0)
 * 	bottom right is (31,15)
 *
 */
color matrix_buffer[NUM_COLS][NUM_ROWS];




// cursor variables
point 	cursor_pos 			= {.x = 0, .y = 0}; // cursor starts at top lef corner
point 	prev_pos			= {.x = 0, .y = 0};		// holds the previous position of the cursor
color 	prev_color			= RED;			// holds the previous color when blinking
uint8_t cursor_thickness 	= 1;				// default cursor thickness is radius of 1
color 	draw_color 			= RED;				// need to check whether the color is NONE or not before drawing anything
uint8_t drawing				= 0; 				// 1 = trace mode, 0 = don't trace joystick movement

// more variables
volatile uint16_t 	xcoord_data 		= X_NEUTRAL;
volatile uint16_t 	ycoord_data 		= Y_NEUTRAL;
volatile uint8_t 	xcoord_adc_flag 	= 1;
volatile uint8_t 	ycoord_adc_flag 	= 1;
//uint8_t enable_serial = 0; 		// if 1 then send updates over UART, essentially is debugging
volatile uint8_t 	timer_flag 			= 1;
		 uint8_t	button_flag		 	= 0;

int main()
{
	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	// initialize the joystick
	SystemClock_Config(); 	// sets system clock to 32MHz
	xcoord_adc_init();		// initializes the ADC for the joystick x data
	ycoord_adc_init();		// initializes the ADC for the joystick y data
	joystick_button_init();	// initializes the button on the joystick

	// initializes the keypad
	keypad_init();

	// initializes the matrix
	matrix_init();			// initializes the matrix pins
	matrix_begin();			// sets the initial matrix pin values

	// sets the initial display
	make_smiley(CYAN); // make_hi(PURPLE);

	// initializes the USART serial output
	USART_init();

	// sets the initial serial output
	USART_Print("doodlestick\n\r");
	USART_Print("	by Jack and Srini\n\n\r");

	// initialize the speed timer
	TIM2_init(320000, 9, 0xFFFFFFFF); // initializes timer with values (arr, psc, ccr1)

	// variables
	KP_MODE kp_mode = DRAW;
	int8_t kp_select = 2;
	int8_t kp_ret = -1;
	uint8_t button_ret = 0;
	color colors[8] = { RED,   	GREEN, 	BLUE,
						YELLOW, CYAN, 	PURPLE,
						WHITE, 	BLACK 	};

	// shape variables
	point line[2] 		= { {.x = -1, .y = -1}, {.x = -1, .y = -1} };
	point square[2] 	= { {.x = -1, .y = -1}, {.x = -1, .y = -1} };
	point triangle[3]	= { {.x = -1, .y = -1}, {.x = -1, .y = -1} , {.x = -1, .y = -1} };

	while(1)
	{
		// update joystick values, this could be more efficient if does this via a timer
		// update x coordinate joystick value
		if(xcoord_adc_flag)
		{
			// reset flag
			xcoord_adc_flag = 0;

			// start another conversion
			xcoord_start_conversion();
		}
		// update y coordinate joystick value
		if(ycoord_adc_flag)
		{
			// reset flag
			ycoord_adc_flag = 0;

			// start another conversion
			ycoord_start_conversion();
		}

		// poll keypad, store as value
		kp_ret = loop_keypad_once(); // gets index of keypad
		if(kp_ret >= 0) // valid command
		{
			// get the value on the button instead of the index
			kp_ret = keypad_vals[kp_ret];

			// check if keypad press was a MODE configuration
			if(kp_ret > 8 || kp_ret == 0)
			{
				switch(kp_ret)
				{
				case 0xA: 	// * = COLOR SELECT MODE
					kp_mode = COLOR;
					kp_select = -1; // default is no selection
					break;
				case 0x0: 	// 0 = FILL SELECT MODE
					kp_mode = FILL;
					kp_select = -1; // default is no selection
					break;
				case 0xB: 	// # = DRAW SELECT MODE
					kp_mode = DRAW;
					reset_shapes(line, square, triangle);
					kp_select = -1; // default is no selection
					break;
				case 0x9: 	// 9 = SPEED SELECT MODE
					kp_mode = SPEED;
					kp_select = -1; // default is no selection
					break;
				default: 	// else error, don't change anything
					break;
				}
			}
			else // keypad press changes the kp_select option
			{
				kp_select = kp_ret;
			}
		}

		// execute command chosen
		switch(kp_mode)
		{
		case COLOR:		// COLOR mode 	= changes color of draw tool
			if(kp_select >= 0 && kp_select < 9)
			{
				draw_color = colors[kp_select - 1];
			}
			break;

		case FILL:		// FILL mode  	= allows a decision on what color to fill the board with
			if(kp_select >= 0 && kp_select < 9)
			{
				fill_matrix(colors[kp_select - 1]);
			}
			kp_select = -1;
			break;

		case DRAW:		// DRAW mode  	= changes type of draw tool
			switch(kp_select)
			{
			case 1:	// free 	= don't change matrix
				drawing = 0;
				break;
			case 2: // trace 	= draw where moving
				drawing = 1;
				break;
			case 3: // line		= can click twice and draw a line between the two spots clicked
				drawing = 0;
				if(button_flag)
				{
					add_pt_to_shape(line, 2);
					// reset button flag
					button_flag = 0;
				}
				break;
			case 4: // square	= can click twice and draw a square with two spots clicked as opposite corners
				drawing = 0;
				if(button_flag)
				{
					add_pt_to_shape(line, 4);
					// reset button flag
					button_flag = 0;
				}
				break;
			case 5: // triangle = can click three times and draw triangle with 3 spots clicked as vertices
				drawing = 0;
				if(button_flag)
				{
					add_pt_to_shape(line, 3);
					// reset button flag
					button_flag = 0;
				}
				break;
			case 6: // thick 	= cursor now 1 dot // now is going to be the cyan w smiley relief
				make_smiley(CYAN); // turn into logo
				break;
			case 7: // thiick	= cursor now 2 dots radius thickness // now is purple hi or cyan smiley
				if(xcoord_data % 2) make_hi(PURPLE);
				else make_smiley(CYAN);
				kp_select = -1;
				break;
			case 8: // thiiick	= cursor now 3 dots radius thickness // now is going to be a continous fill of blank
				fill_matrix(colors[kp_select - 1]);
				break;
			default:	// error in option selected, do nothing
				break;
			}
			break;

		case SPEED:		// SPEED mode 	= changes speed of cursor movement
			TIM2->PSC = kp_select + 1; // larger number = slower speed
			break;

		default:		// invalid mode, do nothing
			break;
		}

		// move cursor
		if(timer_flag)
		{
			move_cursor();
			timer_flag = 0;

//			// print status
//			char buff[BUFF_SIZE];
//			USART_Print("    mode = ");
//			USART_print_int(kp_mode);
//			USART_Print("	 select = ");
//			USART_print_int(kp_select);
//			USART_Print("\n\r");
		}

		// update the display
		update_display();
	}


	return 0;
}




/* ------------------- INTERRUPT FUNCTIONS ------------------- */

// interrupt handler for TIM2
void TIM2_IRQHandler(void)
{
	// if from TIM2 update event
	if(TIM2->SR & TIM_SR_UIF) // from ARR
	{
		TIM2->SR &= ~TIM_SR_UIF; 	// reset interrupt flag
		timer_flag = 1;				// set global timer flag
	}
}

// clear flag and set globals
void ADC1_2_IRQHandler()
{
	// Vref for X is the input to ADC1
	if (ADC1->ISR & ADC_ISR_EOC)
	{
		xcoord_data = ADC1->DR;
		xcoord_adc_flag = 1;
	}

	// Vref for Y is the input to ADC2
	if(ADC2->ISR & ADC_ISR_EOC)
	{
		ycoord_data = ADC2->DR;
		ycoord_adc_flag = 1;
	}
}


/* -------------------- SERIAL FUNCTIONS -------------------- */

// prints an int to USART
void USART_print_int(int num)
{
	char buff[BUFF_SIZE];
	int_to_str(num, buff);
	USART_Print(buff);
}
// converts and int and returns a string of length BUFF_SIZE
void int_to_str(int num, char* buff)
{
	snprintf(buff, /*sizeof(buff)*/BUFF_SIZE, "%d", num);
}

/* ------------------ JOYSTICK FUNCTIONS ------------------ */

// moves the cursor in the direction indicated by the joystick
void move_cursor()
{
	// update the previous cursor position
	prev_pos = cursor_pos;

	// check x+ (right) direction
	if(xcoord_data > X_NEUTRAL + THRESHOLD) // joystick wants to move right
	{
		if(cursor_pos.x < NUM_COLS - 1) 	// room to move right
		{
			cursor_pos.x++;
		}
	}

	// check x- (left) direction
	if(xcoord_data < X_NEUTRAL - THRESHOLD) // joystick wants to move left
	{
		if(cursor_pos.x > 0) 				// room to move left
		{
			cursor_pos.x--;
		}
	}

	// check y+ (down) direction
	if(ycoord_data > Y_NEUTRAL + THRESHOLD) // joystick wants to move down
	{
		if(cursor_pos.y < NUM_ROWS - 1) 	// room to move down
		{
			cursor_pos.y++;
		}
	}

	// check y- (up) direction
	if(ycoord_data < Y_NEUTRAL - THRESHOLD) // joystick wants to move up
	{
		if(cursor_pos.y > 0)
		{
			cursor_pos.y--;
		}
	}

	// create color at the location if drawing
	if(drawing)
	{
		draw_pixel(cursor_pos.x, cursor_pos.y, draw_color);
	}
	static uint8_t state = 0;
	// check if the cursor is in the previous position
	if(cursor_pos.x == prev_pos.x && cursor_pos.y == prev_pos.y) // cursor is at the previous position, blink cursor
	{
		static uint8_t blink_count = 0;
		if(blink_count == BLINK_THRESHOLD)
		{
			if(state) //check_color(cursor_pos, WHITE)) // matrix color at cursor is WHITE
			{
				draw_pixel(cursor_pos.x, cursor_pos.y, prev_color); // BLACK ? // prev_color
				state = 0;
			}
			else
			{
				draw_pixel(cursor_pos.x, cursor_pos.y, check_color(cursor_pos, BLACK) ? WHITE : BLACK); // WHITE
				state = 1;
			}
			// reset blink count
			blink_count = 0;
			// check the button status
			button_flag = get_joystick_button();
		}
		else
		{
			// increment blink count
			blink_count++;
		}
	}
	else // need to replace the color at the previous position, and update prev_color for next time
	{
		draw_pixel(prev_pos.x, prev_pos.y, prev_color);
		prev_color = matrix_buffer[cursor_pos.x][cursor_pos.y];
		state = 0;
	}
}

/* ------------------- MATRIX FUNCTIONS ------------------- */

// adds the current cursor position to the shape
void add_pt_to_shape(point* shape, uint8_t size)
{
	// variables
	uint8_t size_to_use = size == 4 ? 2 : size;
//	uint8_t reset_shape = 1;
	int8_t pt_index = get_shape_index(shape, size_to_use);

	// resets shape if need be
	if(pt_index < 0)
	{
		reset_this_shape(shape, size_to_use);
		pt_index = 0;
	}

	// adds current cursor point to shape
	shape[pt_index].x = cursor_pos.x;
	shape[pt_index].y = cursor_pos.y;

	// check if shape is full now
	pt_index = get_shape_index(shape, size_to_use);

	// if is full, draw shape and reset shape
	if(pt_index < 0)
	{
		draw_shape(shape, size); // keep as original size
		reset_this_shape(shape, size_to_use);
	}

}

// draws the given shape
void draw_shape(point* shape, int size)
{
	// check to see which shape this is
	switch(size)
	{
	case 2: // line
		break;
	case 3: // triangle
		break;
	case 4: // square
		draw_rect(shape[0], shape[1]);
		break;
	default:
		break;
	}
}

// draws a line between the two points
void draw_line(point p1, point p2)
{
	// variables
	point next; // where starting from
	point dest; // where point going to

	// going from left to right, organize points as such
	if(p1.x <= p2.x) // p1 is on left and p2 is on right
	{
		next.x = p1.x;
		next.y = p1.y;
		dest.x = p2.x;
		dest.y = p2.y;
	}
	else			 // p2 is on left and p1 is on right
	{
		next.x = p2.x;
		next.y = p2.y;
		dest.x = p1.x;
		dest.y = p1.y;
	}

	// draw the endpoints
	draw_pixel(p1.x, p1.y, draw_color);
	draw_pixel(p2.x, p2.y, draw_color);

	// draw everything between
	while(!(same_point(next, dest)) && pt_inbounds(next))
	{
		// test for case where slope would be undefined
		if(next.x == dest.x)
		{

		}

		int slope = get_slope(p1,p2); // gets the slope * 1,000,000
	}

}

// draws a rectangle with the two points as opposite corners
void draw_rect(point p1, point p2)
{
	// variables
	uint8_t xmin = get_min(p1.x, p2.x);
	uint8_t xmax = get_max(p1.x, p2.x);
	uint8_t ymin = get_min(p1.y, p2.y);
	uint8_t ymax = get_max(p1.y, p2.y);

	// draw left vertical line
	draw_vertical_line(ymin, ymax, xmin);
	// draw right vertical line
	draw_vertical_line(ymin, ymax, xmax);
	// draw top line
	draw_horizontal_line(xmin, xmax, ymin);
	// draw bottom line
	draw_horizontal_line(xmin, xmax, ymax);
}

// draws a vertical line given ymin, ymax, and x values
void draw_vertical_line(uint8_t ymin, uint8_t ymax, uint8_t xconst)
{
	for(int y = ymin; y <= ymax; y++)
	{
		draw_pixel(xconst, y, draw_color);
	}
}

// draws a horizontal line given the xmin, xmax and y values
void draw_horizontal_line(uint8_t xmin, uint8_t xmax, uint8_t yconst)
{
	for(int x = xmin; x <= xmax; x++)
	{
		draw_pixel(x, yconst, draw_color);
	}
}

// gets the maximum of two uint8_t values
uint8_t get_min(uint8_t a, uint8_t b)
{
	if(a < b) return a;
	else return b;
}

// gets the minimum of two uint8_t values
uint8_t get_max(uint8_t a, uint8_t b)
{
	if(a > b) return a;
	else return b;
}

// returns 1 if the point is within the bounds of the matrix and 0 if not
uint8_t pt_inbounds(point pt)
{
	return pt.x >= 0 && pt.x < NUM_COLS && pt.y >= 0 && pt.y < NUM_ROWS;
}

// returns 1 if the points have the same coordinates and 0 if they don't
uint8_t same_point(point p1, point p2)
{
	return p1.x == p2.x && p1.y == p2.y;
}

// gets the slope times 1 million, to avoid using floats
int get_slope(point p1, point p2)
{
	int numerator 	= p1.y - p2.y * 1000000;
	int denominator = p1.x - p2.x;
	return numerator / denominator;
}

// returns -1 or the index of the shape that isn't set yet
int8_t get_shape_index(point* shape, int size)
{
	uint8_t pt_index = -1;
	for(int i = 0; i < size; i++)
	{
		if(shape[i].x < 0 || shape[i].y < 0)
		{
			pt_index = i;
		}
	}
	return pt_index;
}

// resets the shapes to their defaults, coordinates = -1
void reset_shapes(point line[2], point square[2], point triangle[3])
{
	line[0].x = -1;
	line[0].y = -1;
	line[1].x = -1;
	line[1].y = -1;

	square[0].x = -1;
	square[0].y = -1;
	square[1].x = -1;
	square[1].y = -1;

	triangle[0].x = -1;
	triangle[0].y = -1;
	triangle[1].x = -1;
	triangle[1].x = -1;
	triangle[2].x = -1;
	triangle[2].x = -1;
}

// resets the vars of this shape
void reset_this_shape(point* shape, int size)
{
	for(int i = 0; i < size; i++)
	{
		shape[i].x = -1;
		shape[i].y = -1;
	}
}

// returns 1 if color at point in matrix is same as input, returns 0 if not
uint8_t check_color(point pt, color c)
{
	// check if point is inside the matrix
	if(pt.x >= 0 && pt.x < NUM_COLS && pt.y >= 0 && pt.y < NUM_ROWS)
	{
		color mx_color = matrix_buffer[pt.x][pt.y];
		return mx_color.r == c.r && mx_color.g == c.g && mx_color.b == c.b;
	}
	return 0;
}

// removes the pixel at the input coordinates from the buffer then updates the display
void clear_pixel(uint8_t x, uint8_t y)
{
	matrix_buffer[x][y] = BLACK;
	update_display();
}

// draws the selected color at the input coordinate then updates display
void draw_pixel(uint8_t x, uint8_t y, color c)
{
	matrix_buffer[x][y] = c;
	update_display();
}

// fills the matrix with the selected color
void fill_matrix(color c)
{
	// variables
	uint8_t row, col;

	// loop through matrix
	for(col = 0; col < NUM_COLS; col++)
	{
		for(row = 0; row < NUM_ROWS; row++)
		{
			matrix_buffer[col][row] = c;
		}
	}
}

// clears the matrix buffer and updates the display
void clear_matrix()
{
	// variables
	uint8_t row, col;

	// loop through matrix
	for(col = 0; col < NUM_COLS; col++)
	{
		for(row = 0; row < NUM_ROWS; row++)
		{
			matrix_buffer[col][row] = BLACK;
		}
	}

	// updates the display
	update_display();
}

// updates the display with the matrix buffer
void update_display()
{
	// variables
	uint8_t row, col;

	// initialize matrix control variables
	set_OE(HIGH); // disable output
	set_LAT(HIGH); // latch current data

	// loop through the row sections
	for(row = 0; row < NUM_ROWS / 2; row++)
	{
		// get ready to clock in a section (2 rows) of data
		set_OE(LOW); // enables output
		set_LAT(LOW); // removes latch from previous data

		// loop through the columns of the row
		for(col = 0; col < NUM_COLS; col++)
		{
			// sets the desired upper, lower color
			set_RGB_val(matrix_buffer[col][row], matrix_buffer[col][row + (NUM_ROWS / 2)]);

			// drive the clock
			drive_matrix_clk();

			// clear the previous values in the ODR
			clear_RGB_val(); // if isn't cleared, will fill entire row
		}

		// allow matrix to hold data
		set_OE(HIGH); // disable output, moving to another row
		set_LAT(HIGH); // latch current data, moving to another row

		// set row selection
		set_matrix_section(row);
	}
}


